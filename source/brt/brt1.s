// brt1.s -- PDP-11 B startup code
// Phil Budne January 2021

// based on Thompson PDP-11 B manual:
// https://www.bell-labs.com/usr/dmr/www/kbman.html
// and work by Angelo Pappenhof	http://squoze.net/B/
// and Robert Swierczek https://github.com/rswier/pdp11-B.git

// extern interpreter routines (bilib.a)
.globl ix, mcall

// extern data in brt2.s
.globl stke

// user B code
.globl .main

// B library (libb.a)
.globl .exit

// Thompson says B programs are loaded with:
//	ld object /etc/brt1 -lb /etc/bilib /etc/brt2

// So this file is fallen into by the "tail" of the last file of compiled B code.
// See discussion in the chain subroutine (currently at the end of this file).

// initialize B execution environment:
// r0, r1 scratch
// r2	points to EAE
// r3	interpreter program counter
// r4	display pointer
// r5	interpreter stack pointer

	mov	$MQ,r2		// pointer into EAE regs
	mov	$start,r3
	clr	r4		// display pointer
	mov	$stke, r5	// B stack pointer end (grows down)

	mov	sp,.argv	// set up arg vector
	jmp	*(r3)+

start:	ix; .main
	mcall

	ix; .exit
	mcall

// here on error opening files. say something??
// .exit routine makes no effort to return status,
// so not bothering here either.
3:	sys	exit

// PLB: user B code is loaded first, and the head of each file jumps
// to the tail, which calls "jsr r5,chain" (to turn byte addresses
// into word addresses?).
//
// No examples of actual data exist (printn.o and printf.o in libb.a
// have only a zero word after the call (an end of list sentinal?)).
// So supply the global label, skip the data, and return, to fall into
// the start of the next B file, if any, and eventually to the start
// of this file which calls the user main routine.

.globl chain

chain:	tst	(r5)+
	bne	chain
	rts	r5

.globl .argv
.argv:	.=.+1
