#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QStringList>
#include <QDesktopWidget>
#include <QMessageBox>
#include <QSettings>
#include <QDir>
#include <QFileInfo>
#include <QTranslator>
#include <QLocale>

#include "rotatedisp.h"

const char* SERIAL_DEV_NAME = "USB-SERIAL CH340";

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_settings("doufu", "RotateDisp")

{
    ui->setupUi(this);

    ui->checkBox_Boot->setChecked(m_settings.value("AutoBoot", false).toBool());

    trayIconMenu = nullptr;
    trayIcon = nullptr;
    createTrayIcon();

    retranslate();

    m_bCtrlKeyPressed = false;
    m_bMousePressed = false;
    m_bFirstR = true;

    setWindowFlags(windowFlags() | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setCursor(QCursor(Qt::OpenHandCursor));

    connect(&m_gy25t, &GY_25T_TTL::rotated, this, &MainWindow::onGT_25_rotated);
    connect(&m_gy25t, &GY_25T_TTL::ready, this, &MainWindow::onGT_25_ready);

    initDevSettings();
    initMonitor();

    bool conn = autoConnect();
    if (conn) {
        hide();
    }
    else {
        qDebug() << __FUNCTION__ << "autoConnect failed";
        show();
    }

    m_nTrTimer = startTimer(1000);
}

MainWindow::~MainWindow()
{
    if (m_gy25t.isOpen())
        m_gy25t.close();

    delete ui;
}

void MainWindow::keyPressEvent(QKeyEvent* event)
{
    qDebug() << __FUNCTION__ << event->key();
    auto key = event->key();
    if (key == Qt::Key_F5) {
        if (ui->pushButton_Refresh->isEnabled()) {
            initDevSettings();
            initMonitor();
        }
    }
    else if (key == Qt::Key_Control) {
        m_bCtrlKeyPressed = true;
        qDebug() << "contrl key pressed.";
    }
    else if (key == Qt::Key_Escape) {
        on_pushButton_Hide_clicked();
    }
    QMainWindow::keyPressEvent(event);
}

void MainWindow::keyReleaseEvent(QKeyEvent* event)
{
    qDebug() << __FUNCTION__ << event->key();
    auto key = event->key();
    if (key == Qt::Key_Control) {
        m_bCtrlKeyPressed = false;
    }
    else if (m_bCtrlKeyPressed) {
        qDebug() << __FUNCTION__ << "combination" << event->key();

        RotateDisp::ROTATE r = RotateDisp::ROTATE_DEFAULT;
        switch (key) {
        case Qt::Key_Up:
            r = RotateDisp::ROTATE_180;
            break;
        case Qt::Key_Down:
            r = RotateDisp::ROTATE_DEFAULT;
            break;
        case Qt::Key_Left:
            r = RotateDisp::ROTATE_270;
            break;
        case Qt::Key_Right:
            r = RotateDisp::ROTATE_90;
            break;
        }
        disconnect(&m_gy25t, &GY_25T_TTL::rotated, this, &MainWindow::onGT_25_rotated);
        rotate(r);
        m_bFirstR = true;
        connect(&m_gy25t, &GY_25T_TTL::rotated, this, &MainWindow::onGT_25_rotated);

    }

    QMainWindow::keyReleaseEvent(event);
}

void MainWindow::mousePressEvent(QMouseEvent* event)
{
    m_bMousePressed = true;
    setCursor(QCursor(Qt::ClosedHandCursor));

    m_nMouseClick_X_Coordinate = event->x();
    m_nMouseClick_Y_Coordinate = event->y();
    QMainWindow::mousePressEvent(event);
}

void MainWindow::mouseReleaseEvent(QMouseEvent* event)
{
    m_bMousePressed = false;
    setCursor(QCursor(Qt::OpenHandCursor));
    QMainWindow::mouseReleaseEvent(event);
}

void MainWindow::mouseMoveEvent(QMouseEvent* event)
{
    if (m_bMousePressed)
        move(event->globalX() - m_nMouseClick_X_Coordinate, event->globalY() - m_nMouseClick_Y_Coordinate);
    QMainWindow::mouseMoveEvent(event);
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (trayIcon->isVisible()) {
        if (!m_settings.value("traytips", false).toBool()) {
            QMessageBox::information(this,
                QApplication::translate("MainWindow", "Systray"),
                QApplication::translate("MainWindow", "The program will keep running in the "
                    "system tray. To terminate the program, "
                    "choose <b>Quit</b> in the context menu "
                    "of the system tray entry."));
            m_settings.setValue("traytips", true);
        }
        hide();
        event->ignore();
    }
}

void MainWindow::timerEvent(QTimerEvent* event)
{
    if (event->timerId() == m_nTrTimer) {
        retranslate();
    }
}

void MainWindow::on_checkBox_Boot_stateChanged(int arg1)
{
    m_settings.setValue("AutoBoot", arg1 == Qt::Checked);

    QSettings reg(
        "HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",
        QSettings::NativeFormat);
    QString strAppPath = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
    QString strAppName = QFileInfo(strAppPath).baseName();
    if (arg1 == Qt::Checked)
        reg.setValue(strAppName, strAppPath);
    else
        reg.remove(strAppName);
}

void MainWindow::on_pushButton_Hide_clicked()
{
    close();
}

void MainWindow::iconActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason) {
    case QSystemTrayIcon::Trigger:
    case QSystemTrayIcon::DoubleClick:
        show();
        //iconComboBox->setCurrentIndex((iconComboBox->currentIndex() + 1) % iconComboBox->count());
        break;
    case QSystemTrayIcon::MiddleClick:
        break;
    default:
        ;
    }
}

