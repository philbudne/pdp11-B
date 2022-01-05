/* WARNING: assumes machine is little endian */

/*
 * das: disassemble pdp-11 a.out file
 *      it could do more, but it's all I needed
 */

#include <u.h>
#include <libc.h>

typedef u16int word;
typedef u8int byte;

#define W(x) ((x) & 0177777)

struct aout
{
	word magic;
	word textsz;
	word datasz;
	word bsssz;
	word symsz;
	word entry;	/* 0 */
	word stacksz;	/* 0 */
	word flag;
};
struct aout aout;

struct sym
{
	char name[8];
	word type;
	word val;
};
struct sym *symtab;
int nsym;

word *mem;
word *reloc;
int memsz;

int addr;

enum	/* Instruction type */
{
	Binary,
	Unary,
	RegBinary,
	Reg,
	Trap,
	Br,
	Sob,

	Byte = 01000,
};

struct instdef
{
	char *name;
	word w, m;
	int type;
} instdefs[] = {
	{ "mov",	0010000, 0070000, Binary|Byte },
	{ "cmp",	0020000, 0070000, Binary|Byte },
	{ "bit",	0030000, 0070000, Binary|Byte },
	{ "bic",	0040000, 0070000, Binary|Byte },
	{ "bis",	0050000, 0070000, Binary|Byte },
	{ "add",	0060000, 0170000, Binary },
	{ "sub",	0160000, 0170000, Binary },
	{ "clr",	0005000, 0077700, Unary|Byte },
	{ "com",	0005100, 0077700, Unary|Byte },
	{ "inc",	0005200, 0077700, Unary|Byte },
	{ "dec",	0005300, 0077700, Unary|Byte },
	{ "neg",	0005400, 0077700, Unary|Byte },
	{ "adc",	0005500, 0077700, Unary|Byte },
	{ "sbc",	0005600, 0077700, Unary|Byte },
	{ "tst",	0005700, 0077700, Unary|Byte },
	{ "ror",	0006000, 0077700, Unary|Byte },
	{ "rol",	0006100, 0077700, Unary|Byte },
	{ "asr",	0006200, 0077700, Unary|Byte },
	{ "asl",	0006300, 0077700, Unary|Byte },
	{ "mark",	0006400, 0177700, 0 },		// TODO
	{ "mfpi",	0006500, 0177700, Unary },
	{ "mfpd",	0106500, 0177700, Unary },
	{ "mtpi",	0006600, 0177700, Unary },
	{ "mtpd",	0106600, 0177700, Unary },
	{ "sxt",	0006700, 0177700, Unary },
	{ "mul",	0070000, 0177000, RegBinary },
	{ "div",	0071000, 0177000, RegBinary },
	{ "ash",	0072000, 0177000, RegBinary },
	{ "ashc",	0073000, 0177000, RegBinary },
	{ "xor",	0074000, 0177000, RegBinary },
	{ "sob",	0077000, 0177000, Sob },
	{ "br",		0000400, 0177400, Br },
	{ "bne",	0001000, 0177400, Br },
	{ "beq",	0001400, 0177400, Br },
	{ "bge",	0002000, 0177400, Br },
	{ "blt",	0002400, 0177400, Br },
	{ "bgt",	0003000, 0177400, Br },
	{ "ble",	0003400, 0177400, Br },
	{ "bpl",	0100000, 0177400, Br },
	{ "bmi",	0100400, 0177400, Br },
	{ "bhi",	0101000, 0177400, Br },
	{ "blos",	0101400, 0177400, Br },
	{ "bvc",	0102000, 0177400, Br },
	{ "bvs",	0102400, 0177400, Br },
	{ "bcc",	0103000, 0177400, Br },
	{ "bcs",	0103400, 0177400, Br },
	{ "jsr",	0004000, 0177000, RegBinary },
	{ "emt",	0104000, 0177400, Trap },
	{ "trap",	0104400, 0177400, Trap },
	{ "jmp",	0000100, 0177700, Unary },
	{ "rts",	0000200, 0177770, Reg },
	{ "spl",	0000230, 0177770, 0 },		// TODO
	{ "ccc",	0000240, 0177760, 0 },		// TODO
	{ "scc",	0000260, 0177760, 0 },		// TODO
	{ "swab",	0000300, 0177700, Unary },
	{ "halt",	0000000, 0177777, 0 },
	{ "wait",	0000001, 0177777, 0 },
	{ "rti",	0000002, 0177777, 0 },
	{ "bpt",	0000003, 0177777, 0 },
	{ "iot",	0000004, 0177777, 0 },
	{ "reset",	0000005, 0177777, 0 },
	{ "rtt",	0000006, 0177777, 0 },
	{ nil, 0, 0, 0 }
};

struct instdef*
getinst(word inst)
{
	struct instdef *i;
	for(i = instdefs; i->name; i++)
		if((inst & i->m) == i->w)
			return i;
	return nil;
}

