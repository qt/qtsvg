# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qsvgrenderer Test:
#####################################################################

qt_internal_add_test(tst_qsvgrenderer
    SOURCES
        tst_qsvgrenderer.cpp
    DEFINES
        SRCDIR="${CMAKE_CURRENT_SOURCE_DIR}/"
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
        Qt::Svg
        Qt::Widgets
)

# Resources:
set(resources_resource_files
    "heart.svgz"
    "large.svg"
    "large.svgz"
)

qt_internal_add_resource(tst_qsvgrenderer "resources"
    PREFIX
        "/"
    FILES
        ${resources_resource_files}
)
