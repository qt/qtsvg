TEMPLATE = subdirs
!contains(QT_CONFIG, no-widgets): SUBDIRS += desktopservices fluidlauncher weatherinfo
QT+=widgets

# install
sources.files = embedded.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtsvg/embedded
INSTALLS += sources
