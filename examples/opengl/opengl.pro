TEMPLATE = subdirs
!contains(QT_CONFIG, no-widgets): SUBDIRS += framebufferobject
QT+=widgets

# install
sources.files = opengl.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtsvg/opengl
INSTALLS += sources
