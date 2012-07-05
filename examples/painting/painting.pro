TEMPLATE = subdirs
!contains(QT_CONFIG, no-widgets): SUBDIRS += svgviewer svggenerator
QT+=widgets
