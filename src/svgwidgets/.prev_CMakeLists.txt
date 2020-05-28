#####################################################################
## SvgWidgets Module:
#####################################################################

qt_add_module(SvgWidgets
    SOURCES
        qtsvgwidgetsglobal.h
        qgraphicssvgitem.cpp qgraphicssvgitem.h
        qsvgwidget.cpp qsvgwidget.h
    DEFINES
        QT_NO_USING_NAMESPACE
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::WidgetsPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Widgets
        Qt::Svg
    PRIVATE_MODULE_INTERFACE
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::WidgetsPrivate
)
