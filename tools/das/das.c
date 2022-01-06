// from http://squoze.net/B/tools/das.c (Angelo Papenhoff)
// mangled by Phil Budne for B
// assumes V2/V3 era environment, other B-based assumptions!!
// (look for CROCK)

// XXX TODO?
// branch instructions not getting symbols
// dump sys args as data!!
// attempt symbol lookup in dw (need segment?)
// assume loaded at 16K??
// generate "tAAA" labels (needs two passes)

/* WARNING: assumes machine is little endian */

/*
 * das: disassemble pdp-11 a.out file
 *      it could do more, but it's all I needed
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

typedef u_int16_t word;
typedef u_int8_t byte;

/* symbol type in a.out namelist */
#define N_TYPE 037
#define  T_UND 0
#define  T_ABS 01
#define  T_TXT 02
#define  T_DATA 03
#define  T_BSS 04
#define N_EXT 040

void
sysfatal0(char *str) {
    fprintf(stderr, "%s\n", str);
    exit(1);
}

void
sysfatal1(char *fmt, char *arg) {
    fprintf(stderr, fmt, arg);
    fputc('\n', stderr);
    exit(1);
}
//end PLB

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
	None = 0,
	Binary,
	Unary,
	RegBinary,
	Reg,
	Imm,
	Sys,
	Br,
	Sob,
	Unk,

	Byte = 01000,
};

struct instdef
{
	char *name;
	word w, m;
	int type;
} instdefs[] = {
	{ "..",		0040000, 0177777, None }, /* CROCK! (bic r0, r0) */
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
	{ "mark",	0006400, 0177700, Imm },
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
	{ "emt",	0104000, 0177400, Imm },
	{ "sys",	0104400, 0177400, Sys }, // (trap instr)
	{ "jmp",	0000100, 0177700, Unary },
	{ "rts",	0000200, 0177770, Reg },
	{ "spl",	0000230, 0177770, Imm },
	{ "ccc",	0000240, 0177760, Unk }, // TODO
	{ "scc",	0000260, 0177760, Unk }, // TODO
	{ "swab",	0000300, 0177700, Unary },
	{ "halt",	0000000, 0177777, None },
	{ "wait",	0000001, 0177777, None },
	{ "rti",	0000002, 0177777, None },
	{ "bpt",	0000003, 0177777, None },
	{ "iot",	0000004, 0177777, None },
	{ "reset",	0000005, 0177777, None },
	{ "rtt",	0000006, 0177777, None },
	{ NULL, 0, 0, 0 }
};

