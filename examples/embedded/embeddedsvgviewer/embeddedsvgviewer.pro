TEMPLATE = app
QT += widgets svg

HEADERS += embeddedsvgviewer.h
SOURCES += embeddedsvgviewer.cpp main.cpp
RESOURCES += embeddedsvgviewer.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/qtsvg/embedded/embeddedsvgviewer
sources.files = $$SOURCES $$HEADERS $$RESOURCES *.pro *.html *.svg files
sources.path = $$[QT_INSTALL_EXAMPLES]/qtsvg/embedded/embeddedsvgviewer
INSTALLS += target sources

wince* {
   DEPLOYMENT_PLUGIN += qsvg
}

