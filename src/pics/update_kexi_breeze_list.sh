#!/bin/sh
#
# Updates list of files for kexi_breeze.rcc.
#

function content()
{
#    echo "include(colorize.cmake)"
#    echo
    echo "set(_PNG_FILES"
    find . -name \*png | sed "s/\.\///g" | sort
    echo ")"
    echo

    echo "set(_SVG_OBJECT_FILES"
    find . -regex ".*/\(table\|query\|form\|report\|macro\|script\)\.svg$" \
        | sed "s/\.\///g" | sort
    echo ")"
    echo

    echo "set(_SVG_FILES"
    find . -name \*svg | grep -v -F "table.svg
query.svg
form.svg
report.svg
macro.svg
script.svg" \
        | sed "s/\.\///g" | sort
    echo ")"
    echo

    echo "set(_FILES \${_PNG_FILES} \${_SVG_OBJECT_FILES} \${_SVG_FILES})"
}

content > kexi_breeze_files.cmake
