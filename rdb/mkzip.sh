#!/bin/sh -

rm -f putty-src.zip

# Add all the text files
find * \( -name rdb -prune -false \) -o -type f ! \(  \
    -name \*.o \
    -o -name \*.exe \
    -o -name \*.map \
    -o -name fuzzterm \
    -o -name pageant \
    -o -name plink \
    -o -name pscp \
    -o -name psftp \
    -o -name pterm \
    -o -name putty \
    -o -name puttygen \
    -o -name puttytel \
    -o -name testbn \
    -o -name \*.ico \
    -o -name \*.iss \
    -o -name \*.url \
    -o -name \*.dsp \
    -o -name \*.dsw \
    -o -name \*.cur \
\) | sort | zip -@ -X -q -l putty-src.zip

# And the binary files.
find * \
       -name \*.ico \
    -o -name \*.iss \
    -o -name \*.url \
    -o -name \*.dsp \
    -o -name \*.cur \
| zip -@ -X -q putty-src.zip
