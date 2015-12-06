#!/bin/ksh

TMP=/tmp/mk_x
[ "$1" = "" ] && LOCAL=0.66-rdb

[ -e ${TMP}1 -o -e ${TMP}2 -o -e ${TMP}3 -o -e ${TMP}4 -o -e ${TMP}5 -o -e x.ed ] &&
    { echo Temp file exists ; exit 1; }

: > x.ed
j=0.45
echo 0i > x.ed
expand PuTTY-Codes-$j.txt >>  x.ed
echo . >>  x.ed

for i in 0.49 0.50 0.52 0.53 0.54 0.58 0.63 ${LOCAL}
do
    GL="`git log -1 $i --format=$i\ --\ %ai 2>/dev/null`"
    if [ "$GL" != "" ]
    then echo "VERSION $GL"
    else echo "VERSION $i"
    fi

    x="$i"
    [ "$x" = "0.66-rdb" ] && x=RDB

    expand PuTTY-Codes-$j.txt |
    awk > ${TMP}1 -v i=$x \
	'{
	    str=$0
	    if (str != "") str=substr($0 "                                                                                ",1,76) i
	    print str;
	}'

    expand PuTTY-Codes-$i.txt |
    awk > ${TMP}2 -v i=$x \
	'{
	    str=$0
	    if (str != "") str=substr($0 "                                                                                ",1,76) i
	    print str;
	}'

    diff -e ${TMP}1 ${TMP}2 >>  x.ed

    diff -e PuTTY-Codes-$i.txt PuTTY-Codes-$j.txt | grep '^..........' > ${TMP}5
    if [ -s ${TMP}5 ]
    then
	{
	    echo
	    echo BEFORE VERSION $i
	    cat ${TMP}5
	    [ -f ${TMP}3 ] && cat ${TMP}3
	} > ${TMP}4
	mv ${TMP}4 ${TMP}3
    fi

    j=$i 
done

echo 1i >> x.ed
cat >> x.ed <<\!
ANSI and VTxxx codes understood by PuTTY

    The basic emulation for PuTTY is that of an eight bit VT102 with
    cherry picked features from other ANSI terminals. As such it only
    understands seven bit ECMA-35 but adds onto that numerous 8 bit
    "codepages" and UTF-8.

PuTTY Releases

!
for i in 0.45 0.49 0.50 0.52 0.53 0.54 0.58 0.63 ; do echo '    '$i `git log -1 $i --format=%ai | sed  's/..:.*//'` ; done >> x.ed
echo  >> x.ed
echo . >> x.ed

echo wq PuTTY-Codes.txt >> x.ed
ed < x.ed

{
    echo 
    echo Items changed from previous versions
    echo ------------------------------------
    cat ${TMP}3 
} >> PuTTY-Codes.txt

rm ${TMP}1 ${TMP}2 ${TMP}3 ${TMP}5 x.ed
