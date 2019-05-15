cmake_minimum_required(VERSION 3.14.0)

project(QtSvg
    VERSION 5.14.0
    DESCRIPTION "Qt SVG Libraries"
    HOMEPAGE_URL "https://qt.io/"
    LANGUAGES CXX C
)

find_package(Qt5 ${PROJECT_VERSION} CONFIG REQUIRED COMPONENTS BuildInternals Core Gui Widgets)
qt_build_repo_begin()

add_subdirectory(src)

if (BUILD_TESTING)
    find_package(Qt5 ${PROJECT_VERSION} CONFIG REQUIRED COMPONENTS Test Xml)
    add_subdirectory(tests)
endif()

if (BUILD_EXAMPLES)
    # TODO: Implement me
    #add_subdirectory(examples)
endif()

qt_build_repo_end()
