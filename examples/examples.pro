TEMPLATE = subdirs
SUBDIRS += embedded richtext draganddrop network svg

contains(QT_CONFIG,opengl):!contains(QT_CONFIG,opengles2):SUBDIRS += opengl
QT+=widgets
