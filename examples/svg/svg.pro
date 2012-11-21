TEMPLATE = subdirs

!contains(QT_CONFIG, no-widgets): SUBDIRS += embeddedsvgviewer  svggenerator  svgviewer

QT+=widgets

# install
sources.files = svg.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtsvg/svg
INSTALLS += sources