void MainWindow::on_comboBox_Port_currentIndexChanged(int index)
{
    qDebug() << __FUNCTION__ << ui->comboBox_Port->itemText(index);
}

void MainWindow::on_pushButton_Refresh_clicked()
{
    initDevSettings();
}

void MainWindow::on_pushButton_Open_clicked()
{
    qDebug() << __FUNCTION__;

    m_bFirstR = true;
    if (m_gy25t.isOpen() && ui->pushButton_Open->text() !=
        QApplication::translate("MainWindow", "Close")) {
        qDebug() << m_gy25t.portName() << "is already opened";
        return;
    }
    else if (ui->pushButton_Open->text() ==
        QApplication::translate("MainWindow", "Close")) {
        m_gy25t.close();

        onGT_25_ready(false);

        return;
    }
    if (ui->comboBox_Port->currentText().isEmpty()) {
        QMessageBox::critical(this,
            QApplication::translate("MainWindow", "Error"),
            QApplication::translate("MainWindow", "No device connected!"));
        return;
    }

    m_gy25t.setPortName(ui->comboBox_Port->currentText());
    m_gy25t.setBaudRate(ui->comboBox_Baud->currentData().toInt());
    m_gy25t.setDataBits((QSerialPort::DataBits)ui->comboBox_DataBits->currentData().toInt());
    m_gy25t.setParity((QSerialPort::Parity)ui->comboBox_Parity->currentData().toInt());
    m_gy25t.setStopBits((QSerialPort::StopBits)ui->comboBox_StopBits->currentData().toInt());
    if (m_gy25t.open(QIODevice::ReadWrite)) {
        // replace icon when opening
        ui->pushButton_Open->setText(QApplication::translate("MainWindow", "Opening..."));
        //ui->pushButton_Open->setIcon(QIcon("://pic/logo.png"));
        trayIcon->setIcon(QIcon("://pic/logo.png"));

        m_gy25t.init();

        setAutoBoot();
    }
    else {
        QMessageBox::critical(this,
            QApplication::translate("MainWindow", "Error"),
            QApplication::translate("MainWindow", "Open device failed!")
            + QString("(%1)").arg(ui->comboBox_Port->currentText()));
    }
}

