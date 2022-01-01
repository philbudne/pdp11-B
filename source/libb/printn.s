/ B library - printn

.globl	.printn

.globl	.putcha

.globl	a, b1, b14, b16, b20, b7, c, chain, f, ia, iva, ix
.globl	n11, n2, n3, s, t, u10, u2, va


/ stack:
/ 12	d
/ 10	a
/ 6	b
/ 4	n
/ 2	RET
/ 0	FP

/ printn(n, b) {
/ 	d = 0;
/ 	if(n < 0) {
/ 		n = -n;
/ 		if(n < 0) {
/ 			n--;
/ 			d = 1;
/ 		} else
/ 			putchar('-');
/ 	}
/ 	if(a = n/b)
/ 		printn(a, b);
/	putchar(n%b + '0' + d);
/ }

	/ what's this?
	jmp	9f

.printn:
	.+2
		/ stack size: 14
	s; 14
		/ d = 0
	va; 12
	c; 0
	b1
		/ if not (n < 0) goto l1
	ia; 4
	c; 0
	b7
	f; l1
		/ n = -n
	iva; 4
	a; 4
	u2
	b1
		/ if not (n < 0) goto l2
	ia; 4
	c; 0
	b7
	f; l2
		/ n--
	iva; 4
	u10
		/ d = 1
	iva; 12
	c; 1
	b1
		/ goto l3
	t; l3

l2:	/ n < 0
		/ putchar('-')
	ix; .putcha
	n2
	c; 55	/ '-'
	n3
l3:
l1:	/ n >= 0
		/ a = n / b
	iva; 10
	a; 4
	a; 6
	b20
	b1
		/ if 0; goto l4
	f; l4
	ix; .printn
	n2
	a; 4
	a; 6
	n3
l4:
		/ putchar(n%b + '0' + d)
	ix; .putcha
	n2
	a; 4
	a; 6
	b16
	c; 60	/ '0'
	b14
	a; 12
	b14
	n3
		/ return
	n11


	/ ???
9:	jsr	r5,chain
	0
