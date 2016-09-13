#! /bin/sh
source ../../../kexi_xgettext.sh

potfile=keximigrate_mdb.pot
find_exclude $potfile

# Exclude files containing "#warning noi18n"
LIST=`find . \( $EXCLUDE \) -prune -o \( -name \*.h -o -name \*.cpp -o -name \*.cc -o -name \*.hxx -o -name \*.cxx \) -type f -print | sort | while read f ; do \
    if ! grep -q '^#warning noi18n ' $f ; then echo $f; fi \
done \
`
kexi_xgettext $potfile $LIST
rm -f rc.cpp
