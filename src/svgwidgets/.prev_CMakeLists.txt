# Generated from svgwidgets.pro.

#####################################################################
## SvgWidgets Module:
#####################################################################

qt_internal_add_module(SvgWidgets
    SOURCES
        qgraphicssvgitem.cpp qgraphicssvgitem.h
        qsvgwidget.cpp qsvgwidget.h
        qtsvgwidgetsglobal.h
    DEFINES
        QT_NO_USING_NAMESPACE
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::WidgetsPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Svg
        Qt::Widgets
    PRIVATE_MODULE_INTERFACE
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::WidgetsPrivate
)

## Scopes:
#####################################################################

qt_internal_extend_target(SvgWidgets CONDITION MSVC AND (TEST_architecture_arch STREQUAL "i386")
    LINK_OPTIONS
        "/BASE:0x66000000"
)
