# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

add_subdirectory(embedded)
add_subdirectory(richtext)
add_subdirectory(draganddrop)
if(TARGET Qt::Widgets)
    qt_internal_add_example(embeddedsvgviewer)
    qt_internal_add_example(svggenerator)
    qt_internal_add_example(svgviewer)
endif()
