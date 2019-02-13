cmake_minimum_required(VERSION 3.14.0)

project(QtSvg
    VERSION 5.14.0
    DESCRIPTION "Qt SVG Libraries"
    HOMEPAGE_URL "https://qt.io/"
    LANGUAGES CXX C
)

find_package(Qt5 ${PROJECT_VERSION} CONFIG REQUIRED COMPONENTS Core Gui Widgets)

# TODO: Maybe make this var part of the find script already?
set(QT_CMAKE_MODULE_PATH "${_qt_cmake_dir}/Qt5")
list(APPEND CMAKE_MODULE_PATH ${QT_CMAKE_MODULE_PATH})
list(APPEND CMAKE_PREFIX_PATH ${QT_CMAKE_MODULE_PATH})

## Should this Qt be static or dynamically linked?
option(BUILD_SHARED_LIBS "Build Qt statically or dynamically" ON)
set(QT_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})

include(QtSetup)

include(FeatureSummary)

add_subdirectory(src)

if (BUILD_TESTING)
    find_package(Qt5 ${PROJECT_VERSION} CONFIG REQUIRED COMPONENTS Test Xml)
    add_subdirectory(tests)
endif()

if (BUILD_EXAMPLES)
    # TODO: Implement me
    #add_subdirectory(examples)
endif()

## Delayed actions on some of the Qt targets:
include(QtPostProcess)

## Print a feature summary:
feature_summary(WHAT PACKAGES_FOUND PACKAGES_NOT_FOUND FATAL_ON_MISSING_REQUIRED_PACKAGES)