void MainWindow::on_comboBox_Monitor_currentIndexChanged(int index)
{
    qDebug() << __FUNCTION__ << index;
    RotateDisp rd;
    QString monitor = rd.getMonitorName(index);
    m_settings.setValue("monitor", monitor);

    RotateDisp rd1(monitor);
    RotateDisp::ROTATE r = rd1.getRotate(monitor);
    setMonitorState(r);

    //rebuildRotateMap();
}

void MainWindow::on_checkBox_Auto_stateChanged(int state)
{
    qDebug() << __FUNCTION__ << state;
    if (state == Qt::CheckState::Checked) {
        rebuildRotateMap();
    }
}

void MainWindow::on_checkBox_Reverse_stateChanged(int state)
{
    Q_UNUSED(state);
    rebuildRotateMap();
}

void MainWindow::on_buttonGroup_Manu_buttonClicked(QAbstractButton* button)
{
    //qDebug() << __FUNCTION__ << button->objectName();
    ui->checkBox_Auto->setChecked(false);

    RotateDisp::ROTATE r;
    if (button == ui->toolButton_Up)
        r = RotateDisp::ROTATE_180;
    else if (button == ui->toolButton_Down)
        r = RotateDisp::ROTATE_DEFAULT;
    else if (button == ui->toolButton_Left)
        r = RotateDisp::ROTATE_270;
    else if (button == ui->toolButton_Right)
        r = RotateDisp::ROTATE_90;
    else
        return;

    disconnect(&m_gy25t, &GY_25T_TTL::rotated, this, &MainWindow::onGT_25_rotated);
    rotate(r);
    m_bFirstR = true;
    connect(&m_gy25t, &GY_25T_TTL::rotated, this, &MainWindow::onGT_25_rotated);
}

void MainWindow::onGT_25_rotated(GY_25T_TTL::Rotate r)
{
    ui->label_GY25TState->setText(QString("(%1)").arg(r));

    if (!ui->checkBox_Auto->isChecked())
        return;

    if (m_bFirstR) {
        rebuildRotateMap();

        m_bFirstR = false;
        return;
    }

    if (!m_mpRotate.contains(r))
        return;

    RotateDisp::ROTATE rr = m_mpRotate[r];
    //qDebug() << __FUNCTION__ << "rotate disp:" << rr;
    rotate(rr);
}

void MainWindow::onGT_25_ready(bool ready)
{
    if (ready) {
        ui->pushButton_Open->setText(QApplication::translate("MainWindow", "Close"));
        ui->pushButton_Open->setIcon(QIcon("://pic/logo-green.png"));
        trayIcon->setIcon(QIcon("://pic/logo-green.png"));

        saveSerialInfo();
    }
    else {
        qDebug() << __FUNCTION__ << "false";
        ui->label_GY25TState->clear();
        ui->pushButton_Open->setText(QApplication::translate("MainWindow", "Open"));
        ui->pushButton_Open->setIcon(QIcon("://pic/logo-red.png"));
        trayIcon->setIcon(QIcon("://pic/logo-red.png"));
        m_bFirstR = true;

        show();
    }

    enableSettings(!ready);
}

void MainWindow::setAutoBoot()
{
    if (!ui->checkBox_Boot->isChecked()) {
        int ret = QMessageBox::question(this,
            QApplication::translate("MainWindow", "Startup"),
            QApplication::translate("MainWindow", "Do you want this program to automatically start?"));
        if (ret == QMessageBox::Yes) {
            ui->checkBox_Boot->setChecked(true);
            hide();
        }
    }
    else {
        on_checkBox_Boot_stateChanged(Qt::Checked);
    }
}

void MainWindow::createTrayIcon()
{
    if (trayIconMenu == nullptr)
        trayIconMenu = new QMenu(this);
    else
        trayIconMenu->clear();

    trayIconMenu->addAction(QIcon("://pic/monitor-d.png"),
        QApplication::translate("MainWindow", "Show"),
        this, &QMainWindow::show);
    trayIconMenu->addAction(QIcon("://pic/end.png"),
        QApplication::translate("MainWindow", "Quit"),
        qApp, &QCoreApplication::quit);

    if (trayIcon == nullptr) {
        trayIcon = new QSystemTrayIcon(this);
        connect(trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::iconActivated);

        //trayIcon->setIcon(QIcon("://pic/logo.png"));
        trayIcon->setIcon(QIcon("://pic/logo-red.png"));

        trayIcon->setContextMenu(trayIconMenu);

        trayIcon->setToolTip(qApp->applicationName());
        trayIcon->setVisible(true);
    }
}

