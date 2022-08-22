# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(svgviewer LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/svg/svgviewer")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Svg SvgWidgets Widgets)

qt_add_executable(svgviewer
    exportdialog.cpp exportdialog.h
    main.cpp
    mainwindow.cpp mainwindow.h
    svgview.cpp svgview.h
)

set_target_properties(svgviewer PROPERTIES
    WIN32_EXECUTABLE FALSE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(svgviewer PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Svg
    Qt::SvgWidgets
    Qt::Widgets
)

# Resources:
set(svgviewer_resource_files
    "files/bubbles.svg"
)

qt6_add_resources(svgviewer "svgviewer"
    PREFIX
        "/"
    FILES
        ${svgviewer_resource_files}
)

if(TARGET Qt::OpenGL AND TARGET Qt::OpenGLWidgets)
    target_compile_definitions(svgviewer PUBLIC
        USE_OPENGLWIDGETS
    )

    target_link_libraries(svgviewer PUBLIC
        Qt::OpenGL
        Qt::OpenGLWidgets
    )
endif()

install(TARGETS svgviewer
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
