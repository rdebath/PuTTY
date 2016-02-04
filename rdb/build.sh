#!/bin/sh
#
# This will build any of the tagged versions of PuTTY from the first
# available in the GIT repository. For ALL versions it builds a Windows
# executable without the SSH code, including those that were never
# released as a puttytel.exe. Some minor patches are needed to build
# earlier versions with a modern cross-gcc, they are provide for the
# actual tagged releases but may not apply on other commits.

set -e

main() {
    VER=`git describe --abbrev=0 --tags "$1"`

    echo Building "`git describe --tags \"$1\"`"

    if [ -f "rdb/No_SSH-$VER.dif" ]
    then cp -p "rdb/No_SSH-$VER.dif" No_SSH-patch.dif
    else [ -f No_SSH-patch.dif ] && rm -f No_SSH-patch.dif
    fi

    gitempty # Clear the workspace

    git checkout --detach "$1"

    [ -f No_SSH-patch.dif ] && {
	echo Patching "$VER"
	patch -p1 -N -r - --no-backup-if-mismatch < "No_SSH-patch.dif" ||:
	rm -f No_SSH-patch.dif
    }

    build_putty

    gitempty -f # Clear the workspace

    git checkout @{-3}	# Back where we started.
}

build_putty() {
LINKOK=yes
W=

DEFS="-D_WINDOWS"
DEFS="$DEFS -D_WIN32_IE=0x0500"
DEFS="$DEFS -DWINVER=0x0500 -D_WIN32_WINDOWS=0x0410 -D_WIN32_WINNT=0x0500"

RCDEFS="--define WIN32=1 --define _WIN32=1 --define WINVER=0x0400"
LDFLAGS="-static -fstack-protector -Wl,--nxcompat -Wl,--dynamicbase -s"
CFLAGS="-std=gnu90 -w -O2 -mwindows"

LIBS="-ladvapi32 -luser32 -lgdi32 -lwsock32 -lcomctl32 -lcomdlg32"

CFLAGS="$CFLAGS -I`pwd`"
[ -d windows ] && { W=windows/ ; CFLAGS="$CFLAGS -I`pwd`/windows" ; }

SRC="${W}window.c ${W}windlg.c terminal.c misc.c"

[ -f be_nossh.c ] || {
    make_stubs
    SRC="$SRC old_putty_stubs.c"
}

[ -f unicode.c ] && LIBS="$LIBS -lwinmm -limm32"
[ -f printing.c ] && LIBS="$LIBS -lwinspool"
[ -f ${W}winucs.c ] && LIBS="$LIBS -lwinmm -limm32"
[ -f ${W}winprint.c ] && LIBS="$LIBS -lwinspool"

[ -f ${W}wingss.c ] && DEFS="$DEFS -DNO_GSSAPI -DSECURITY_WIN32"
[ -f ${W}winjump.c ] && LIBS="$LIBS -lshell32 -lole32"

RCSRC=${W}win_res.rc
[ -f ${W}puttytel.rc ] && RCSRC=${W}puttytel.rc
[ -f ${W}version.rc2 ] && RCDEFS="$RCDEFS -I."
OBJ=

for f in \
    be_nossh.c telnet.c rlogin.c raw.c winser.c sercfg.c \
    sizetip.c ldisc.c ldiscucs.c xlat.c settings.c winstore.c \
    winnet.c winctrls.c dialog.c tree234.c unicode.c \
    logging.c wcwidth.c cmdline.c printing.c \
    proxy.c pproxy.c nocproxy.c \
    winutils.c winmisc.c wincfg.c config.c windefs.c \
    timing.c pinger.c minibidi.c time.c \
    winucs.c winprint.c winhandl.c winhelp.c winjump.c \
    conf.c callback.c miscucs.c

do [ -f $f ] && SRC="$SRC $f"
   [ "$W" != "" -a -f $W$f ] && SRC="$SRC ${W}$f"
done

lg() { echo "$*" ; "$@" || LINKOK=no ; }

for p in $SRC
do
    b="`basename \"$p\" .c`"

    lg i686-w64-mingw32-gcc \
	$CFLAGS $DEFS \
	-c $p -o $b.o

    OBJ="$OBJ $b.o"
done

[ -f ${RCSRC} ] || { echo "Resource file '${RCSRC}' missing" ; exit 1; }
lg i686-w64-mingw32-windres $RCDEFS ${RCSRC} -o win_res.o
OBJ="$OBJ win_res.o"

[ -f version.c ] && {
    lg i686-w64-mingw32-gcc -DRELEASE=`git describe --tags` $DEFS version.c -c -o version.o
    OBJ="$OBJ version.o"
}

[ -f old_putty_stubs.c ] && rm old_putty_stubs.c

[ "$LINKOK" = no ] && { echo 'Build failed' ; exit 1; }

lg i686-w64-mingw32-gcc \
    $CFLAGS $DEFS $LDFLAGS \
    -o putty-`git describe --tags`.exe \
    $OBJ \
    $LIBS

# i686-w64-mingw32-size $OBJ | tee putty-`git describe --tags`.log

rm -f $OBJ ||:

[ "$LINKOK" = no ] && { echo 'Build failed' ; exit 1; }

lg i686-w64-mingw32-strip putty-`git describe --tags`.exe

[ "$LINKOK" = no ] && { echo 'Build failed' ; exit 1; }

return 0
}

gitempty() {
git checkout "$@" \
$(echo "tree $(git hash-object -t tree -w /dev/null)
author nobody <> 1 +0000
committer nobody <> 1 +0000

 
" | git hash-object -t commit -w --stdin )
}

make_stubs() {

cat > old_putty_stubs.c <<\!
#include <windows.h>
#include "ssh.h"

void noise_ultralight(DWORD data) { }
void random_save_seed(void) { }
int rsastr_len(struct RSAKey *key) { return 42; }
void rsastr_fmt(char *str, struct RSAKey *key) { strcpy(str, "No KEY"); }
!
}

main "$@"
