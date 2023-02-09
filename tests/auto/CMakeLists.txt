# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# For now, don't built auto tests when QT_BUILD_MINIMAL_STATIC_TEST
# is specified and the build is targeting iOS. QT_BUILD_MINIMAL_STATIC_TEST is used in our CI.
# Regular non-cmake build tests shouldn't be built because the CI will try to run them and fail
# due to missing simulator support.
if(IOS AND QT_BUILD_MINIMAL_STATIC_TESTS)
    return()
endif()

if(TARGET Qt::Widgets)
    add_subdirectory(qsvgdevice)
    if(TARGET Qt::Xml)
        add_subdirectory(qsvggenerator)
    endif()
    add_subdirectory(qsvgplugin)
    add_subdirectory(qicon_svg)
    # add_subdirectory(cmake) # special case remove
    # add_subdirectory(installed_cmake) # special case remove
endif()
if(TARGET Qt::Widgets AND NOT ANDROID)
    add_subdirectory(qsvgrenderer)
endif()
