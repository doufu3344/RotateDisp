#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QAbstractButton>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QTimerEvent>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QSettings>

#include "rotatedisp.h"
#include "gy_25t_ttl.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

protected:
    virtual void keyPressEvent(QKeyEvent* event);
    virtual void keyReleaseEvent(QKeyEvent* event);
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void mouseReleaseEvent(QMouseEvent* event);
    virtual void mouseMoveEvent(QMouseEvent* event);
    virtual void closeEvent(QCloseEvent* event);
    virtual void timerEvent(QTimerEvent* event);

private slots:
    void on_checkBox_Boot_stateChanged(int arg1);

    void on_pushButton_Hide_clicked();

    void iconActivated(QSystemTrayIcon::ActivationReason reason);
    void on_comboBox_Port_currentIndexChanged(int index);
    void on_pushButton_Refresh_clicked();
    void on_pushButton_Open_clicked();

    void on_comboBox_Monitor_currentIndexChanged(int index);
    void on_checkBox_Auto_stateChanged(int state);
    void on_checkBox_Reverse_stateChanged(int arg1);
    void on_buttonGroup_Manu_buttonClicked(QAbstractButton* button);

    void onGT_25_rotated(GY_25T_TTL::Rotate r);
    void onGT_25_ready(bool ready);

private:
    void setAutoBoot();
    void createTrayIcon();

    void saveSerialInfo();
    bool autoConnect();

    void initDevSettings();
    void initMonitor();
    void enableSettings(bool enable);

    void rotate(RotateDisp::ROTATE r);
    void rebuildRotateMap();

    void retranslate();

    Ui::MainWindow* ui;

    QSettings m_settings;

    QAction* quitAction;
    QAction* showAction;
    QSystemTrayIcon* trayIcon;
    QMenu* trayIconMenu;

    bool m_bCtrlKeyPressed;

    GY_25T_TTL m_gy25t;
    bool m_bFirstR;
    QMap<GY_25T_TTL::Rotate, RotateDisp::ROTATE> m_mpRotate;

    bool m_bMousePressed;
    int m_nMouseClick_X_Coordinate;
    int m_nMouseClick_Y_Coordinate;

    int m_nTrTimer;
};
#endif // MAINWINDOW_H
