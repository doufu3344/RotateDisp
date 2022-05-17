#include "gy_25t_ttl.h"

#include <QDebug>
#include <QEventLoop>
#include <QTimer>

const int ANGLE_DRIFT = 5;
const int ACC_WINDOW = 20;

GY_25T_TTL::GY_25T_TTL(QObject* parent)
    : QSerialPort(parent)
{
    m_aSyncWData.clear();
    m_nlastYaw = INT16_MIN;

    m_iAccIndex = 0;
    m_iAccCount = 0;
    m_aAcc = new int* [3];
    for (int i = 0; i < 3; ++i) {
        m_aAcc[i] = new int[ACC_WINDOW + 1];
        for (int j = 0; j < ACC_WINDOW + 1; ++j)
            m_aAcc[i][j] = 0;
        m_aAcc[i][ACC_WINDOW + 1] = INT16_MAX;
    }
    m_mpCount.clear();

    connect(this, &QSerialPort::readyRead, this, &GY_25T_TTL::onSerialPortReadyRead);
    connect(this, &QSerialPort::bytesWritten, this, &GY_25T_TTL::onSerialPortBytesWritten);
    connect(this, &QSerialPort::errorOccurred, this, &GY_25T_TTL::onGT_25_errorOccurred);
}

GY_25T_TTL::~GY_25T_TTL()
{
    for (int i = 0; i < 3; ++i)
        delete[] m_aAcc[i];
    delete[]m_aAcc;
}

bool GY_25T_TTL::init()
{
    m_aSyncWData.clear();
    m_nlastYaw = INT16_MIN;

    m_iAccIndex = 0;
    m_iAccCount = 0;
    for (int i = 0; i < 3; ++i) {
        m_aAcc[i] = new int[ACC_WINDOW + 1];
        for (int j = 0; j < ACC_WINDOW + 1; ++j)
            m_aAcc[i][j] = 0;
        m_aAcc[i][ACC_WINDOW + 1] = INT16_MAX;
    }
    m_mpCount.clear();

    int ret = 0;
    if (isOpen()) {
        setManu(true);

        // set mode
        QByteArray mode = QByteArray::fromHex("00 06 07 53 60"); //horizental
        ret = writeSync(mode);
        qDebug() << "set mode ret:" << ret;

        //QByteArray rate = QByteArray::fromHex("00 06 02 00 08"); //10hz
        QByteArray rate = QByteArray::fromHex("00 06 02 01 09");   //50hz
        ret = writeSync(rate);
        qDebug() << "set 10hz ret:" << ret;

        // read register setting
        //QByteArray rreg = QByteArray::fromHex("00 03 14 08 1F"); //angle
        QByteArray rreg = QByteArray::fromHex("00 03 08 06 11 ");   //acc
        ret = writeSync(rreg);
        qDebug() << "set reading register ret:" << ret;

        setManu(false);

        // save settings
        QByteArray save = QByteArray::fromHex("00 06 05 55 60");
        ret = writeSync(save);
        qDebug() << "save settings ret:" << ret;
    }
    return true;
}

GY_25T_TTL::Rotate GY_25T_TTL::getRotate()
{
    if (!isOpen())
        return ACC_UNKNOWN;

    int x = m_aAcc[0][ACC_WINDOW + 1];
    int y = m_aAcc[1][ACC_WINDOW + 1];
    int z = m_aAcc[2][ACC_WINDOW + 1];

    QString xyz = QString("%1 %2 %3")
        .arg(x).arg(y).arg(z);
    qDebug().noquote() << "changed direction." << xyz;

    if (y == 1) {
        return ACC_RIGHT;
    }
    if (y == 0) {
        //return ACC_DOWN;
    }
    if (y == -1) {
        return ACC_LEFT;
    }
    if (x == 0) {
        return ACC_UP;
    }
    if (x == 2) {
        return ACC_DOWN;
    }
    return ACC_UNKNOWN;
}

bool GY_25T_TTL::setManu(bool manu)
{
    QByteArray data;
    if (manu) {
        data = QByteArray::fromHex("00 06 03 01 0A");
    }
    else {
        data = QByteArray::fromHex("00 06 03 00 09");
    }
    int ret = writeSync(data);
    qDebug() << "set reading Manu?" << manu << "ret:" << ret;
    return ret == data.size();
}

bool GY_25T_TTL::checksum(QByteArray& buf)
{
    if (buf.isEmpty())
        return false;

    quint64 sum = 0;
    for (int i = 0; i < buf.length() - 1; ++i) {
        sum += (quint8)buf.at(i);
    }

    quint8 rx_s = (quint8)buf.at(buf.length() - 1);
    return !((quint8)sum) ^ rx_s;
}

