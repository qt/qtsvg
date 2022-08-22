# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from auto.pro.

if(TARGET Qt::Widgets)
    add_subdirectory(qsvgdevice)
    if(TARGET Qt::Xml) # special case
        add_subdirectory(qsvggenerator) # special case
    endif() # special case
    add_subdirectory(qsvgplugin)
    add_subdirectory(qicon_svg)
    # add_subdirectory(cmake) # special case remove
    # add_subdirectory(installed_cmake) # special case remove
endif()
if(TARGET Qt::Widgets AND NOT ANDROID)
    add_subdirectory(qsvgrenderer)
endif()
