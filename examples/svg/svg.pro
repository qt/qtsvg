TEMPLATE = subdirs

!contains(QT_CONFIG, no-widgets): SUBDIRS += embeddedsvgviewer  svggenerator  svgviewer
SUBDIRS += embedded richtext draganddrop network

contains(QT_CONFIG,opengl):!contains(QT_CONFIG,opengles2):SUBDIRS += opengl

QT+=widgets
