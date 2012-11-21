TEMPLATE = subdirs
!contains(QT_CONFIG, no-widgets): SUBDIRS += delayedencoding
QT+=widgets

# install
sources.files = draganddrop.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtsvg/draganddrop
INSTALLS += sources
