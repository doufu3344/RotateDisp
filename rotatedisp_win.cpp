#include "rotatedisp.h"

#include <windows.h>
#include <iostream>

#include <QSize>
#include <QApplication>

#pragma comment (lib, "User32.lib")

using namespace std;

static bool getDefSize(wchar_t* name, QSize& size)
{
    DEVMODE devMode;

    QString dev;

    ZeroMemory(&devMode, sizeof(DEVMODE));
    devMode.dmSize = sizeof(devMode);
    int ret = EnumDisplaySettingsEx(name, ENUM_CURRENT_SETTINGS, &devMode, NULL);
    if (ret == 0) {
        size.setWidth(0);
        size.setHeight(0);
        return false;
    }

    if (devMode.dmDisplayOrientation == DMDO_DEFAULT) {
        size.setWidth(devMode.dmPelsWidth);
        size.setHeight(devMode.dmPelsHeight);
    }
    if (devMode.dmDisplayOrientation == DMDO_90) {
        size.setHeight(devMode.dmPelsWidth);
        size.setWidth(devMode.dmPelsHeight);
    }
    if (devMode.dmDisplayOrientation == DMDO_180) {
        size.setWidth(devMode.dmPelsWidth);
        size.setHeight(devMode.dmPelsHeight);
    }
    if (devMode.dmDisplayOrientation == DMDO_270) {
        size.setHeight(devMode.dmPelsWidth);
        size.setWidth(devMode.dmPelsHeight);
    }
    return true;
}

static void ShowDevMode(DEVMODE& devMode)
{
    cout << "DEVMODE Settings:" << endl;
    cout << " dmPelsWidth: " << devMode.dmPelsWidth << endl;
    cout << " dmPelsHeight: " << devMode.dmPelsHeight << endl;
    cout << " dmBitsPerPel: " << devMode.dmBitsPerPel << endl;
    cout << " dmDisplayFrequency: " << devMode.dmDisplayFrequency << endl;
    cout << " dmDisplayOrientation: " << devMode.dmDisplayOrientation << endl;
}

static QString errinf(long code)
{
    switch (code)
    {
    case DISP_CHANGE_SUCCESSFUL:
        return "ChangeDisplaySettingsEx returns DISP_CHANGE_SUCCESSFUL";
        break;
    case DISP_CHANGE_BADDUALVIEW:
        return "ChangeDisplaySettingsEx returns DISP_CHANGE_BADDUALVIEW";
        break;
    case DISP_CHANGE_BADFLAGS:
        return "ChangeDisplaySettingsEx returns DISP_CHANGE_BADFLAGS";
        break;
    case DISP_CHANGE_BADMODE:
        return "ChangeDisplaySettingsEx returns DISP_CHANGE_BADMODE";
        break;
    case DISP_CHANGE_BADPARAM:
        return "ChangeDisplaySettingsEx returns DISP_CHANGE_BADPARAM";
        break;
    case DISP_CHANGE_FAILED:
        return "ChangeDisplaySettingsEx returns DISP_CHANGE_FAILED";
        break;
    case DISP_CHANGE_NOTUPDATED:
        return "ChangeDisplaySettingsEx returns DISP_CHANGE_NOTUPDATED";
        break;
    case DISP_CHANGE_RESTART:
        return "ChangeDisplaySettingsEx returns DISP_CHANGE_RESTART";
        break;
    default:
        return "ChangeDisplaySettingsEx - Unexpected return value.";
    }
}

RotateDisp::RotateDisp()
{
    getDefSize(NULL, m_defSzie);
}

QString RotateDisp::errmsg()
{
    return m_sErrinf;
}

bool RotateDisp::rotate(RotateDisp::ROTATE r)
{
    return rotate("", r);
}

