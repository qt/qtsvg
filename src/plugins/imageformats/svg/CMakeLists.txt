# Generated from svg.pro.

#####################################################################
## qsvg Plugin:
#####################################################################

qt_add_plugin(qsvg
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
# PLUGIN_EXTENDS = "svg"
