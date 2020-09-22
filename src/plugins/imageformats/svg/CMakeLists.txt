# Generated from svg.pro.

#####################################################################
## QSvgPlugin Plugin:
#####################################################################

qt_internal_add_plugin(QSvgPlugin
    OUTPUT_NAME qsvg
    TYPE imageformats
    SOURCES
        main.cpp
        qsvgiohandler.cpp qsvgiohandler.h
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Svg
)

#### Keys ignored in scope 1:.:.:svg.pro:<TRUE>:
# PLUGIN_EXTENDS = "svg"