// NOTE!! V2 syscalls are all that matter for extant B library
struct syscall {
    int args;
    char *name;
} syscalls[] = {
	{ 0, "rele" },		/* 0. (not in assembler; indir in v4 (1 arg) */
	{ 0, "exit" },		/* 1. status in r0 */
	{ 0, "fork" },		/* 2. */
	{ 2, "read" },		/* 3. (fd in r0) */
	{ 2, "write" },		/* 4. (fd in r0) */
	{ 2, "open" },		/* 5. */
	{ 0, "close" },		/* 6. (fd in r0) */
	{ 0, "wait" },		/* 7. */
	{ 2, "creat" },		/* 8. */
	{ 2, "link" },		/* 9. */
	{ 1, "unlink" },	/* 10. */
	{ 2, "exec" },		/* 11. */
	{ 1, "chdir" },		/* 12. */
	{ 0, "time" },		/* 13. */
	{ 2, "makdir" },	/* 14.  (renamed to mknod w/ 3 args in v4) */
	{ 2, "chmod" },		/* 15. */
	{ 2, "chown" },		/* 16. (3 args in v7) */
	{ 1, "break" },		/* 17. */
	{ 2, "stat" },		/* 18. */
	{ 2, "seek" },		/* 19. (fd in r0; 3 args (lseek) in v7) */
	{ 2, "tell" },		/* 20. (fd in r0, never implemented?) getpid in v7 */
	{ 2, "mount" },		/* 21. */
	{ 1, "umount" },	/* 22. */
	{ 0, "setuid" },	/* 23. (uid in r0) */
	{ 0, "getuid" },	/* 24. */
	{ 0, "stime" },		/* 25. (time in AC-MQ) */
	{ 1, "quit" },		/* 26. (ptrace in v6) */
	{ 1, "intr" },		/* 27. (nosys in v6; alarm in v7) */
	{ 1, "fstat" },		/* 28. (fd in r0) */
	{ 1, "cemt" },		/* 29. (nosys v6; pause in v7) */
	{ 1, "mdate" },		/* 30. (date in AC-MQ); utime in v7; added in PWB?? */
	{ 1, "stty" },		/* 31. (fd in r0) */
	{ 1, "gtty" },		/* 32. (fd in r0) */
	{ 1, "ilgins" },	/* 33. (nosys in v6; access in v7) */
	{ 0, "hog" },		/* 34. (renamed to nice in v3) */
	{ 0, "sleep" },		/* 35. (60ths in r0, NIAS; ftime in v7) */
	{ 0, "sync" },		/* 36. (not in assembler) */
	{ 0, "kill" },		/* 37. pid in r0 */
	/* new in 3rd Edition manual: */
	{ 0, "csw" },		/* 38. (not in assembler) */
	{ 0, "boot"},		/* 39. (not in assembler); "setpgrp" in v7 */
	{ 1, "fpe"},		/* 40. (NIAS; nosys in v6; v7 says was "tell"?!) */
	{ 0, "dup" },		/* 41. (fd in r0) */
	/* new in 4th Edition manual: */
	{ 0, "pipe" },		/* 42. v4 (noop in nsys) */
	{ 1, "times" },		/* 43. v4 */
	{ 4, "profil" },	/* 44. v4 (not in assembler; noop in nsys) */
	{ 0, "tiu" },		/* 45. (name from nsys/v6)
				   getfp in v7m
				   CB 2.1 syscb: umask=1, reboot=2, lock=3,
					sprofil=4, ucore=5, getcsw=?
				   lock in svr1)
				*/
	{ 0, "setgid" },	/* 46. v4 (not in assembler) */
	{ 0, "getgid" },	/* 47. v4 (not in assembler)*/
	{ 2, "signal" },	/* 48. v4 */
	/* from v7 sysent.c: */
	/* 49 = reserved for USG (msg in CB 2.1; SVR1 IPC Messages)
	/* 50 = reserved for USG (Reserved for local use in SVR1) */
	/* 51 = acct (1 arg) */
	/* 52 = phys (4 args); SVR1 shm (in CB 2.3?) */
	/* 53 = lock (1 arg); SVR1 sem */
	/* 54 = ioctl (3 args) */
	/* 55 = readwrite ("in abeyance"); reboot in 4.1BSD */
	/* 56 = mpxcall (2 args) */
	/* 57 = reserved for USG
		PWB: "pwbsys" (0=uname, 1=udata, 2=ustat)
		CB 2.1 "cbsys": uname=0; 
		"uts" in SVR1 (uname, ustat); in Sys III?) */
	/* 58 = reserved for USG: {get,free,dis,swit}maus in CB 2.1 */
	/*		"multi-access user space"; dedicated portion of memory */
	/*		swap functions in SVR2; "local functions" in 2.9BSD */
	/* 59 = exece (3 args) */
	/* 60 = umask (1 arg) */
	/* 61 = chroot (1 arg) */
	/* 62 = SVR1 fcntl (in CB-2.1, Sys III); v7m ttlocal (2 args) */
	/*	"reserved to local sites" in 4.1BSD */
	/* 63 = Sys III: ulimit; v7m getfp (2 args) [prproc in "nsys"!] */
	/*	semas in CB 2.3, "used internally" in v7m, 4.1BSD */
};
#define NSYS (sizeof(syscalls)/sizeof(syscalls[0]))

struct instdef*
getinst(word inst)
{
	struct instdef *i;
	for(i = instdefs; i->name; i++)
		if((inst & i->m) == i->w)
			return i;
	return NULL;
}

struct sym*
findsym(word type, word val)
{
	struct sym *s;
	for(s = symtab; s < &symtab[nsym]; s++)
		if((s->type&7) == (type&7) && s->val == val)
			return s;
	return NULL;
}

