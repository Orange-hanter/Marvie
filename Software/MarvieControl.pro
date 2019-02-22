#-------------------------------------------------
#
# Project created by QtCreator 2019-02-16T19:22:33
#
#-------------------------------------------------

QT       += core xml network gui xmlpatterns widgets bluetooth serialport printsupport concurrent charts

TARGET = MarvieControl
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++14

SOURCES += \
    AccountWindow.cpp \
    ComPortsConfigWidget.cpp \
    Crc32SW.cpp \
    DataTransferProgressWindow.cpp \
    main.cpp \
    MarvieControl.cpp \
    MLinkClient.cpp \
    MonitoringDataItem.cpp \
    MonitoringDataModel.cpp \
    MonitoringDataTreeWidget.cpp \
    MonitoringLog.cpp \
    SensorDescription.cpp \
    SensorErrorsModel.cpp \
    SensorFieldAddressMapModel.cpp \
    SensorUnfoldedDesc.cpp \
    SynchronizationWindow.cpp \
    VPortOverIpModel.cpp \
    VPortsOverIpDelegate.cpp \
    VPortTileListWidget.cpp \
    VPortTileWidget.cpp

HEADERS += \
    AccountWindow.h \
    ComPortsConfigWidget.h \
    Crc32SW.h \
    DataTransferProgressWindow.h \
    MarvieControl.h \
    MLinkClient.h \
    MonitoringDataItem.h \
    MonitoringDataModel.h \
    MonitoringDataTreeWidget.h \
    MonitoringLog.h \
    SensorDescription.h \
    SensorErrorsModel.h \
    SensorFieldAddressMapModel.h \
    SensorUnfoldedDesc.h \
    SynchronizationWindow.h \
    VPortOverIpModel.h \
    VPortsOverIpDelegate.h \
    VPortTileListWidget.h \
    VPortTileWidget.h \
    AccountWindow.h \
    ComPortsConfigWidget.h \
    Crc32SW.h \
    DataTransferProgressWindow.h \
    MarvieControl.h \
    MLinkClient.h \
    MonitoringDataItem.h \
    MonitoringDataModel.h \
    MonitoringDataTreeWidget.h \
    MonitoringLog.h \
    SensorDescription.h \
    SensorErrorsModel.h \
    SensorFieldAddressMapModel.h \
    SensorUnfoldedDesc.h \
    SynchronizationWindow.h \
    VPortOverIpModel.h \
    VPortsOverIpDelegate.h \
    VPortTileListWidget.h \
    VPortTileWidget.h

RESOURCES += \
    MarvieControl.qrc

FORMS += \
    AccountWindow.ui \
    MarvieControl.ui \
    SdStatistics.ui

# Rules for deployment.
unix {
    isEmpty(PREFIX) {
        PREFIX = /usr/local
    }

    target.path = $$PREFIX/bin

    desktop.path = /usr/share/applications
    desktop.files += marviecontrol.desktop
    icons.path = /usr/share/icons/hicolor/48x48/apps
    icons.files += MarvieControl.ico

    data.path = $$PREFIX/share/MarvieControl/

    INSTALLS += target desktop icons data
}

DISTFILES += \
    marviecontrol.desktop
