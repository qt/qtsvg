TEMPLATE = app
TARGET = weatherinfo
SOURCES = weatherinfo.cpp
RESOURCES = weatherinfo.qrc
QT += network svg

target.path = $$[QT_INSTALL_EXAMPLES]/qtsvg/embedded/weatherinfo
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS *.pro icons
sources.path = $$[QT_INSTALL_EXAMPLES]/qtsvg/embedded/weatherinfo
INSTALLS += target sources
QT+=widgets
