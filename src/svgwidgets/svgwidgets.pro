TARGET     = QtSvgWidgets
QT += svg core-private gui-private widgets-private

DEFINES   += QT_NO_USING_NAMESPACE
msvc:equals(QT_ARCH, i386): QMAKE_LFLAGS += /BASE:0x66000000

HEADERS += \
    qtsvgwidgetsglobal.h     \
    qsvgwidget.h            \
    qgraphicssvgitem.h

SOURCES += \
    qsvgwidget.cpp          \
    qgraphicssvgitem.cpp

load(qt_module)
