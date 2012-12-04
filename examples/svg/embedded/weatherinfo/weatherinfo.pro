TEMPLATE = app
TARGET = weatherinfo
SOURCES = weatherinfo.cpp
RESOURCES = weatherinfo.qrc
QT += network svg

target.path = $$[QT_INSTALL_EXAMPLES]/svg/embedded/weatherinfo
INSTALLS += target
QT+=widgets
