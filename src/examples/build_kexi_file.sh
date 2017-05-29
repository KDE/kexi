#!/bin/sh
set -e

# Builds a single .kexi file from a .sql file specified as $1.
# The destination .kexi file is saved with name specified as $2
# and is set as read-only.
# $2 if optional if $1 is of a form "name.kexi.sql" then
# the destination file will be "name.kexi".
# Only .kexi file that is older than .sql file is recreated.
# sqlite3 command line tool is needed on the $PATH.

which sqlite3 > /dev/null

[ $# -lt 1 ] && echo "Missing .sql filename." && exit 1

if [ $# -lt 2 ] ; then
    kexi_file=`echo $1 | sed -e "s/\.kexi\.sql/\.kexi/"`
else
    kexi_file=$2
fi

if test -f "$kexi_file" -a ! "$kexi_file" -ot "$1" ; then
    echo "Local $kexi_file is newer than $1 - skipping it"
    exit 0
fi

rm -f "$kexi_file"
echo "Creating \"$kexi_file\" ... "
sqlite3 "$kexi_file" < "$1"
chmod a-w "$kexi_file"
echo "OK"
