# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qsvgdevice Test:
#####################################################################

qt_internal_add_test(tst_qsvgdevice
    SOURCES
        tst_qsvgdevice.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
        Qt::Widgets
)
