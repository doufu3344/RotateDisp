#ifndef GY_25T_TTL_H
#define GY_25T_TTL_H

#include <QSerialPort>
#include <QSerialPortInfo>
#include <QByteArray>
#include <QMap>

class GY_25T_TTL : public QSerialPort
{
    Q_OBJECT

public:
    enum Rotate {
        ACC_UP,
        ACC_RIGHT,
        ACC_DOWN,
        ACC_LEFT,
        ACC_UNKNOWN
    };

    GY_25T_TTL(QObject* parent = nullptr);
    virtual ~GY_25T_TTL();

    bool init();

    GY_25T_TTL::Rotate getRotate();

    bool setManu(bool manu);

private:
    bool checksum(QByteArray& buf);
    void average_acc();
    void printBuffer(QByteArray& buf);
    void handleBuffer(QByteArray& buf);
    bool handleBuffer_WriteFB(QByteArray& buf);
    bool handleBuffer_Angle(QByteArray& buf);
    bool handleBuffer_Acc(QByteArray& buf);

    int writeSync(QByteArray& buf);

    QByteArray m_aSyncWData;

    qint16 m_nlastYaw;

    int m_iAccIndex;
    int m_iAccCount;
    int** m_aAcc;
    QMap<QString, quint32> m_mpCount;

private slots:
    void onSerialPortReadyRead();
    void onSerialPortBytesWritten(qint64 bytes);
    void onGT_25_errorOccurred(QSerialPort::SerialPortError error);

signals:
    void ready(bool ready);

    void writeReturned(QByteArray buf);
    void rotated(GY_25T_TTL::Rotate r);
};

#endif // GY_25T_TTL_H
