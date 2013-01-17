TARGET  = qsvgicon

PLUGIN_TYPE = iconengines
PLUGIN_CLASS_NAME = QSvgIconPlugin
load(qt_plugin)

HEADERS += qsvgiconengine.h
SOURCES += main.cpp \
           qsvgiconengine.cpp
OTHER_FILES += qsvgiconengine.json
QT += xml svg gui
