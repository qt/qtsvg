# special case begin
if(QT_BUILD_STANDALONE_TESTS)
    qt_find_package(Qt6 REQUIRED COMPONENTS BuildInternals Core Test)
    qt_find_package(Qt6 COMPONENTS Svg Widgets Xml)
endif()
# special case end

qt_build_tests()
