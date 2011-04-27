TEMPLATE = app
TARGET = weatherinfo
SOURCES = weatherinfo.cpp
RESOURCES = weatherinfo.qrc
QT += network svg

symbian {
    TARGET.UID3 = 0xA000CF77
    CONFIG += qt_demo
    TARGET.CAPABILITY = NetworkServices
}

target.path = $$[QT_INSTALL_DEMOS]/qtsvg/embedded/weatherinfo
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS *.pro
sources.path = $$[QT_INSTALL_DEMOS]/qtsvg/embedded/weatherinfo
INSTALLS += target sources
