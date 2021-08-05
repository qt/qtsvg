# Generated from svg.pro.

#####################################################################
## QSvgPlugin Plugin:
#####################################################################

qt_internal_add_plugin(QSvgPlugin
    OUTPUT_NAME qsvg
    PLUGIN_TYPE imageformats
    SOURCES
        main.cpp
        qsvgiohandler.cpp qsvgiohandler.h
    LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Svg
)

#### Keys ignored in scope 1:.:.:svg.pro:<TRUE>:
# PLUGIN_EXTENDS = "svg"
