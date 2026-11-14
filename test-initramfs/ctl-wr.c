// SPDX-License-Identifier: GPL-2.0
/*
 * Nastaví jednu celočíselnou nebo přepínací hodnotu ovládacího prvku ALSA.
 * Bez libc. Smí jen prvky ze seznamu v tomto souboru (allow-list), nic jiného
 * nezapíše.
 *
 *   ctl-wr NÁZEV HODNOTA     NÁZEV je přesný název prvku, HODNOTA celé číslo
 */

#define SYS_openat	56
#define SYS_ioctl	29
#define SYS_write	64
#define SYS_exit	93

#define AT_FDCWD	(-100)
#define O_RDONLY	0
#define IOCTL_WRITE	0xC4C85513UL	/* _IOWR('U', 0x13, snd_ctl_elem_value) */

#define IOCTL_LIST	0xC0505510UL	/* _IOWR('U', 0x10, snd_ctl_elem_list) */
#define IOCTL_INFO	0xC1105511UL	/* _IOWR('U', 0x11, snd_ctl_elem_info) */
#define IOCTL_READ	0xC4C85512UL	/* _IOWR('U', 0x12, snd_ctl_elem_value) */

#define ID_SIZE		64
#define MAX_IDS		512
#define TYPE_ENUMERATED	3

static long sys(long nr, long a, long b, long c)
{
	register long x8 asm("x8") = nr;
	register long x0 asm("x0") = a;
	register long x1 asm("x1") = b;
	register long x2 asm("x2") = c;
	register long x3 asm("x3") = 0;

	asm volatile("svc #0" : "+r"(x0) : "r"(x8), "r"(x1), "r"(x2), "r"(x3) : "memory");
	return x0;
}

static unsigned long slen(const char *s)
{
	unsigned long n = 0;

	while (s[n])
		n++;
	return n;
}

static void put(const char *s)
{
	sys(SYS_write, 1, (long)s, slen(s));
}

static void put_num(unsigned long v)
{
	char buf[24];
	int i = 23;

	buf[i] = 0;
	do {
		buf[--i] = '0' + v % 10;
		v /= 10;
	} while (v);
	put(buf + i);
}

static int contains(const char *hay, const char *needle)
{
	unsigned long i, j;

	if (!needle[0])
		return 1;
	for (i = 0; hay[i]; i++) {
		for (j = 0; needle[j] && hay[i + j] == needle[j]; j++)
			;
		if (!needle[j])
			return 1;
	}
	return 0;
}


/* Jen tyto prvky; přidává se po jednom podle potřeby. */
static const char *const allowed[] = {
	"SPK DSP1 Preload Switch",
	"SPK PCM Source",		/* 1 = DSP (protection), 0 = ASP bypasses it */
	"SPK Digital PCM Volume",
	"SPK Analog PCM Volume",
	"AIF2TX1 Input 1 Volume",
	"AIF2TX1 Input 1",		/* 0 = None, 1 = Tone Generator 1 */
	0,
};

static int same(const char *a, const char *b)
{
	while (*a && *a == *b) {
		a++;
		b++;
	}
	return *a == *b;
}

static long parse(const char *s)
{
	long v = 0;

	while (*s >= '0' && *s <= '9')
		v = v * 10 + (*s++ - '0');
	return v;
}

void start_c(long *sp)
{
	unsigned char ids[MAX_IDS][ID_SIZE];
	unsigned long list[10];
	unsigned char val[1224] __attribute__((aligned(8)));
	long argc = sp[0];
	const char *name = argc > 2 ? ((char **)(sp + 1))[1] : "";
	long want = argc > 2 ? parse(((char **)(sp + 1))[2]) : 0;
	unsigned int *l = (unsigned int *)list;
	unsigned int i, total;
	long fd;
	int ok = 0;

	for (i = 0; allowed[i]; i++)
		if (same(allowed[i], name))
			ok = 1;
	if (!ok || argc < 3) {
		put("nepovoleny prvek nebo chybi argumenty\n");
		sys(SYS_exit, 2, 0, 0);
	}

	fd = sys(SYS_openat, AT_FDCWD, (long)"/dev/snd/controlC0", 2);
	if (fd < 0) {
		put("nejde otevrit controlC0\n");
		sys(SYS_exit, 1, 0, 0);
	}
	for (i = 0; i < 10; i++)
		list[i] = 0;
	l[1] = MAX_IDS;
	list[2] = (unsigned long)ids;
	if (sys(SYS_ioctl, fd, IOCTL_LIST, (long)list) < 0) {
		put("ioctl LIST selhal\n");
		sys(SYS_exit, 1, 0, 0);
	}
	total = l[3];
	for (i = 0; i < l[2] && i < total; i++) {
		unsigned long k;

		if (!same((const char *)ids[i] + 16, name))
			continue;
		for (k = 0; k < sizeof(val); k++)
			val[k] = 0;
		for (k = 0; k < ID_SIZE; k++)
			val[k] = ids[i][k];
		if (sys(SYS_ioctl, fd, IOCTL_READ, (long)val) < 0) {
			put("cteni selhalo\n");
			sys(SYS_exit, 1, 0, 0);
		}
		put("pred: ");
		put_num(*(long *)(val + 72));
		*(long *)(val + 72) = want;
		if (sys(SYS_ioctl, fd, IOCTL_WRITE, (long)val) < 0) {
			put("\nzapis selhal\n");
			sys(SYS_exit, 1, 0, 0);
		}
		put(" po: ");
		put_num(want);
		put("\n");
		sys(SYS_exit, 0, 0, 0);
	}
	put("prvek nenalezen\n");
	sys(SYS_exit, 1, 0, 0);
}

asm(".global _start\n"
    "_start:\n"
    "	mov x0, sp\n"
    "	bl start_c\n");
