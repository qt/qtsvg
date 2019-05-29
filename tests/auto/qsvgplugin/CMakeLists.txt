# Generated from qsvgplugin.pro.

#####################################################################
## tst_qsvgplugin Test:
#####################################################################

add_qt_test(tst_qsvgplugin
    SOURCES
        tst_qsvgplugin.cpp
    DEFINES
        SRCDIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/\\\"
    LIBRARIES
        Qt::GuiPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Svg
        Qt::Widgets
)

# Resources:
add_qt_resource(tst_qsvgplugin "resources" FILES
    imageInclude.svg
    imageIncludeA.svg
    square.svg
    square_size.svg
    square_size_viewbox.svg
    square_viewbox.svg
    tall.svg
    tall_size.svg
    tall_size_viewbox.svg
    tall_viewbox.svg
    wide.svg
    wide_size.svg
    wide_size_viewbox.svg
    wide_viewbox.svg)


#### Keys ignored in scope 1:.:.:qsvgplugin.pro:<TRUE>:
# CONFIG = "testcase"
