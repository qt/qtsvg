# Generated from embeddedsvgviewer.pro.

cmake_minimum_required(VERSION 3.16)
project(embeddedsvgviewer LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/svg/embeddedsvgviewer")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Widgets)
find_package(Qt6 COMPONENTS Svg)

qt_add_executable(embeddedsvgviewer
    embeddedsvgviewer.cpp embeddedsvgviewer.h
    main.cpp
)
set_target_properties(embeddedsvgviewer PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(embeddedsvgviewer PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Svg
    Qt::Widgets
)


# Resources:
set(embeddedsvgviewer_resource_files
    "files/default.svg"
    "files/v-slider-handle.svg"
)

qt6_add_resources(embeddedsvgviewer "embeddedsvgviewer"
    PREFIX
        "/"
    FILES
        ${embeddedsvgviewer_resource_files}
)

install(TARGETS embeddedsvgviewer
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
