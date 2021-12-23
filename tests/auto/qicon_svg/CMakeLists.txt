# Generated from qicon_svg.pro.

#####################################################################
## tst_qicon_svg Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    icons/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qicon_svg
    LOWDPI # special case
    SOURCES
        tst_qicon_svg.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
        Qt::Svg
    TESTDATA ${test_data}
)

# Resources:
set(tst_qicon_svg_resource_files
    "./icons/heart.svg"
    "./icons/heart.svgz"
    "./icons/image.png"
    "./icons/rect.svg"
    "./icons/rect_size_100percent.svg"
    "./icons/trash.svg"
    "./icons/triangle.svg"
)

qt_internal_add_resource(tst_qicon_svg "tst_qicon_svg"
    PREFIX
        "/"
    FILES
        ${tst_qicon_svg_resource_files}
)

