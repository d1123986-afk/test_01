QT       += core gui

# Linux/Unix-specific configuration
unix:!macx {
    CONFIG += link_pkgconfig
    DEFINES += LINK_LIBUDEV
    PKGCONFIG += libudev
}

# Windows-specific configuration
win32 {
    DEFINES += _WIN32_WINNT=0x0601
    LIBS += -lws2_32 -lsetupapi
    # Note: USBIP VHCI driver for Windows must be installed separately
    # See: https://github.com/cezanne/usbip-win
}

# Windows-specific configuration
win32 {
    DEFINES += _WIN32_WINNT=0x0601
    LIBS += -lws2_32 -lsetupapi
    # Note: USBIP VHCI driver for Windows must be installed separately
    # See: https://github.com/cezanne/usbip-win
}

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    qt_sokol_client.cpp \
    qt_sokol_client_driver.cpp \
    qt_sokol_client_names.cpp \
    qt_sokol_client_network.cpp

# Windows-specific source file for VHCI driver stub
win32:SOURCES += qt_sokol_client_driver_win_stub.cpp

HEADERS += \
    qt_sokol_client.h \
    qt_sokol_client_driver.h \
    qt_sokol_client_names.h \
    qt_sokol_client_network.h \
    qt_sokol_new_server.h

FORMS +=

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
