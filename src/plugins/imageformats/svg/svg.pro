load(qt_build_config)

TARGET  = qsvg
load(qt_plugin)

HEADERS += qsvgiohandler.h
SOURCES += main.cpp \
           qsvgiohandler.cpp
QT += xml svg

DESTDIR = $$QT.svg.plugins/imageformats
target.path += $$[QT_INSTALL_PLUGINS]/imageformats
INSTALLS += target
