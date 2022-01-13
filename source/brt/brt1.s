// brt1.s -- PDP-11 B startup code
// Phil Budne January 2021

// based on Ken Thompson's PDP-11 B manual:
// https://www.bell-labs.com/usr/dmr/www/kbman.html
// and work by Angelo Pappenhof @ http://squoze.net/B/
// and Robert Swierczek https://github.com/rswier/pdp11-B.git

// extern interpreter routines (bilib.a)
.globl x, n1, ix

// extern data in brt2.s
.globl stke

// user B code
.globl .main

// B library (libb.a)
.globl .exit

// In Ken Thompson's manual says B programs are loaded with:
//	ld object /etc/brt1 -lb /etc/bilib /etc/brt2

// So this file is fallen into by the "tail" of the last file of
// compiled B code.  See discussion in the chain subroutine (currently
// at the end of this file).

// set up argv
.globl .argv
	mov	sp,r0		// get pointer to [argc, args...]
	mov	(r0)+,r1	// get argc
	beq	2f

// make argv entries into word pointers
// PLB NOTE! had to patch apout to put strings at even addrs!!
// have (as yet) been unable to see what "unix72" kernel does
1:	asr	(r0)+		// make word addr
	dec	r1
	bne	1b

// make a word pointer to argv (argc is first entry)
2:	mov	sp,r0		// get argv pointer
	asr	r0		// make word addr
	mov	r0,.argv

// initialize B execution environment:
// r0, r1 scratch
// r2	points to EAE (as noted by Angelo P.)
// r3	interpreter program counter
// r4	display pointer
// r5	interpreter stack pointer

	mov	$MQ,r2		// pointer into EAE regs
	mov	$start,r3	// bootstrap code
	clr	r4		// display pointer

// perhaps just init to sp + N? only libb/ctime.s uses sp.
	mov	$stke, r5	// B stack pointer end (grows down)
	jmp	*(r3)+

// boostrap code
start:	x; .main
	n1
	ix; .exit
	n1

// The B runtime expects word addresses for all data.
// This requires the first word of vectors (including string literals)
// to be be shifted/divided, but the only load-time "fixups" available
// in a.out format are adding an offset to an external symbol.
// To work around this the runtime has to patch the addresses of all
// such addresses at runtime before calling into B code (main). The
// way this is done is -- according to Steve Johnson -- a marvellous
// hack by Dennis Ritchie. The first instruction in each object file
// jumps to the end of the file and there calls a subroutine chain,
// whose job it is to patch all the addresses of that object
// file. After the return it falls off the end of the file and into
// the next one, which does its patching the same way.

// NOTE! libb.a and bilib.a are loaded AFTER this file, so any B files
// (printn.o printf.o) are not included in the chain, but they don't
// require fixups.

// PLB: Currently outputting a list of addresseses of pointers to
// words to patch (each with a pN label).  The extra pointer might
// be avoided by writing the "tail" to a temp file with the global
// labels referencing generated labels for the actual (vector) storage.
// ie; append to the temp file:
//   .label: 1f
//   .data
//   1: initializers and/or .=.+2*N
//   .text
// BUT this requires delaying outputting the label until it's
// it's known to be the name of a vector,
// and for string lits generate "x; s0" and append to the temp file:
//   s0: 1f
//   .data
//   1: <hello world\n\e>
//   .text

.globl chain
chain:	mov	(r5)+,r0	// fetch pointer pointer
	beq	1f		// quit on zero word
	asr	(r0)		// adjust the referenced word
	br	chain

1:	rts	r5		// return to end of file, fall into next
