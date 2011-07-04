TEMPLATE = subdirs
SUBDIRS += richtext draganddrop painting network desktop

contains(QT_CONFIG,opengl):SUBDIRS += opengl
