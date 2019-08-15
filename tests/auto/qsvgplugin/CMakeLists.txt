# Generated from qsvgplugin.pro.

#####################################################################
## tst_qsvgplugin Test:
#####################################################################

add_qt_test(tst_qsvgplugin
    GUI
    SOURCES
        tst_qsvgplugin.cpp
    LIBRARIES
        Qt::GuiPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Svg
        Qt::Widgets
)

# Resources:
set(resources_resource_files
    "imageInclude.svg"
    "imageIncludeA.svg"
    "square.svg"
    "square_size.svg"
    "square_size_viewbox.svg"
    "square_viewbox.svg"
    "tall.svg"
    "tall_size.svg"
    "tall_size_viewbox.svg"
    "tall_viewbox.svg"
    "wide.svg"
    "wide_size.svg"
    "wide_size_viewbox.svg"
    "wide_viewbox.svg"
)

add_qt_resource(tst_qsvgplugin "resources"
    PREFIX
        "/"
    FILES
        ${resources_resource_files}
)

