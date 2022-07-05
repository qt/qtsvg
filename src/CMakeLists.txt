# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from src.pro.

if(TARGET Qt::Gui)
    add_subdirectory(svg)
    add_subdirectory(plugins)
endif()
if(TARGET Qt::Widgets)
    add_subdirectory(svgwidgets)
endif()
