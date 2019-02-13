# Generated from svg.pro.

#####################################################################
## qsvg Plugin:
#####################################################################

add_qt_plugin(qsvg
    TYPE imageformats
    SOURCES
        main.cpp
        qsvgiohandler.cpp qsvgiohandler.h
    LIBRARIES
        Qt::Svg
    # PLUGIN_CLASS_NAME = "QSvgPlugin"
    # PLUGIN_EXTENDS = "svg"
    # _LOADED = "qt_plugin"
)
