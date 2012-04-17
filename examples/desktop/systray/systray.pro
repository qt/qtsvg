HEADERS       = window.h
SOURCES       = main.cpp \
                window.cpp
RESOURCES     = systray.qrc
QT           += xml svg

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtsvg/desktop/systray
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS systray.pro resources images
sources.path = $$[QT_INSTALL_EXAMPLES]/qtsvg/desktop/systray
INSTALLS += target sources

wince* {
	CONFIG(debug, release|debug) {
		addPlugins.files = $$QT_BUILD_TREE/plugins/imageformats/qsvgd4.dll
	}
	CONFIG(release, release|debug) {
		addPlugins.files = $$QT_BUILD_TREE/plugins/imageformats/qsvg4.dll
	}
	addPlugins.path = imageformats
	DEPLOYMENT += addPlugins
}

simulator: warning(This example does not work on Simulator platform)
QT+=widgets
