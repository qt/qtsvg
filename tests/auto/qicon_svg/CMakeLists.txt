# Generated from qicon_svg.pro.

#####################################################################
## tst_qicon_svg Test:
#####################################################################

add_qt_test(tst_qicon_svg
    SOURCES
        tst_qicon_svg.cpp
    LIBRARIES
        Qt::GuiPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Svg
)

# Resources:
add_qt_resource(tst_qicon_svg "tst_qicon_svg" PREFIX "/" FILES
    ./icons/heart.svg
    ./icons/heart.svgz
    ./icons/image.png
    ./icons/rect.svg
    ./icons/rect_size_100percent.svg
    ./icons/trash.svg)


#### Keys ignored in scope 1:.:.:qicon_svg.pro:<TRUE>:
# CONFIG = "testcase"
# TESTDATA = "icons/*"
