# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from svg.pro.

#####################################################################
## QSvgPlugin Plugin:
#####################################################################

qt_internal_add_plugin(QSvgPlugin
    OUTPUT_NAME qsvg
    PLUGIN_TYPE imageformats
    SOURCES
        main.cpp
        qsvgiohandler.cpp qsvgiohandler.h
    LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Svg
)

#### Keys ignored in scope 1:.:.:svg.pro:<TRUE>:
# PLUGIN_EXTENDS = "svg"
