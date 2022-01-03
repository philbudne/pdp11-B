# order preserved from /usr/lib/bilib.a

OBJ=	a.o b1.o b10.o b102.o b103.o b11.o b112.o b113.o b114.o b115.o \
	b116.o b117.o b12.o b120.o b13.o b14.o b15.o b16.o b17.o b2.o \
	b20.o b3.o b4.o	b5.o b6.o b7.o c.o f.o n11.o n123.o n4.o n6.o \
	n7.o s.o t.o u10.o u2.o u3.o u4.o u5.o u6.o u7.o va.o vx.o \
	x.o y.o z.o cksto.o

# OBJ for make clean
ALL=	bilib.a $(OBJ)

bilib.a: $(OBJ)
	rm -f bilib.a
	$(AR) r bilib.a $(OBJ)

include ../Makefile.inc
