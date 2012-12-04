TEMPLATE = subdirs
!contains(QT_CONFIG, no-widgets): SUBDIRS += desktopservices fluidlauncher weatherinfo
QT+=widgets
