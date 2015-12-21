#! /bin/sh
source ../../../kexi_xgettext.sh

$EXTRACTRC *.rc *.ui >> rc.cpp
kexi_xgettext keximigrate_spreadsheet.pot `find . -name \*.cpp`
rm -f rc.cpp
