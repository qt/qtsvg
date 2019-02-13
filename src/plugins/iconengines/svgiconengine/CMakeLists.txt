# Generated from svgiconengine.pro.

#####################################################################
## qsvgicon Plugin:
#####################################################################

add_qt_plugin(qsvgicon
    TYPE iconengines
    SOURCES
        main.cpp
        qsvgiconengine.cpp qsvgiconengine.h
    PUBLIC_LIBRARIES
        Qt::Svg
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate

    # OTHER_FILES = "qsvgiconengine.json" "qsvgiconengine-nocompress.json"
    # PLUGIN_CLASS_NAME = "QSvgIconPlugin"
    # PLUGIN_EXTENDS = "svg"
    # _LOADED = "qt_plugin"
)
