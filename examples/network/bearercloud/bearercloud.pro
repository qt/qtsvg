HEADERS = bearercloud.h \
          cloud.h

SOURCES = main.cpp \
          bearercloud.cpp \
          cloud.cpp

RESOURCES = icons.qrc

TARGET = bearercloud

QT = core gui widgets network svg

CONFIG += console

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtsvg/network/bearercloud
sources.files = $$SOURCES $$HEADERS *.pro $$RESOURCES *.svg
sources.path = $$[QT_INSTALL_EXAMPLES]/qtsvg/network/bearercloud
INSTALLS += target sources
