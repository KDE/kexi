#! /bin/sh
source ../../../kundo2_aware_xgettext.sh

potfile=keximigrate_odb.pot
find_exclude $potfile

# Exclude files containing "#warning noi18n"
LIST=`find . \( $EXCLUDE \) -prune -o \( -name \*.h -o -name \*.cpp -o -name \*.cc -o -name \*.hxx -o -name \*.cxx \) -type f -print | sort | while read f ; do \
    if ! grep -q '^#warning noi18n ' $f ; then echo $f; fi \
done \
`
kundo2_aware_xgettext $potfile $LIST
rm -f rc.cpp
