#!/bin/sh

PROG=`basename $1 .b`
shift

APOUT=../../tools/apout/apout

export APOUT_UNIX_VERSION=V2
export APOUT_ROOT=../../fs/root
BIN=$APOUT_ROOT/bin

USRLIB=../../fs/usr/lib

# get libb.a from fs/usr/lib
# fs/usr not located inside fs/root, so cannot use -lb
# (could also use ../libb/libb.a)
LIBB=$USRLIB/libb.a

# get bilib.a from fs/usr/lib
# (could also use ../bilib/libb.a)
BILIB=$USRLIB/bilib.a

# substitute B runtime files
BRT1=../brt/brt1.o
BRT2=../brt/brt2.o

./b711 $PROG.b | awk -f post.awk > $PROG.s

$APOUT $BIN/as $PROG.s
mv a.out $PROG.o

$APOUT $BIN/ld $PROG.o $BRT1 $USRLIB/libb.a $USRLIB/bilib.a $BRT2
mv a.out $PROG

#DEBUG=-inst
$APOUT $DEBUG ./$PROG $*
