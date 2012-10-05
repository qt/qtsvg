TEMPLATE = subdirs

!contains(QT_CONFIG, no-widgets): SUBDIRS += embeddedsvgviewer  svggenerator  svgviewer

QT+=widgets


