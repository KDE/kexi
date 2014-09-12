#! /bin/sh
source ../../../../../calligra_xgettext.sh

potfile=kformdesigner_mapbrowser
find_exclude $potfile

# Exclude files containing "#warning noi18n"
LIST=`find . \( $EXCLUDE \) -prune -o \( -name \*.h -o -name \*.cpp -o -name \*.cc -o -name \*.hxx -o -name \*.cxx \) -type f -print | sort | while read f ; do \
    if ! grep -q '^#warning noi18n ' $f ; then echo $f; fi \
done \
`
calligra_xgettext $potfile.pot $LIST
rm -f rc.cpp
