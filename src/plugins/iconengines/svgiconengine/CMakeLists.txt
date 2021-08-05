# Generated from svgiconengine.pro.

#####################################################################
## QSvgIconPlugin Plugin:
#####################################################################

qt_internal_add_plugin(QSvgIconPlugin
    OUTPUT_NAME qsvgicon
    PLUGIN_TYPE iconengines
    SOURCES
        main.cpp
        qsvgiconengine.cpp qsvgiconengine.h
    LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::Svg
)

#### Keys ignored in scope 1:.:.:svgiconengine.pro:<TRUE>:
# OTHER_FILES = "qsvgiconengine.json" "qsvgiconengine-nocompress.json"
# PLUGIN_EXTENDS = "svg"
