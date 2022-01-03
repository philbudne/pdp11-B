# order preserved from /usr/lib/libb.a

OBJ=	chdir.o chmod.o chown.o close.o creat.o ctime.o execl.o \
	execv.o exit.o fork.o fstat.o getchr.o getuid.o gtty.o intr.o \
	lchar.o link.o makdir.o open.o printf.o printn.o putchr.o \
	quit.o read.o seek.o setuid.o sleep.o stat.o stty.o time.o \
	unlink.o wait.o write.o char.o hog.o

# OBJ for make clean
ALL=	libb.a $(OBJ)

libb.a: $(OBJ)
	rm -f libb.a
	$(AR) r libb.a $(OBJ)

include ../Makefile.inc
