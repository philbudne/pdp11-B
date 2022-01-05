// brt2.s -- loaded at end of executable

.bss
.globl stke

// maybe put B stack below "sp" stack
// (ie; subtract some number from r6???)
stksiz = 512.	/ stack size for B stack (r5)

stkb: .=.+stksiz / stack, (r5) points into this
stke = .	/ stack end (grows down)
