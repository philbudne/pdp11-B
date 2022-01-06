// brt2.s -- loaded at end of executable

.bss
.globl .argv
.argv:	.=.+2


// maybe put B stack below "sp" stack
// (ie; subtract some number from r6???)
stksiz = 512.	/ stack size for B stack (r5)

stkb: .=.+stksiz / B stack
.globl stke
stke = .	/ stack end (grows down)
