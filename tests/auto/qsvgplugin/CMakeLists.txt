# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qsvgplugin Test:
#####################################################################

# Collect test data
qt_internal_add_test(tst_qsvgplugin
    SOURCES
        tst_qsvgplugin.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
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
    "simple_Utf8.svg"
    "simple_Utf16LE.svg"
    "simple_Utf16BE.svg"
    "simple_Utf32LE.svg"
    "simple_Utf32BE.svg"
    "invalid_xml.svg"
    "xml_not_svg.svg"
    "invalid_then_valid.svg"
)

qt_internal_add_resource(tst_qsvgplugin "resources"
    PREFIX
        "/"
    FILES
        ${resources_resource_files}
)
