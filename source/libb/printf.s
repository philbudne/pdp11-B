/ B library - printf

.globl .putcha, .printn, .char
.globl .printf

/ stack:
/ 40	j
/ 36	i
/ 34	c
/ 32	x
/ 30	adx
/ 26	x9
/ 24	x8
/ 22	x7
/ 20	x6
/ 16	x5
/ 14	x4
/ 12	x3
/ 10	x2
/ 6	x1
/ 4	fmt
/ 2	RET
/ 0	FP

/  printf(fmt, x1,x2,x3,x4,x5,x6,x7,x8,x9) {
/  	extrn printn, char, putchar;
/  	auto adx, x, c, i, j;
/
/  	i = 0; /* fmt index */
/  	adx = &x1; /* argument pointer */
/  loop:
/  	while((c=char(fmt,i++)) != '%') {
/  		if(c == '*e')
/  			return;
/  		putchar(c);
/  	}
/  	x = *adx++;
/  	switch c = char(fmt, i++) {
/
/  	case 'd': /* decimal */
/  	case 'o': /* octal */
/  		printn(x, c=='o'?8:10);
/  		goto loop;
/
/  	case 'c': /* char */
/  		putchar(x);
/  		goto loop;
/
/  	case 's': /* string */
/  		j = 0;
/  		while((c=char(x,j++)) != '*e')
/  			putchar(c);
/  		goto loop;
/  	}
/  	putchar('%');
/  	i--;
/  	adx--;
/  	goto loop;
/  }

	/ what's this?
	jmp	9f

.printf:
	.+2
		/ stack size: 42
	s; 42
		/ i = 0
	va; 36
	c; 0
	b1
		/ adx = &x1
	iva; 30
	va; 6
	b1
loop:
l1:
		/ reset stack
	s; 42
		/ c = char(fmt,i++)
	va; 34
	x; .char; n2
	a; 4
	va; 36
	u7
	n3
	b1
		/ if not (c != '%') goto l3
	c; 45	/ '%'
	b5
	f l3
		/ if not (c == 0) goto l4
	ia; 34
	c; 0
	b4
	f l4
		/ return
	n11
l4:
		/ putchar(c)
	ix; .putcha; n2
	a; 34
	n3
		/ while loop
	t; l1
l3:
		/ x = *adx++
	iva; 32
	va; 30
	u7
	u3
	b1
		/ c = char(fmt, i++)
	iva; 34
	x; .char; n2
	a; 4
	va; 36
	u7
	n3
	b1
		/ switch c
	z l5
l7:	/ case %d %o format
		/ printn(x, c == 'o' ? 8 : 10)
	ix; .printn; n2
	a; 32
	a; 34
	c; 157	/ 'o'
	b4
	f; l12
	ic; 10
	t; l13
l12:	ic; 12
l13:	n3
		/ goto loop
	ix; 8f; n6

l15:	/ case %c format
		/ putchar(x)
	ix; .putcha; n2
	a; 32
	n3
		/ goto loop
	ix; 8f; n6

l17:	/ case %s format
		/ j = 0
	iva; 40
	c; 0
	b1
		/ c = char(x, j++)
l20:	iva; 34
	x; .char; n2
	a; 32
	va; 40
	u7
	n3
	b1
		/ if not c != 0 goto l21
	c; 0
	b5
	f; l21
		/ putchar(c)
	ix; .putcha; n2
	a; 34
	n3
	t; l20
l21:
		/ goto loop
	ix; 8f; n6
		/ after switch
	t; 7f

l5:	4
	144; l7		/ d
	157; l7		/ o
	143; l15	/ c
	163; l17	/ s
7:	/ after switch
		/ putchar('%')
	ix; .putchar; n2
	c; 45	/ '%'
	n3
		/ i--
	iva; 36
	u10
		/ adx--
	iva; 30
	u10
		/ goto loop
	ix; 8f; n6
		/ return - not reached
	n11

8:	loop

	/ ???
9:	jsr	r5,chain
	0
