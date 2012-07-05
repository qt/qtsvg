TEMPLATE = subdirs
!contains(QT_CONFIG, no-widgets): SUBDIRS += desktopservices embeddedsvgviewer fluidlauncher weatherinfo
QT+=widgets
