// SPDX-License-Identifier: GPL-2.0
/*
 * Přečte hodnoty ovládacích prvků ALSA (jen čtení, ioctl SNDRV_CTL_IOCTL_*).
 * Bez libc, aby šel sestavit toolchainem pro kernel; na telefonu s Androidem
 * není tinymix ani amixer.
 *
 *   ctl-rd [HLEDANÝ_TEXT] [items]  vypíše prvky karty 0, jejichž název obsahuje text
 *                           (bez textu všechny), u výčtových i vybranou položku;
 *                           s druhým argumentem i všechny položky výčtu
 */

#define SYS_openat	56
#define SYS_ioctl	29
#define SYS_write	64
#define SYS_exit	93

#define AT_FDCWD	(-100)
#define O_RDONLY	0

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

void start_c(long *sp)
{
	/* On the stack: a .bss segment without file backing does not load here */
	unsigned char ids[MAX_IDS][ID_SIZE];
	unsigned long list[10];		/* snd_ctl_elem_list, 80 bytes */
	unsigned char info[272] __attribute__((aligned(8)));
	unsigned char val[1224] __attribute__((aligned(8)));
	long argc = sp[0];
	const char *want = argc > 1 ? ((char **)(sp + 1))[1] : "";
	long fd;
	unsigned int *l = (unsigned int *)list;
	unsigned int i, total;

	fd = sys(SYS_openat, AT_FDCWD, (long)"/dev/snd/controlC0", O_RDONLY);
	if (fd < 0) {
		put("nejde otevrit controlC0\n");
		sys(SYS_exit, 1, 0, 0);
	}

	for (i = 0; i < 10; i++)
		list[i] = 0;
	l[0] = 0;			/* offset */
	l[1] = MAX_IDS;			/* space */
	list[2] = (unsigned long)ids;	/* pids */
	if (sys(SYS_ioctl, fd, IOCTL_LIST, (long)list) < 0) {
		put("ioctl LIST selhal\n");
		sys(SYS_exit, 1, 0, 0);
	}
	total = l[3];			/* count */

	for (i = 0; i < l[2] && i < total; i++) {
		const char *name = (const char *)ids[i] + 16;
		unsigned long k;
		unsigned int type;

		if (!contains(name, want))
			continue;
		for (k = 0; k < sizeof(info); k++)
			info[k] = 0;
		for (k = 0; k < ID_SIZE; k++)
			info[k] = ids[i][k];
		if (sys(SYS_ioctl, fd, IOCTL_INFO, (long)info) < 0)
			continue;
		type = *(unsigned int *)(info + 64);
		for (k = 0; k < sizeof(val); k++)
			val[k] = 0;
		for (k = 0; k < ID_SIZE; k++)
			val[k] = ids[i][k];
		if (sys(SYS_ioctl, fd, IOCTL_READ, (long)val) < 0)
			continue;
		put(name);
		put(" = ");
		if (type == TYPE_ENUMERATED) {
			unsigned int item = *(unsigned int *)(val + 72);
			unsigned int items = *(unsigned int *)(info + 80);

			/* Název položky: nastavit item a znovu zavolat INFO */
			*(unsigned int *)(info + 84) = item;
			put_num(item);
			put(" of ");
			put_num(items);
			if (sys(SYS_ioctl, fd, IOCTL_INFO, (long)info) >= 0) {
				put(" (");
				put((const char *)(info + 88));
				put(")");
			}
			if (argc > 2) {
				unsigned int it;

				put("\n");
				for (it = 0; it < items; it++) {
					*(unsigned int *)(info + 84) = it;
					if (sys(SYS_ioctl, fd, IOCTL_INFO, (long)info) < 0)
						continue;
					put("  ");
					put_num(it);
					put(" ");
					put((const char *)(info + 88));
					put("\n");
				}
			}
		} else {
			put_num(*(long *)(val + 72));
		}
		put("\n");
	}
	sys(SYS_exit, 0, 0, 0);
}

asm(".global _start\n"
    "_start:\n"
    "	mov x0, sp\n"
    "	bl start_c\n");
