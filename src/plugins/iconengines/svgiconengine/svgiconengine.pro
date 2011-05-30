load(qt_module)

TARGET  = qsvgicon
load(qt_plugin)

HEADERS += qsvgiconengine.h
SOURCES += main.cpp \
           qsvgiconengine.cpp
QT += xml svg

DESTDIR  = $$QT.svg.plugins/iconengines
target.path += $$[QT_INSTALL_PLUGINS]/iconengines
INSTALLS += target

symbian:TARGET.UID3=0x2001B2E3
