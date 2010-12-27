load(qt_module)

TARGET  = qsvg
include($$QT_SOURCE_TREE/src/plugins/qpluginbase.pri)

HEADERS += qsvgiohandler.h
SOURCES += main.cpp \
           qsvgiohandler.cpp
QT += xml svg

DESTDIR = $$QT_BUILD_TREE/plugins/imageformats
target.path += $$[QT_INSTALL_PLUGINS]/imageformats
INSTALLS += target

symbian:TARGET.UID3=0x2001E618
