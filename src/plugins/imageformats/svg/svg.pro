TARGET  = qsvg

PLUGIN_TYPE = imageformats
load(qt_plugin)

HEADERS += qsvgiohandler.h
SOURCES += main.cpp \
           qsvgiohandler.cpp
QT += xml svg
