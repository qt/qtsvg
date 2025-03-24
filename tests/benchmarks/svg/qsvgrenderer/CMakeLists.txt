# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_bench_qsvgrenderer Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_qsvgrenderer
    SOURCES
        tst_qsvgrenderer.cpp
    LIBRARIES
        Qt::Gui
        Qt::Test
        Qt::Widgets
        Qt::Svg
)

qt_internal_add_resource(tst_bench_qsvgrenderer "tst_bench_qsvgrenderer_dat"
    PREFIX
        "/"
    FILES
        "./data/tiger.svg"
)
