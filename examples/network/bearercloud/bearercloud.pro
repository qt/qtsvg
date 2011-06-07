HEADERS = bearercloud.h \
          cloud.h

SOURCES = main.cpp \
          bearercloud.cpp \
          cloud.cpp

RESOURCES = icons.qrc

TARGET = bearercloud

QT = core gui network svg

CONFIG += console

symbian: {
    TARGET.CAPABILITY = NetworkServices ReadUserData
    CONFIG += qt_example
}
maemo5: CONFIG += qt_example
symbian: warning(This example might not fully work on Symbian platform)
maemo5: warning(This example might not fully work on Maemo platform)