// fetch and format an instruction operand, or non-instruction word
char*
fetch(int pcidx)
{
	static char buf[32];
	char *s = buf;
	word w = mem[addr/2];
	word r = 0;
	word rt = 0;
	struct sym *sp;
	short off;
	word oaddr = addr;

	if (reloc) {
	    r = reloc[addr/2];
	    rt = r & 016;
	}
	off = w;			// signed
	addr += 2;

	if(pcidx) {
		w = addr + off;
		if (r == 01) // abs, pc relative
			w += 040000;	/* XXX CROCK */
		off = 0;
	}
	if(rt) {
		if(rt == 010) {			// ext
			s += sprintf(buf, "%.8s", symtab[r>>4].name);
			if(off > 0)
				*s++ = '+';
			if(off != 0)
				s += sprintf(s, "%o", off);
		}
		else { // rt != 010
			struct sym *sp;
			// libb.a and bilib.a only have text;
			if(r == 2 && w >= 040000)
				w -= 040000;
			if((rt == 2 || rt == 4 || rt == 6) &&
			   (sp = findsym((rt>>1)+1, w))) {
				s += sprintf(s, "%.8s", sp->name);
			}
			else { // no symbol
				off = w;
				switch (rt) {
				case 2:		// text
					if (w - addr < 5) {
						s += sprintf(s, ".");
						off = w - oaddr;
					}
					else
						s += sprintf(s, "TEXT");
					break;
				case 4:		// data
					s += sprintf(s, "DATA");
					break;
				case 6:		// bss
					s += sprintf(s, "BSS");
					break;
				default:
					s += sprintf(s, "RT%#o", rt);
					break;
				}

				if (off > 0)
					s += sprintf(s, "+%o", off);
				else if (off < 0)
					s += sprintf(s, "-%o", -off);
			} // no symbol
		} // rt != 010
	} // rt != 0
	else {			// no relocation (absolute)
		if (w == 0177300)
			s += sprintf(s, "DIV");
		else if (w == 0177302)
			s += sprintf(s, "AC");
		else if (w == 0177304)
			s += sprintf(s, "MQ");
		else if (w == 0177314)
			s += sprintf(s, "LSH");
		else
			s += sprintf(s, "%o", w);
	}
	return buf;
}

// format mode/register operand in m into s
// (can be called with just register)
// returns updated pointer
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
		s += sprintf(s, "%s", rstr[r]);
		break;
	case 1:
		s += sprintf(s, "(%s)", rstr[r]);
		break;
	case 2:
	case 3:
		if(r == 7){
			s += sprintf(s, "$%s", fetch(0));
		}else
			s += sprintf(s, "(%s)+", rstr[r]);
		break;
	case 4:
	case 5:
		s += sprintf(s, "-(%s)", rstr[r]);
		break;
	case 6:
	case 7:
		if(r == 7)
			s += sprintf(s, "%s", fetch(1));
		else
			s += sprintf(s, "%s(%s)", fetch(0), rstr[r]);
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
	short broff;
	word braddr;
	struct sym *sp;

	memset(line, 0, sizeof(line));

	if (inst < 0100) {		/* CROCK! */
		// compiled B code operand
		sprintf(line, "%o", inst);
		return line;
	}
	def = getinst(inst);
	if(def == NULL)
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
			broff = (inst|0177400)<<1;
		else
			broff = (inst&0377)<<1;
		*s++ = '\t';
	branch:
		braddr = addr - 2 + broff;

		// never happens
		sp = findsym(02, braddr);
		if (sp)
			s += sprintf(s, "%.8s", sp->name);
		else {
#if 0
			// XXX generate local label??
			s += sprintf(s, ".");
			broff += 2;	// account for instruction
			if (broff >= 0)
				s += sprintf(s, "+%o", broff);
			else
				s += sprintf(s, "-%o", -broff);
#else
			s += sprintf(s, "TEXT+%o", braddr+2);
#endif
		}
		break;
	case Imm:
		// emt, trap, mark: low 6 bits
		// spl: low 3 bits
		s += sprintf(s, "\t%o", inst & ~def->m);
		break;
	case Sob:
		*s++ = '\t';
		s = disop(s, inst>>6 & 07);	// register
		*s++ = ',';
		// positive offset for backwards branch
		braddr = - (inst&077)<<1;
		goto branch;
	case Sys:
		inst &= 0377;
		if (inst > NSYS)
			s += sprintf(s, "\t%o", inst);
		else {
			s += sprintf(s, "\t%s", syscalls[inst].name);
			// XXX dump syscalls[inst].args words as data
		}
		break;
	case Unk:
	default:
		s += sprintf(s, "\t???");
	case None:
		break;
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
	s += sprintf(s, "%2o ", r & 017);
	if(r == 010)
		sprintf(s, "%.8s", symtab[r>>4].name);
	return buf;
}

void
dw(word a, word w, word r, word seg) {
    // XXX target column for comment
    // XXX output using fetch()?
    unsigned char hi = w >> 8;
    unsigned char lo = w & 0377;
    struct sym *s = findsym(seg, w);	/* word value as symbol */

    printf("\t\t/ %06o: %06o (%s) %d. %#o %#o %c%c %-10s\n",
	   a, w,
	   (s ? s->name : ""),		/* symbol (in same segment) */
	   (short)w,			/* signed decimal */
	   lo, hi,			/* octal bytes */
	   (lo >= ' ' && lo <= '~') ? lo : '.', /* ASCII char */
	   (hi >= ' ' && hi <= '~') ? hi : '.', /* ASCII char */
	   getreloc(r));		/* external sym ref */
}

