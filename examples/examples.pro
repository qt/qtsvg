TEMPLATE = subdirs
SUBDIRS += embedded richtext draganddrop painting network desktop

contains(QT_CONFIG,opengl):SUBDIRS += opengl
QT+=widgets