struct sym*
findsym(word type, word val)
{
	struct sym *s;
	for(s = symtab; s < &symtab[nsym]; s++)
		if((s->type&7) == (type&7) && s->val == val)
			return s;
	return nil;
}

word
fetch(void)
{
	word w;
	w = mem[addr/2];
	addr += 2;
	return w;
}

char*
disop(char *s, int m)
{
	static char *rstr[8] = {
		"r0", "r1", "r2", "r3",
		"r4", "r5", "sp", "pc"
	};
	int r;
	word w;

	r = m & 7;
	m >>= 3;
	if(m != 1 && m & 1)
		*s++ = '*';
	switch(m & 7){
	case 0:
		s += sprint(s, "%s", rstr[r]);
		break;
	case 1:
		s += sprint(s, "(%s)", rstr[r]);
		break;
	case 2:
	case 3:
		if(r == 7){
			w = fetch();
			s += sprint(s, "$%o", w);
		}else
			s += sprint(s, "(%s)+", rstr[r]);
		break;
	case 4:
	case 5:
//		if(r == 7)
//			addr -= 2;
		s += sprint(s, "-(%s)", rstr[r]);
		break;
	case 6:
	case 7:
		w = fetch();
		if(r == 7)
			s += sprint(s, "%o", W(w + addr));
		else if(w & 0100000)
			s += sprint(s, "-%o(%s)", W(~w+1), rstr[r]);
		else
			s += sprint(s, "%o(%s)", w, rstr[r]);
		break;
	}
	return s;
}

char*
dis(word inst)
{
	static char line[128];
	char *s;
	struct instdef *def;

	memset(line, 0, sizeof(line));
	def = getinst(inst);
	if(def == nil)
		return line;
	s = line;
	strcpy(s, def->name);
	s += strlen(def->name);
	if(inst & 0100000 && def->type & Byte)
		*s++ = 'b';
	switch(def->type & ~Byte){
	case Binary:
		*s++ = '\t';
		s = disop(s, inst>>6 & 077);
		*s++ = ',';
		s = disop(s, inst & 077);
		break;
	case Unary:
		*s++ = '\t';
		s = disop(s, inst & 077);
		break;
	case RegBinary:
		*s++ = '\t';
		s = disop(s, inst>>6 & 07);
		*s++ = ',';
		s = disop(s, inst & 077);
		break;
	case Reg:
		*s++ = '\t';
		s = disop(s, inst & 07);
		break;
	case Br:
		if(inst & 0200)
			s += sprint(s, "\t%o", W(addr + ((inst|0177400)<<1)));
		else
			s += sprint(s, "\t%o", W(addr + ((inst&0377)<<1)));
		break;
	case Trap:
			s += sprint(s, "\t%o", inst&0377);
		break;
	case Sob:
		;// TODO
	}
	*s = '\0';
	return line;
}

char*
getreloc(word r)
{
	static char buf[32];
	char *s;

	s = buf;
	memset(buf, '\0', 32);
	s += sprint(s, "%2o ", r & 017);
	if(r & 010)
		sprint(s, "%.8s", symtab[r>>4].name);
	return buf;
}

void
textdump(void)
{
	word a, w, r;
	struct sym *s;

	addr = 0;
	while(addr < memsz){
		a = addr;
		s = findsym(02, a);
		w = mem[a/2];
		r = reloc[a/2];
		addr += 2;
		print("%06o: %06o %-10s ", a, w, getreloc(r));
		if(s)
			print("%.8s:\t", s->name);
		else
			print("\t");
		print("%s\n", dis(w));
		a += 2;
		while(a < addr){
			w = mem[a/2];
			r = reloc[a/2];
			print("%06o: %06o %-10s\n", a, w, getreloc(r));
			a += 2;
		}
	}
}

void
symdump(void)
{
	struct sym *s;
	for(s = symtab; s < &symtab[nsym]; s++)
		print("%.8s %o %o\n", s->name, s->type, s->val);
}

void
usage(void)
{
	fprint(2, "usage: %s a.out\n");
	exits("usage");
}

void
main(int argc, char *argv[])
{
	struct sym *s;
	int fd;

	if(argc < 2)
		usage();
	fd = open(argv[1], OREAD);
	if(fd < 0)
		sysfatal("cannot open %s", argv[1]);
	read(fd, &aout, 16);
	print("%o. %o %o %o. %o. %o %o %o\n",
		aout.magic,
		aout.textsz, aout.datasz, aout.bsssz,
		aout.symsz,
		aout.entry, aout.stacksz, aout.flag);
	memsz = aout.textsz + aout.datasz;
	mem = malloc(memsz);
	read(fd, mem, memsz);
	if(aout.flag == 0){
		reloc = malloc(memsz);
		read(fd, reloc, memsz);
	}
	if(aout.symsz % 12)
		sysfatal("botch in symbol table");
	nsym = aout.symsz / 12;
	symtab = malloc(nsym*sizeof(*symtab));
	for(s = symtab; s < &symtab[nsym]; s++)
		read(fd, s, 12);
	close(fd);

	symdump();
	textdump();

	exits(nil);
}