void MainWindow::saveSerialInfo()
{
    QString port = ui->comboBox_Port->currentText();
    m_settings.setValue("port", port);

    auto baud = (QSerialPort::BaudRate)ui->comboBox_Baud->currentData().toInt();
    m_settings.setValue("baud", baud);

    auto data = (QSerialPort::DataBits)ui->comboBox_DataBits->currentData().toInt();
    m_settings.setValue("databits", data);

    auto parity = (QSerialPort::Parity)ui->comboBox_Parity->currentData().toInt();
    m_settings.setValue("parity", parity);

    auto stop = (QSerialPort::StopBits)ui->comboBox_StopBits->currentData().toInt();
    m_settings.setValue("stopbits", stop);
}

bool MainWindow::autoConnect()
{
    QString port = m_settings.value("port", "").toString();
    if (port.isNull()) {
        return false;
    }
    QSerialPortInfo info(port);
    if (info.isNull()) {
        return false;
    }
    if (info.description() != SERIAL_DEV_NAME) {
        return false;
    }

    bool ok = false;
    auto baud = (QSerialPort::BaudRate)m_settings.value("baud", -1).toInt(&ok);
    if (!ok || ui->comboBox_Baud->findData(baud) < 0) {
        return false;
    }
    auto data = (QSerialPort::DataBits)m_settings.value("databits", -1).toInt(&ok);
    if (!ok || ui->comboBox_DataBits->findData(data) < 0) {
        return false;
    }
    auto parity = (QSerialPort::Parity)m_settings.value("parity", -1).toInt(&ok);
    if (!ok || ui->comboBox_Parity->findData(parity) < 0) {
        return false;
    }
    auto stop = (QSerialPort::StopBits)m_settings.value("stopbits", -1).toInt(&ok);
    if (!ok || ui->comboBox_StopBits->findData(stop) < 0) {
        return false;
    }

    m_gy25t.setPortName(port);
    m_gy25t.setBaudRate(baud);
    m_gy25t.setDataBits(data);
    m_gy25t.setParity(parity);
    m_gy25t.setStopBits(stop);

    if (m_gy25t.open(QIODevice::ReadWrite)) {
        ui->pushButton_Open->setIcon(QIcon("://pic/logo.png"));
        trayIcon->setIcon(QIcon("://pic/logo.png"));
        m_gy25t.init();

        return ui->checkBox_Boot->isChecked();
    }
    else {
        return false;
    }
}

