#-------------------------------------------------
#
# Project created by QtCreator 2014-06-30T17:06:20
#
#-------------------------------------------------QT       += core gui multimedia
QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = rinf-gui
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    gps-func.cpp \
    soundnotify.cpp \
    soundplay.cpp \
    logger.cpp \
    pixmaker.cpp

HEADERS  += mainwindow.h \
    gps-func.h \
    radarobjects.h \
    soundnotify.h \
    soundplay.h \
    logger.h \
    pixmaker.h

FORMS    +=

OTHER_FILES += \
    speedcam_navitel_format.txt \
    GKtoWGS.txt \
    WGtoGK.txt \
    img/mute-button.png

LIBS += -lgps -lasound

RESOURCES += \
    graph.qrc
