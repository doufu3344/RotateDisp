QT       += core gui serialport

win32:VERSION = 1.2.0.0
else: VERSION = 1.2.0

win32{
    RC_ICONS = RotateDisp.ico
    QMAKE_TARGET_DESCRIPTION = "Rotate dispaly based on GY-25T"
    QMAKE_TARGET_COPYRIGHT = doufu
}

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

SOURCES += \
    gy_25t_ttl.cpp \
    main.cpp \
    mainwindow.cpp \
    rotatedisp_win.cpp

HEADERS += \
    gy_25t_ttl.h \
    mainwindow.h \
    rotatedisp.h

FORMS += \
    mainwindow.ui

RESOURCES += \
    res.qrc

TRANSLATIONS = translations/RotateDisp_zh_CN.ts