void MainWindow::initDevSettings()
{
    QSerialPortInfo inf;
    auto lPort = inf.availablePorts();
    ui->comboBox_Port->clear();
    for (auto& p : lPort) {
        if (p.description() != SERIAL_DEV_NAME)
            continue;

        ui->comboBox_Port->addItem(p.portName(), p.serialNumber());
    }

    // fill baud rate
    ui->comboBox_Baud->clear();
    ui->comboBox_Baud->addItem(QString::number(QSerialPort::Baud1200), QSerialPort::Baud1200);
    ui->comboBox_Baud->addItem(QString::number(QSerialPort::Baud2400), QSerialPort::Baud2400);
    ui->comboBox_Baud->addItem(QString::number(QSerialPort::Baud4800), QSerialPort::Baud4800);
    ui->comboBox_Baud->addItem(QString::number(QSerialPort::Baud9600), QSerialPort::Baud9600);
    ui->comboBox_Baud->addItem(QString::number(QSerialPort::Baud19200), QSerialPort::Baud19200);
    ui->comboBox_Baud->addItem(QString::number(QSerialPort::Baud38400), QSerialPort::Baud38400);
    ui->comboBox_Baud->addItem(QString::number(QSerialPort::Baud57600), QSerialPort::Baud57600);
    ui->comboBox_Baud->addItem(QString::number(QSerialPort::Baud115200), QSerialPort::Baud115200);
    ui->comboBox_Baud->setCurrentText(QString::number(QSerialPort::Baud9600));

    // fill data bits
    ui->comboBox_DataBits->clear();
    ui->comboBox_DataBits->addItem(QString::number(QSerialPort::Data5), QSerialPort::Data5);
    ui->comboBox_DataBits->addItem(QString::number(QSerialPort::Data6), QSerialPort::Data6);
    ui->comboBox_DataBits->addItem(QString::number(QSerialPort::Data7), QSerialPort::Data7);
    ui->comboBox_DataBits->addItem(QString::number(QSerialPort::Data8), QSerialPort::Data8);
    ui->comboBox_DataBits->setCurrentText(QString::number(QSerialPort::Data8));

    // fill parity
    ui->comboBox_Parity->clear();
    ui->comboBox_Parity->addItem(QApplication::translate("MainWindow", "None"), QSerialPort::NoParity);
    ui->comboBox_Parity->addItem(QApplication::translate("MainWindow", "Even"), QSerialPort::EvenParity);
    ui->comboBox_Parity->addItem(QApplication::translate("MainWindow", "Odd"), QSerialPort::OddParity);
    ui->comboBox_Parity->addItem(QApplication::translate("MainWindow", "Space"), QSerialPort::SpaceParity);
    ui->comboBox_Parity->addItem(QApplication::translate("MainWindow", "Mark"), QSerialPort::MarkParity);
    ui->comboBox_Parity->setCurrentText(QApplication::translate("MainWindow", "None"));

    // fill stop bits
    ui->comboBox_StopBits->clear();
    ui->comboBox_StopBits->addItem(QApplication::translate("MainWindow", "One"), QSerialPort::OneStop);
    ui->comboBox_StopBits->addItem(QApplication::translate("MainWindow", "One And Half"), QSerialPort::OneAndHalfStop);
    ui->comboBox_StopBits->addItem(QApplication::translate("MainWindow", "Two"), QSerialPort::TwoStop);
    ui->comboBox_StopBits->setCurrentText(QApplication::translate("MainWindow", "One"));
}

void MainWindow::initMonitor()
{
    RotateDisp rd;
    int count = rd.getMonitorCount();

    // fill monitor
    ui->comboBox_Monitor->clear();
    for (int i = 0; i < count; ++i) {
        //ui->comboBox_Monitor->addItem(QString::number(i + 1), i);

        QString name = rd.getMonitorName(i);
        ui->comboBox_Monitor->addItem(name, i);
    }
    on_comboBox_Monitor_currentIndexChanged(0);
}

void MainWindow::enableSettings(bool enable)
{
    ui->comboBox_Port->setEnabled(enable);
    ui->comboBox_Baud->setEnabled(enable);
    ui->comboBox_DataBits->setEnabled(enable);
    ui->comboBox_Parity->setEnabled(enable);
    ui->comboBox_StopBits->setEnabled(enable);
    ui->pushButton_Refresh->setEnabled(enable);

    ui->comboBox_Monitor->setEnabled(enable);
}

void MainWindow::rotate(RotateDisp::ROTATE r)
{
    if (ui->comboBox_Monitor->count() <= 0) {
        qDebug() << "None monitor";
        return;
    }

    QString monitor = m_settings.value("monitor").toString();
    RotateDisp rd(monitor);
    bool ret = rd.rotate(monitor, r);
    if (!ret) {
        qDebug() << rd.errmsg();
    }
    else {
        setMonitorState(r);
    }
}

