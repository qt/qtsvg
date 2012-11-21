TEMPLATE = subdirs
!contains(QT_CONFIG, no-widgets): SUBDIRS += textobject
QT+=widgets

# install
sources.files = richtext.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtsvg/richtext
INSTALLS += sources
