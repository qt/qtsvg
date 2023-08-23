# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause


# Collect test data
list(APPEND test_data "../data")
set(BASELINE_DIR "../../../../qtbase/tests/baseline/shared")

find_package(Qt6 ${PROJECT_VERSION} CONFIG REQUIRED COMPONENTS Network)

qt_internal_add_test(tst_baseline_qsvgrenderer
    SOURCES
         ${BASELINE_DIR}/baselineprotocol.cpp
         ${BASELINE_DIR}/baselineprotocol.h
         ${BASELINE_DIR}/lookup3.cpp
         ${BASELINE_DIR}/qbaselinetest.cpp
         ${BASELINE_DIR}/qbaselinetest.h
        tst_baseline_qsvgrenderer.cpp
    INCLUDE_DIRECTORIES
        ${BASELINE_DIR}
    LIBRARIES
        Qt::Svg
        Qt::Network
    TESTDATA ${test_data}
)
