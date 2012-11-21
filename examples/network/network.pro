TEMPLATE = subdirs
!contains(QT_CONFIG, no-widgets): SUBDIRS += bearercloud
QT+=widgets

# install
sources.files = network.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtsvg/network
INSTALLS += sources
