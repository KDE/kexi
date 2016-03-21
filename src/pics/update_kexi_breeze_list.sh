#!/bin/sh
#
# Updates list of files for kexi_breeze.rcc.
#

(echo "set(_FILES"; find . -name \*svg -o -name \*png | sed "s/\.\///g" | sort; echo ")") \
    > kexi_breeze_files.cmake
