QT       += core gui

CONFIG += link_pkgconfig
DEFINES += LINK_LIBUDEV
PKGCONFIG += libudev

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
