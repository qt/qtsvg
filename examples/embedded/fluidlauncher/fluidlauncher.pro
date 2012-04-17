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

embedded{
    target.path = $$[QT_INSTALL_EXAMPLES]/qtsvg/embedded/fluidlauncher
    sources.files = $$SOURCES $$HEADERS $$RESOURCES *.pro *.html config.xml screenshots slides
    sources.path = $$[QT_INSTALL_EXAMPLES]/qtsvg/embedded/fluidlauncher
    INSTALLS += target sources
}

wince*{
    QT += svg

    BUILD_DIR = release
    if(!debug_and_release|build_pass):CONFIG(debug, debug|release) {
        BUILD_DIR = debug
    }

    executables.files = \
        $$QT_BUILD_TREE/qtsvg/examples/embedded/embeddedsvgviewer/$${BUILD_DIR}/embeddedsvgviewer.exe \
        $$QT_BUILD_TREE/qtbase/examples/embedded/styleexample/$${BUILD_DIR}/styleexample.exe \
        $$QT_BUILD_TREE/qtbase/examples/painting/deform/$${BUILD_DIR}/deform.exe \
        $$QT_BUILD_TREE/qtbase/examples/painting/pathstroke/$${BUILD_DIR}/pathstroke.exe \
        $$QT_BUILD_TREE/qtbase/examples/graphicsview/elasticnodes/$${BUILD_DIR}/elasticnodes.exe \
        $$QT_BUILD_TREE/qtbase/examples/widgets/wiggly/$${BUILD_DIR}/wiggly.exe \
        $$QT_BUILD_TREE/qtbase/examples/painting/concentriccircles/$${BUILD_DIR}/concentriccircles.exe \
        $$QT_BUILD_TREE/qtbase/examples/draganddrop/$${BUILD_DIR}/fridgemagnets.exe

    executables.path = .

    files.files = $$PWD/screenshots $$PWD/slides $$PWD/../embeddedsvgviewer/shapes.svg
    files.path = .

    config.files = $$PWD/config_wince/config.xml
    config.path = .

    DEPLOYMENT += config files executables

    DEPLOYMENT_PLUGIN += qgif qjpeg qmng qsvg
}
