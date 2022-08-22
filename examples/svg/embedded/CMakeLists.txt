# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(TARGET Qt::Widgets)
    qt_internal_add_example(desktopservices)
endif()
if(QT_FEATURE_process AND TARGET Qt::Widgets)
    qt_internal_add_example(fluidlauncher)
endif()
if(TARGET Qt::Network AND TARGET Qt::Widgets)
    qt_internal_add_example(weatherinfo)
endif()
