# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial

add_subdirectory(iconengines)
if(QT_FEATURE_imageformatplugin)
    add_subdirectory(imageformats)
endif()
