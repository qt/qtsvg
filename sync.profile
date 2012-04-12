%modules = ( # path to module name map
    "QtSvg" => "$basedir/src/svg",
);
%moduleheaders = ( # restrict the module headers to those found in relative path
);
%classnames = (
    "qtsvgversion.h" => "QtSvgVersion",
);
%mastercontent = (
    "core" => "#include <QtCore/QtCore>\n",
    "gui" => "#include <QtGui/QtGui>\n",
);
# Module dependencies.
# Every module that is required to build this module should have one entry.
# Each of the module version specifiers can take one of the following values:
#   - A specific Git revision.
#   - any git symbolic ref resolvable from the module's repository (e.g. "refs/heads/master" to track master branch)
#
%dependencies = (
    "qtbase" => "refs/heads/master",
);
