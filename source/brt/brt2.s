// brt2.s -- loaded at end of executable

.bss
.globl stke

stksiz = 512.	/ stack size for B stack (r5)

stkb: .=.+stksiz / stack, (r5) points into this
stke = .	/ stack end (grows down)
