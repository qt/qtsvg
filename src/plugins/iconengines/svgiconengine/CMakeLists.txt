# Generated from svgiconengine.pro.

#####################################################################
## qsvgicon Plugin:
#####################################################################

qt_add_plugin(qsvgicon
    TYPE iconengines
    CLASS_NAME QSvgIconPlugin
    SOURCES
        main.cpp
        qsvgiconengine.cpp qsvgiconengine.h
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Svg
)

#### Keys ignored in scope 1:.:.:svgiconengine.pro:<TRUE>:
# OTHER_FILES = "qsvgiconengine.json" "qsvgiconengine-nocompress.json"
# PLUGIN_EXTENDS = "svg"
