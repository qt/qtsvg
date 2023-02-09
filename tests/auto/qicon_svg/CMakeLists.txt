# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qicon_svg Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    icons/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qicon_svg
    LOWDPI
    SOURCES
        tst_qicon_svg.cpp
    LIBRARIES
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

