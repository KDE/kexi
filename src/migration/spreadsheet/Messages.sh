#! /bin/sh
$EXTRACTRC *.rc *.ui >> rc.cpp
$XGETTEXT -kkundo2_i18nc:1c,2 -kkundo2_i18ncp:1c,2,3 `find . -name \*.cpp ` -o $podir/keximigrate_spreadsheet.pot
rm -f rc.cpp
