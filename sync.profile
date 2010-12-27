%modules = ( # path to module name map
    "QtSvg" => "$basedir/src/svg",
);
%moduleheaders = ( # restrict the module headers to those found in relative path
);
%classnames = (
);
%mastercontent = (
    "core" => "#include <QtCore/QtCore>\n",
    "gui" => "#include <QtGui/QtGui>\n",
);
%modulepris = (
    "QtSvg" => "$basedir/modules/qt_svg.pri",
);