bool RotateDisp::rotate(QString name, RotateDisp::ROTATE r)
{
    if (m_defSzie.isEmpty()) {
        m_sErrinf = QString(__FUNCTION__) + ": get monitor's default size failed.";
        return false;
    }

    wchar_t dev[256] = { 0 };
    wchar_t* pDev = dev;
    if (!name.isEmpty())
        name.toWCharArray(dev);
    else
        pDev = NULL;

    DEVMODE devMode;
    long ret;

    // Init DEVMODE to current settings
    ZeroMemory(&devMode, sizeof(DEVMODE));
    devMode.dmSize = sizeof(devMode);
    EnumDisplaySettingsEx(pDev, ENUM_CURRENT_SETTINGS, &devMode, NULL);

    ShowDevMode(devMode);

    auto oldOri = devMode.dmDisplayOrientation;

    switch (r)
    {
    case ROTATE_DEFAULT:
        devMode.dmDisplayOrientation = DMDO_DEFAULT;
        devMode.dmPelsWidth = m_defSzie.width();
        devMode.dmPelsHeight = m_defSzie.height();
        break;
    case ROTATE_90:
        /* Rotate Orientation - 90 */
        devMode.dmDisplayOrientation = DMDO_90;
        swap(devMode.dmPelsHeight, devMode.dmPelsWidth);
        devMode.dmPelsWidth = m_defSzie.height();
        devMode.dmPelsHeight = m_defSzie.width();
        break;
    case ROTATE_180:
        /* Rotate Orientation - 180 */
        devMode.dmDisplayOrientation = DMDO_180;
        devMode.dmPelsWidth = m_defSzie.width();
        devMode.dmPelsHeight = m_defSzie.height();
        break;
    case ROTATE_270:
        /* Rotate Orientation - 270 */
        devMode.dmDisplayOrientation = DMDO_270;
        swap(devMode.dmPelsHeight, devMode.dmPelsWidth);
        devMode.dmPelsWidth = m_defSzie.height();
        devMode.dmPelsHeight = m_defSzie.width();
        break;
    default:
        m_sErrinf = QString(__FUNCTION__) + ": parameter invalid.";
        return false;
    }

    if (oldOri == devMode.dmDisplayOrientation) {
        return true;
    }

    devMode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYORIENTATION;
    ret = ChangeDisplaySettingsEx(pDev, &devMode, NULL, CDS_RESET, NULL);

    m_sErrinf = errinf(ret);

    return ret == DISP_CHANGE_SUCCESSFUL;
}

RotateDisp::ROTATE RotateDisp::getRotate()
{
    return getRotate("");
}

RotateDisp::ROTATE RotateDisp::getRotate(QString name)
{
    DEVMODE devMode;

    wchar_t dev[256] = { 0 };
    wchar_t* pDev = dev;
    if (!name.isEmpty())
        name.toWCharArray(dev);
    else
        pDev = NULL;

    // Init DEVMODE to current settings
    ZeroMemory(&devMode, sizeof(DEVMODE));
    devMode.dmSize = sizeof(devMode);
    EnumDisplaySettingsEx(pDev, ENUM_CURRENT_SETTINGS, &devMode, NULL);

    switch (devMode.dmDisplayOrientation) {
    case DMDO_DEFAULT: return RotateDisp::ROTATE_DEFAULT; break;
    case DMDO_90: return RotateDisp::ROTATE_90; break;
    case DMDO_180: return RotateDisp::ROTATE_180; break;
    case DMDO_270: return RotateDisp::ROTATE_270; break;
    default:return RotateDisp::ROTATE_DEFAULT; break;
    }
}

int RotateDisp::getMonitorCount()
{
    int valid = 0;
    int deviceIndex = 0;

    DISPLAY_DEVICE displayDevice;
    displayDevice.cb = sizeof(DISPLAY_DEVICE);
    int result;
    do {
        result = EnumDisplayDevices(NULL, deviceIndex++, &displayDevice, 0);
        if (displayDevice.StateFlags & DISPLAY_DEVICE_ACTIVE) {
            PDISPLAY_DEVICE monitor = new DISPLAY_DEVICE();
            monitor->cb = sizeof(DISPLAY_DEVICE);
            EnumDisplayDevices(displayDevice.DeviceName, 0, monitor, 0);
            delete monitor;

            ++valid;
        }
    } while (result);

    return valid;
}

QString RotateDisp::getMonitorName(int index)
{
    QString ret = "";

    DISPLAY_DEVICE displayDevice;
    displayDevice.cb = sizeof(DISPLAY_DEVICE);
    int result = EnumDisplayDevices(NULL, index, &displayDevice, 0);
    if (result != 0 && (displayDevice.StateFlags & DISPLAY_DEVICE_ACTIVE)) {
        PDISPLAY_DEVICE monitor = new DISPLAY_DEVICE();
        monitor->cb = sizeof(DISPLAY_DEVICE);
        EnumDisplayDevices(displayDevice.DeviceName, 0, monitor, 0);
        delete monitor;

        ret = QString::fromWCharArray(displayDevice.DeviceName);
    }

    return ret;
}