void MainWindow::rebuildRotateMap()
{
    GY_25T_TTL::Rotate r1 = m_gy25t.getRotate();
    if (r1 == GY_25T_TTL::ACC_UNKNOWN)
        return;

    QString monitor = m_settings.value("monitor").toString();
    RotateDisp rd(monitor);
    RotateDisp::ROTATE r2 = rd.getRotate(monitor);
    setMonitorState(r2);

    QVector<GY_25T_TTL::Rotate> ar1;
    ar1 << GY_25T_TTL::Rotate::ACC_UP
        << GY_25T_TTL::Rotate::ACC_RIGHT
        << GY_25T_TTL::Rotate::ACC_DOWN
        << GY_25T_TTL::Rotate::ACC_LEFT;

    QVector<RotateDisp::ROTATE> ar2;
    ar2 << RotateDisp::ROTATE::ROTATE_DEFAULT
        << RotateDisp::ROTATE::ROTATE_90
        << RotateDisp::ROTATE::ROTATE_180
        << RotateDisp::ROTATE::ROTATE_270;

    m_mpRotate.clear();
    m_mpRotate[r1] = r2;

    int i1 = ar1.indexOf(r1);
    int i2 = ar2.indexOf(r2);

    for (int i = 0; i < 4; ++i) {
        int ii1 = (i1 + i) % 4;
        int ii2 = 0;
        if (ui->checkBox_Reverse->isChecked())
            ii2 = (i2 - i + 4) % 4;
        else
            ii2 = (i2 + i) % 4;
        m_mpRotate[ar1[ii1]] = ar2[ii2];
    }
    qDebug() << __FUNCTION__ << "rotate map" << m_mpRotate;
}

void MainWindow::setMonitorState(RotateDisp::ROTATE r)
{
    ui->toolButton_Up->setStyleSheet("border: 2px solid #000000;");
    ui->toolButton_Down->setStyleSheet("border: 2px solid #000000;");
    ui->toolButton_Left->setStyleSheet("border: 2px solid #000000;");
    ui->toolButton_Right->setStyleSheet("border: 2px solid #000000;");
    switch (r)
    {
    case RotateDisp::ROTATE_180:
        ui->toolButton_Up->setStyleSheet("border: 2px solid #00ff00;");
        break;
    case RotateDisp::ROTATE_DEFAULT:
        ui->toolButton_Down->setStyleSheet("border: 2px solid #00ff00;");
        break;
    case RotateDisp::ROTATE_270:
        ui->toolButton_Left->setStyleSheet("border: 2px solid #00ff00;");
        break;
    case RotateDisp::ROTATE_90:
        ui->toolButton_Right->setStyleSheet("border: 2px solid #00ff00;");
        break;
    }
}

void MainWindow::retranslate()
{
    static auto oldLang = QLocale::English;
    static QList<QTranslator*> trList;
    QLocale loc = QLocale::system();
    auto lang = loc.language();
    if (lang == oldLang) {
        return;
    }

    QFileInfoList newTrFileInfo;
    if (lang != QLocale::English) {
        QStringList nameFilter;
        QString strTransPath = QDir::toNativeSeparators(
            QCoreApplication::applicationDirPath() + "/translations");
        QDir dir = QDir(strTransPath);
        if (loc.language() == QLocale::Chinese) {
            nameFilter << "*_zh_CN.qm";
        }
        newTrFileInfo = dir.entryInfoList(nameFilter);
    }
    //qDebug() << lang << newTrFileInfo;
    if (newTrFileInfo.isEmpty()) {
        return;
    }

    oldLang = lang;
    bool reTr = false;
    for (auto& tr : trList) {
        qApp->removeTranslator(tr);
        reTr = true;
    }
    qDeleteAll(trList);
    trList.clear();
    for (auto& fi : newTrFileInfo) {
        QTranslator* tr = new QTranslator();
        auto fn = fi.absoluteFilePath();
        if (tr->load(loc, fn)) {
            qApp->installTranslator(tr);
            trList << tr;
            reTr = true;
        }
    }

    if (reTr)
        ui->retranslateUi(this);

    ui->pushButton_Open->setText(QApplication::translate("MainWindow", "Close"));
    ui->pushButton_Open->setText(QApplication::translate("MainWindow", "Open"));
    initDevSettings();
    createTrayIcon();
}
