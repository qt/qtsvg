TEMPLATE = subdirs
SUBDIRS += embedded richtext draganddrop painting network

contains(QT_CONFIG,opengl):!contains(QT_CONFIG,opengles2):SUBDIRS += opengl
QT+=widgets
