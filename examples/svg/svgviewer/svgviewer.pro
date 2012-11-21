HEADERS       = mainwindow.h \
                svgview.h
RESOURCES     = svgviewer.qrc
SOURCES       = main.cpp \
                mainwindow.cpp \
                svgview.cpp
QT           += svg xml

contains(QT_CONFIG, opengl): QT += opengl

CONFIG += console

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtsvg/svg/svgviewer
sources.files = $$SOURCES $$HEADERS $$RESOURCES svgviewer.pro files
sources.path = $$[QT_INSTALL_EXAMPLES]/qtsvg/svg/svgviewer
INSTALLS += target sources

wince*: {
     addFiles.files = files\\*.svg
     addFiles.path = "\\My Documents"
     DEPLOYMENT += addFiles
}

QT+=widgets
