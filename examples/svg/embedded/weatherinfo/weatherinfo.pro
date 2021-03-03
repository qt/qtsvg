TEMPLATE = app
TARGET = svgweatherinfo
SOURCES = weatherinfo.cpp
RESOURCES = weatherinfo.qrc
QT += network widgets svg svgwidgets

target.path = $$[QT_INSTALL_EXAMPLES]/svg/embedded/weatherinfo
INSTALLS += target
