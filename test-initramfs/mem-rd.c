// SPDX-License-Identifier: GPL-2.0
/*
 * Čtení 32bitových registrů přes /dev/mem (mmap). Bez libc, aby šel sestavit
 * toolchainem pro kernel; busybox v testovacím initramfs devmem nemá a čtení
 * /dev/mem přes read() na arm64 pro registry nefunguje.
 *
 *   mem-rd ADDR [COUNT]    vypíše COUNT slov (výchozí 1) od adresy ADDR
 *
 * ADDR je šestnáctkově (bez 0x i s ním) a musí být násobek 4, COUNT desítkově,
 * nejvýš 64 a v rámci jedné stránky. Soubor se otevírá jen pro čtení a mapuje
 * se jen PROT_READ: nástroj zapisovat neumí.
 */

#define SYS_openat	56
#define SYS_write	64
#define SYS_exit	93
#define SYS_mmap	222

#define AT_FDCWD	(-100)
#define O_RDONLY	0

#define PROT_READ	1
#define MAP_SHARED	1

#define PAGE		0x1000UL

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
	unsigned long addr, count = 1, off, i;
	volatile unsigned int *reg;
	long fd, map;

	if (argc != 2 && argc != 3)
		die("pouziti: mem-rd ADDR [COUNT]");

	addr = num(argv[1], 16);
	if (argc == 3)
		count = num(argv[2], 10);
	off = addr & (PAGE - 1);
	if (addr & 3 || !count || count > 64 || off + count * 4 > PAGE)
		die("hodnota mimo rozsah");

	fd = sys(SYS_openat, AT_FDCWD, (long)"/dev/mem", O_RDONLY, 0, 0, 0);
	if (fd < 0)
		die("nejde otevrit /dev/mem");
	map = sys(SYS_mmap, 0, PAGE, PROT_READ, MAP_SHARED, fd, addr - off);
	if (map < 0 && map > -4096)
		die("mmap selhal");

	reg = (volatile unsigned int *)(map + off);
	for (i = 0; i < count; i++) {
		put_hex(addr + i * 4, 8);
		put(": ");
		put_hex(reg[i], 8);
		put("\n");
	}
	sys(SYS_exit, 0, 0, 0, 0, 0, 0);
}

asm(".global _start\n"
    "_start:\n"
    "	mov x0, sp\n"
    "	bl start_c\n");
