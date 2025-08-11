# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(svgviewer LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/svg/svgviewer")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Svg Widgets)

qt_add_executable(svgviewer
    exportdialog.cpp exportdialog.h
    main.cpp
    mainwindow.cpp mainwindow.h
    mainwindow.ui
    svgview.h svgview.cpp
)

qt_add_resources(svgviewer "files"
    FILES
        files/abstract_tree.svg
        files/black.png
        files/white.png
        files/chequered.png
)

set_target_properties(svgviewer PROPERTIES
    WIN32_EXECUTABLE FALSE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(svgviewer PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Svg
    Qt::Widgets
)

install(TARGETS svgviewer
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
