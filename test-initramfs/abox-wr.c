// SPDX-License-Identifier: GPL-2.0
/*
 * Zápis jednoho registru zvukové jednotky ABOX přes /dev/mem (pokusy se
 * zvukem bez firmwaru, FINDINGS.md 31). Bez libc jako mem-rd.
 *
 *   abox-wr OFFSET VALUE     zapíše VALUE do registru 0x14a50000 + OFFSET
 *
 * Obě čísla šestnáctkově. Zapisovat jde JEN do registrů ze seznamu níže:
 * napájení a směrování ABOXu, směšovač přehrávání, časovač 0, rozhraní UAIF0
 * a kanál přehrávání RDMA1.
 * Jinou adresu nástroj odmítne. Po zápisu registr přečte a vypíše.
 */

#define SYS_openat	56
#define SYS_write	64
#define SYS_exit	93
#define SYS_mmap	222

#define AT_FDCWD	(-100)
#define O_RDWR		2
#define O_SYNC		0x101000

#define PROT_READ	1
#define PROT_WRITE	2
#define MAP_SHARED	1

#define PAGE		0x1000UL
#define ABOX_BASE	0x14a50000UL	/* reg "sfr" uzlu abox v původním DT */

/* Jména podle abox.h původního kernelu */
static const unsigned short allowed[] = {
	0x0010,	/* ABOX_SYSPOWER_CTRL */
	0x0030,	/* ABOX_DYN_CLOCK_OFF */
	0x0038,	/* ABOX_QCHANNEL_DISABLE */
	0x0040,	/* ABOX_ROUTE_CTRL0 */
	0x0050,	/* ABOX_TICK_DIV_RATIO */
	0x0200,	/* ABOX_SPUS_CTRL0 */
	0x0204,	/* ABOX_SPUS_CTRL1: formát směšovače, volba výstupů */
	0x0204,	/* ABOX_SPUS_CTRL1 */
	0x0500,	/* ABOX_UAIF_CTRL0(0) */
	0x0504,	/* ABOX_UAIF_CTRL1(0) */
	0x0604,	/* ABOX_TIMER_CTRL1(0) */
	0x1100,	/* RDMA1: ABOX_RDMA_CTRL0 */
	0x1104,	/* RDMA1: ABOX_RDMA_CTRL1 */
	0x1108,	/* RDMA1: ABOX_RDMA_BUF_STR */
	0x110c,	/* RDMA1: ABOX_RDMA_BUF_END */
	0x1110,	/* RDMA1: ABOX_RDMA_BUF_OFFSET */
	0x1114,	/* RDMA1: ABOX_RDMA_STR_POINT */
	0x1118,	/* RDMA1: ABOX_RDMA_VOL_FACTOR */
	0x111c,	/* RDMA1: ABOX_RDMA_VOL_CHANGE */
};

static long sys(long nr, long a, long b, long c, long d, long e, long f)
{
	register long x8 asm("x8") = nr;
	register long x0 asm("x0") = a;
	register long x1 asm("x1") = b;
	register long x2 asm("x2") = c;
	register long x3 asm("x3") = d;
	register long x4 asm("x4") = e;
	register long x5 asm("x5") = f;

	asm volatile("svc #0" : "+r"(x0) : "r"(x8), "r"(x1), "r"(x2), "r"(x3),
		     "r"(x4), "r"(x5) : "memory");
	return x0;
}

static void put(const char *s)
{
	long n = 0;

	while (s[n])
		n++;
	sys(SYS_write, 1, (long)s, n, 0, 0, 0);
}

static void die(const char *s)
{
	put(s);
	put("\n");
	sys(SYS_exit, 1, 0, 0, 0, 0, 0);
}

static unsigned long num(const char *s, int base)
{
	unsigned long v = 0;
	int n = 0;

	if (base == 16 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
		s += 2;
	if (!*s)
		die("prazdne cislo");
	for (; *s; s++) {
		int d;

		if (*s >= '0' && *s <= '9')
			d = *s - '0';
		else if (*s >= 'a' && *s <= 'f')
			d = *s - 'a' + 10;
		else if (*s >= 'A' && *s <= 'F')
			d = *s - 'A' + 10;
		else
			d = 99;
		if (d >= base || ++n > 12)
			die("spatne cislo");
		v = v * base + d;
	}
	return v;
}

static void put_hex(unsigned long v, int digits)
{
	static const char hex[] = "0123456789abcdef";
	char buf[19] = "0x";
	int i;

	for (i = 0; i < digits; i++)
		buf[2 + i] = hex[(v >> ((digits - 1 - i) * 4)) & 0xf];
	buf[2 + digits] = 0;
	put(buf);
}

void start_c(long *sp)
{
	long argc = sp[0];
	char **argv = (char **)(sp + 1);
	unsigned long off, val, i, ok = 0;
	volatile unsigned int *reg;
	long fd, map;

	if (argc != 3)
		die("pouziti: abox-wr OFFSET VALUE");

	off = num(argv[1], 16);
	val = num(argv[2], 16);
	for (i = 0; i < sizeof(allowed) / sizeof(allowed[0]); i++)
		if (off == allowed[i])
			ok = 1;
	if (!ok || val > 0xffffffffUL)
		die("registr neni na seznamu povolenych");

	fd = sys(SYS_openat, AT_FDCWD, (long)"/dev/mem", O_RDWR | O_SYNC, 0, 0, 0);
	if (fd < 0)
		die("nejde otevrit /dev/mem");
	map = sys(SYS_mmap, 0, PAGE, PROT_READ | PROT_WRITE, MAP_SHARED, fd,
		  ABOX_BASE + (off & ~(PAGE - 1)));
	if (map < 0 && map > -4096)
		die("mmap selhal");

	reg = (volatile unsigned int *)(map + (off & (PAGE - 1)));
	*reg = (unsigned int)val;
	put_hex(ABOX_BASE + off, 8);
	put(": ");
	put_hex(*reg, 8);
	put("\n");
	sys(SYS_exit, 0, 0, 0, 0, 0, 0);
}

asm(".global _start\n"
    "_start:\n"
    "	mov x0, sp\n"
    "	bl start_c\n");
