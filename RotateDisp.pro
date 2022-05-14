QT       += core gui serialport

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

RC_FILE = icon.rc

