# Generated from svgviewer.pro.

cmake_minimum_required(VERSION 3.16)
project(svgviewer LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/svg/svgviewer")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Widgets)
find_package(Qt6 COMPONENTS Svg)
find_package(Qt6 COMPONENTS SvgWidgets)

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