void GY_25T_TTL::average_acc()
{
    if (m_iAccCount <= 0)
        return;

    for (int i = 0; i < 3; ++i) {
        qint64 s = 0;
        for (int j = 0; j < m_iAccCount; ++j) {
            s += m_aAcc[i][j];
        }
        m_aAcc[i][ACC_WINDOW] = s / m_iAccCount;
        m_aAcc[i][ACC_WINDOW + 1] = qRound(m_aAcc[i][ACC_WINDOW] / 160.0);
    }
}

void GY_25T_TTL::printBuffer(QByteArray& buf)
{
    QString msg = buf.toHex(' ').toUpper();

    QString rec = QString("Rx: %1 (%2)").arg(msg).arg(buf.length());

    qDebug().noquote() << rec;
}

void GY_25T_TTL::handleBuffer(QByteArray& buf)
{
    bool valid = checksum(buf);
    if (!valid) {
        qDebug() << __FUNCTION__ << "checksum failed";
        return;
    }
    if ((quint8)buf[0] != 0xA4) {
        qDebug() << __FUNCTION__ << "frame header ID invalid";
        return;
    }

    if (handleBuffer_WriteFB(buf)) {
        m_aSyncWData.clear();
        emit writeReturned(buf);
    }
    else if (handleBuffer_Angle(buf)) {

    }
    else if (handleBuffer_Acc(buf)) {

    }
}

bool GY_25T_TTL::handleBuffer_WriteFB(QByteArray& buf)
{
    if (m_aSyncWData.isEmpty())
        return false;

    if ((quint8)buf[1] == 0x03 || (quint8)buf[1] == 0x06) {
        if (buf[1] == m_aSyncWData[1] &&
            buf[2] == m_aSyncWData[2] &&
            buf[3] == m_aSyncWData[3]) {
            qDebug() << __FUNCTION__ << "write success -" << m_aSyncWData.toHex();
            return true;
        }
    }
    else if ((quint8)buf[1] == 0x86) {
        qDebug() << __FUNCTION__ << "write failed" << m_aSyncWData.toHex();
        return true;
    }
    else if ((quint8)buf[1] == 0x83) {
        qDebug() << __FUNCTION__ << "read param invalid" << m_aSyncWData.toHex();
        return true;
    }

    return false;
}

bool GY_25T_TTL::handleBuffer_Angle(QByteArray& buf)
{
    /* A4 03 14 08 F5 A0 FA 9A 23 21 0C FB 37
    //   A4: frame header ID
    //   03: func code - read
    //   14: start register
    //   08: registers count
    //   F5 A0: rool(h l)
    //   FA 9A: pitch(h l)
    //   23 21: yaw(h l)
    //   0C FB: temperature(h l)
    //   37: checksum(l)
    //*/

    if ((quint8)buf[2] != 0x14) {
        //qDebug() << __FUNCTION__ << "start register invalid";
        return false;
    }
    if ((quint8)buf[3] != 0x08) {
        //qDebug() << __FUNCTION__ << "registers count invalid";
        return false;
    }

    //quint16 up = (quint8)buf[6] << 8 | (quint8)buf[7];
    quint16 uy = (quint8)buf[8] << 8 | (quint8)buf[9];
    quint16 ut = (quint8)buf[10] << 8 | (quint8)buf[11];

    //qint16 fp = ((qint16)up/100);
    qint16 iy = ((qint16)uy / 100) + 180;
    qreal ft = ((qint16)ut) / 100.0;
    Q_UNUSED(ft);

    //qDebug() << __FUNCTION__ << "pitch:" << fp;
    //qDebug() << __FUNCTION__ << "yaw:" << iy;
    //qDebug() << __FUNCTION__ << "temp:" << ft;

    if (m_nlastYaw == INT16_MIN)
        m_nlastYaw = iy;

    int diff = m_nlastYaw - iy;

    int absdiff = qAbs(diff);
    if (absdiff < ANGLE_DRIFT) {
        return true;
    }
    //qDebug() << "absolute difference: " << absdiff;

    if (absdiff > 135) {
        m_nlastYaw += 180;
        m_nlastYaw %= 360;
        qDebug() << "yaw:" << iy << "rotate 180";
    }
    if (diff < -45 && diff > -135) {
        m_nlastYaw += 90;
        m_nlastYaw %= 360;
        qDebug() << "yaw:" << iy << "clockwise 90";
    }
    else if (diff > 45 && diff < 135) {
        m_nlastYaw += 270;
        m_nlastYaw %= 360;

        qDebug() << "yaw:" << iy << "anti-clockwise 90";
    }
    else {
        //qDebug() << "yaw:"  << iy;
    }
    return true;
}

