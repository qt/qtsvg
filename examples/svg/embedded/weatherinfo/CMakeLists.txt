# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(svgweatherinfo LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/svg/embedded/weatherinfo")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Network Svg SvgWidgets Widgets)

qt_add_executable(svgweatherinfo
    weatherinfo.cpp
)

set_target_properties(svgweatherinfo PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(svgweatherinfo PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Network
    Qt::Svg
    Qt::SvgWidgets
    Qt::Widgets
)

# Resources:
set(weatherinfo_resource_files
    "icons/weather-few-clouds.svg"
    "icons/weather-fog.svg"
    "icons/weather-haze.svg"
    "icons/weather-icy.svg"
    "icons/weather-overcast.svg"
    "icons/weather-showers.svg"
    "icons/weather-sleet.svg"
    "icons/weather-snow.svg"
    "icons/weather-storm.svg"
    "icons/weather-sunny-very-few-clouds.svg"
    "icons/weather-sunny.svg"
    "icons/weather-thundershower.svg"
)

qt6_add_resources(svgweatherinfo "weatherinfo"
    PREFIX
        "/"
    FILES
        ${weatherinfo_resource_files}
)

install(TARGETS svgweatherinfo
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
