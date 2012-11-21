QT += widgets

VERSION = $$QT_VERSION

HEADERS += \
           demoapplication.h \
           fluidlauncher.h \
           pictureflow.h \
           slideshow.h

SOURCES += \
           demoapplication.cpp \
           fluidlauncher.cpp \
           main.cpp \
           pictureflow.cpp \
           slideshow.cpp


target.path = $$[QT_INSTALL_EXAMPLES]/qtsvg/embedded/fluidlauncher
sources.files = $$SOURCES $$HEADERS $$RESOURCES *.pro *.html config.xml screenshots slides
sources.path = $$[QT_INSTALL_EXAMPLES]/qtsvg/embedded/fluidlauncher
INSTALLS += target sources

wince*{
    QT += svg

    BUILD_DIR = release
    if(!debug_and_release|build_pass):CONFIG(debug, debug|release) {
        BUILD_DIR = debug
    }

    baseex = $$shadowed($$QT.qtbase.sources/../../examples)
    svgex = $$shadowed($$QT.qtsvg.sources/../../examples)
    executables.files = \
        $$svgex/embedded/embeddedsvgviewer/$${BUILD_DIR}/embeddedsvgviewer.exe \
        $$baseex/embedded/styleexample/$${BUILD_DIR}/styleexample.exe \
        $$baseex/painting/deform/$${BUILD_DIR}/deform.exe \
        $$baseex/painting/pathstroke/$${BUILD_DIR}/pathstroke.exe \
        $$baseex/graphicsview/elasticnodes/$${BUILD_DIR}/elasticnodes.exe \
        $$baseex/widgets/wiggly/$${BUILD_DIR}/wiggly.exe \
        $$baseex/painting/concentriccircles/$${BUILD_DIR}/concentriccircles.exe \
        $$baseex/draganddrop/$${BUILD_DIR}/fridgemagnets.exe

    executables.path = .

    files.files = $$PWD/screenshots $$PWD/slides $$PWD/../embeddedsvgviewer/shapes.svg
    files.path = .

    config.files = $$PWD/config_wince/config.xml
    config.path = .

    DEPLOYMENT += config files executables

    DEPLOYMENT_PLUGIN += qgif qjpeg qmng qsvg
}