bool GY_25T_TTL::handleBuffer_Acc(QByteArray& buf)
{
    /* A4 03 08 06 FE E0 05 EF 4C 73 46
    //   A4: frame header ID
    //   03: func code - read
    //   08: start register
    //   06: registers count
    //   FE E0: acc_x(h l)
    //   05 EF: acc_y(h l)
    //   4C 73: acc_z(h l)
    //   46: checksum(l)
    //*/

    if ((quint8)buf[2] != 0x08) {
        //qDebug() << __FUNCTION__ << "start register invalid";
        return false;
    }
    if ((quint8)buf[3] != 0x06) {
        //qDebug() << __FUNCTION__ << "registers count invalid";
        return false;
    }

    emit ready(true);

    quint16 ux = (quint8)buf[4] << 8 | (quint8)buf[5];
    quint16 uy = (quint8)buf[6] << 8 | (quint8)buf[7];
    quint16 uz = (quint8)buf[8] << 8 | (quint8)buf[9];

    qint16 ix = ((qint16)ux / 100);
    qint16 iy = ((qint16)uy / 100);
    qint16 iz = ((qint16)uz / 100);

    //qDebug() << __FUNCTION__ << "acc_x:" << ix;
    //qDebug() << __FUNCTION__ << "acc_x:" << iy;
    //qDebug() << __FUNCTION__ << "acc_x:" << iz;

    if (m_iAccCount < ACC_WINDOW) {
        m_iAccCount++;
    }

    m_iAccIndex = (m_iAccIndex + 1) % ACC_WINDOW;
    m_aAcc[0][m_iAccIndex] = ix;
    m_aAcc[1][m_iAccIndex] = iy;
    m_aAcc[2][m_iAccIndex] = iz;

    average_acc();

    //qDebug() << __FUNCTION__ << "avg_acc_x:"
    //         << m_aAcc[0][ACC_WINDOW] << m_aAcc[0][ACC_WINDOW + 1];
    //qDebug() << __FUNCTION__ << "avg_acc_x:"
    //         << m_aAcc[1][ACC_WINDOW] << m_aAcc[1][ACC_WINDOW + 1];
    //qDebug() << __FUNCTION__ << "avg_acc_x:"
    //         << m_aAcc[2][ACC_WINDOW] << m_aAcc[2][ACC_WINDOW + 1];

    QString key = QString("%1-%2-%3")
        .arg(m_aAcc[0][ACC_WINDOW + 1])
        .arg(m_aAcc[1][ACC_WINDOW + 1])
        .arg(m_aAcc[2][ACC_WINDOW + 1]);
    auto it = m_mpCount.find(key);
    if (it == m_mpCount.end()) {
        m_mpCount.clear();
        m_mpCount[key] = 1;
    }
    else {
        if (it.value() == 20) {
            Rotate r = getRotate();
            if (r != ACC_UNKNOWN) {
                emit rotated(r);
            }
        }
        if (it.value() < 100) {
            m_mpCount[key] = it.value() + 1;
        }
    }

    return true;
}

int GY_25T_TTL::writeSync(QByteArray& buf)
{
    if (buf.isEmpty())
        return 0;
    if (!m_aSyncWData.isEmpty()) {
        qDebug() << "writing in progress";
        return 0;
    }

    m_aSyncWData = buf;
    int count = write(buf);

    QEventLoop loop;
    QTimer timer;
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    connect(this, &GY_25T_TTL::writeReturned, &loop, &QEventLoop::quit);
    timer.start(3000);
    loop.exec();

    if (!timer.isActive()) {
        qDebug() << "sync write wating feedback timeout";
        count = 0;
    }
    m_aSyncWData.clear();

    return count;
}

void GY_25T_TTL::onSerialPortReadyRead()
{
    QByteArray buf;
    for (;;) {
        QByteArray tmp;
        tmp = read(100);
        if (!tmp.isEmpty())
            buf.append(tmp);

        if (tmp.isEmpty() || this->atEnd())
            break;
    }

    //printBuffer(buf);
    handleBuffer(buf);
}

void GY_25T_TTL::onSerialPortBytesWritten(qint64 bytes)
{
    qDebug() << __FUNCTION__ << "write:" << bytes;
}

void GY_25T_TTL::onGT_25_errorOccurred(QSerialPort::SerialPortError error)
{
    qDebug() << __FUNCTION__ << error;
    if (error == QSerialPort::PermissionError ||
        error == QSerialPort::ResourceError) {
        emit ready(false);
    }
}
