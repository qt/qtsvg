TARGET  = qsvgicon
load(qt_plugin)

HEADERS += qsvgiconengine.h
SOURCES += main.cpp \
           qsvgiconengine.cpp
OTHER_FILES += qsvgiconengine.json
QT += xml svg gui

DESTDIR  = $$QT.svg.plugins/iconengines
target.path += $$[QT_INSTALL_PLUGINS]/iconengines
INSTALLS += target
