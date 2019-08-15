# Generated from svg.pro.

#####################################################################
## qsvg Plugin:
#####################################################################

add_qt_plugin(qsvg
    TYPE imageformats
    CLASS_NAME QSvgPlugin
    SOURCES
        main.cpp
        qsvgiohandler.cpp qsvgiohandler.h
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Svg
)

#### Keys ignored in scope 1:.:.:svg.pro:<TRUE>:
# PLUGIN_CLASS_NAME = "QSvgPlugin"
# PLUGIN_EXTENDS = "svg"
# PLUGIN_TYPE = "imageformats"
# _LOADED = "qt_plugin"
