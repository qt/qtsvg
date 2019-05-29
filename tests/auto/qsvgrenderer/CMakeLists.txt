# Generated from qsvgrenderer.pro.

#####################################################################
## tst_qsvgrenderer Test:
#####################################################################

add_qt_test(tst_qsvgrenderer
    SOURCES
        tst_qsvgrenderer.cpp
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
add_qt_resource(tst_qsvgrenderer "resources" FILES
    heart.svgz)


#### Keys ignored in scope 1:.:.:qsvgrenderer.pro:<TRUE>:
# CONFIG = "testcase"
