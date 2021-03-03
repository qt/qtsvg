HEADERS       = mainwindow.h \
                svgview.h \
                exportdialog.h
RESOURCES     = svgviewer.qrc
SOURCES       = main.cpp \
                mainwindow.cpp \
                svgview.cpp \
                exportdialog.cpp
QT           += widgets svg svgwidgets

qtHaveModule(opengl): qtHaveModule(openglwidgets) {
    DEFINES += USE_OPENGLWIDGETS
    QT += opengl openglwidgets
}

CONFIG += console

# install
target.path = $$[QT_INSTALL_EXAMPLES]/svg/svgviewer
INSTALLS += target