void
textdump(void)
{
	word a, w, r = 0;
	struct sym *s;

	if (!aout.textsz)
		return;
	printf(".text\n");
	addr = 0;
	while(addr < aout.textsz){
		word oa = a = addr;
		s = findsym(02, a);
		w = mem[a/2];
		if (reloc)
			r = reloc[a/2];
		if(s)
			printf("%.8s:\n", s->name);

		if(r) {	// relocation data? not an instruction.
			printf("\t%s", fetch(0)); // address
		}
		else {
			addr += 2;
			printf("\t%s", dis(w)); /* may advance addr */
		}
		a += 2;

		dw(oa, w, r, 02);

		// instruction operands
		while(a < addr){
			w = mem[a/2];
			if (reloc)
				r = reloc[a/2];
			dw(a, w, r, 02);
			a += 2;
		}
	}
}

void
datadump(void)
{
	word a, w, r;
	struct sym *s;

	if (!aout.datasz)
		return;
	printf(".data\n");
	while(addr < memsz){
		w = mem[addr/2];
		s = findsym(03, addr);
		if (reloc)
			r = reloc[addr/2];
		// XXX make comment
		dw(addr, w, r, 03);
		if(s)
			printf("%.8s:\n", s->name);
		printf("\t%s\n", fetch(0));
	}
}

char
symtype(int type)
{
	int tt = type & N_TYPE;
	char c;
	// 05 is common region (internal use only?)
	if (tt <= T_BSS)
		c = "uatdb"[tt];
	else if (tt == 024)
		c = 'r';		/* v6 reg? */
	else if (tt == 037)
		c = 'f';		/* file */
	else
		return '?';
	if (type & N_EXT)		/* extern? */
		c ^= 040;		/* upper case */
	return c;
}

static int
compare_nlist_entry(const void *a, const void *b) {
    return ((struct sym *)a)->val - ((struct sym *)b)->val;
}

// NOTE!! reorders symtab (do it after initial symbol dump??) */
void
bssdump() {
	word prev = 0;
	struct sym *s;
	if (!aout.bsssz)
		return;
	printf(".bss\n");
	qsort(symtab, nsym, sizeof(struct sym),
	      compare_nlist_entry);
	for(s = symtab; s < &symtab[nsym]; s++) {
		if ((s->type & N_TYPE) != T_BSS)
			continue;
		if (s->val > prev) {
			printf("\t.=.+%o\n", s->val - prev);
			prev = s->val;
		}
		if (s->type & N_EXT)
			printf(".globl %.8s\n", s->name);
		printf("%.8s:\n", s->name);
	}
	if (aout.bsssz > prev)
		printf("\t.=.+%o\n", aout.bsssz - prev);

}

void
symdump(void)
{
	struct sym *s;
	for(s = symtab; s < &symtab[nsym]; s++) {
		if (s->type & N_EXT)
			printf(".globl %-8.8s /", s->name);
		else
			printf("/      %-8.8s  ", s->name);
		printf(" %c %#o\n", symtype(s->type), s->val);
	}
}

void
usage(void)
{
	fprintf(stderr, "usage: das a.out\n");
	exit(1);
}

void
main(int argc, char *argv[])
{
	struct sym *s;
	int fd;

	if(argc < 2)
		usage();
	fd = open(argv[1], O_RDONLY);
	if(fd < 0)
		sysfatal1("cannot open %s", argv[1]);
	read(fd, &aout, 16);
	printf("/ %#o %u. %u. %u. %u. %#o %u. %u\n",
		aout.magic,
		aout.textsz, aout.datasz,
		aout.bsssz, aout.symsz,
		aout.entry, aout.stacksz, aout.flag);
	memsz = aout.textsz + aout.datasz;
	mem = malloc(memsz);
	read(fd, mem, memsz);
	if(aout.flag == 0){
		reloc = malloc(memsz);
		read(fd, reloc, memsz);
	}
	if(aout.symsz % 12)
		sysfatal0("botch in symbol table");
	nsym = aout.symsz / 12;
	symtab = malloc(nsym*sizeof(*symtab));
	for(s = symtab; s < &symtab[nsym]; s++)
		read(fd, s, 12);
	close(fd);

	symdump();
	textdump();
	datadump();
	bssdump();

	exit(0);
}
