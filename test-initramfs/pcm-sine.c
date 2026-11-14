// SPDX-License-Identifier: GPL-2.0
/*
 * Přehraje sinus 1 kHz (48 kHz, stereo, S16_LE) na PCM zařízení ALSA, bez libc.
 *
 *   pcm-sine ZAŘÍZENÍ AMPLITUDA SEKUNDY
 *
 * Příklad: pcm-sine /dev/snd/pcmC0D1p 8000 8
 * AMPLITUDA je 0 až 8000 (z 32767). Nic jiného nedělá: nastaví parametry,
 * zapíše vzorky, počká na dohrání a skončí.
 */
#define __EXPORTED_HEADERS__
#include <linux/ioctl.h>
#include <sound/asound.h>

#define SYS_openat	56
#define SYS_close	57
#define SYS_ioctl	29
#define SYS_write	64
#define SYS_exit	93
#define AT_FDCWD	(-100)
#define O_RDWR		2

#define RATE		48000
#define PERIOD		960
#define PERIODS		4

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

void *memset(void *d, int c, unsigned long n)
{
	volatile unsigned char *p = d;

	while (n--)
		*p++ = c;
	return d;
}

static void put(const char *s)
{
	long n = 0;

	while (s[n])
		n++;
	sys(SYS_write, 1, (long)s, n);
}

static void putnum(long v)
{
	char b[24];
	int i = 23;

	b[i] = 0;
	if (v < 0) {
		put("-");
		v = -v;
	}
	do {
		b[--i] = '0' + v % 10;
		v /= 10;
	} while (v);
	put(&b[i]);
}

static long num(const char *s)
{
	long v = 0;

	while (*s >= '0' && *s <= '9')
		v = v * 10 + (*s++ - '0');
	return v;
}

static struct snd_pcm_hw_params hw;
static struct snd_pcm_sw_params sw;

static void fix_interval(struct snd_interval *i, unsigned int v)
{
	i->min = i->max = v;
	i->openmin = i->openmax = 0;
	i->integer = 1;
	i->empty = 0;
}

static const short sine[48] = {
	0, 4276, 8480, 12539, 16383, 19948, 23170, 25996, 28378, 30273,
	31650, 32487, 32767, 32487, 31650, 30273, 28378, 25996, 23170,
	19948, 16383, 12539, 8480, 4276, 0, -4276, -8480, -12539, -16383,
	-19948, -23170, -25996, -28378, -30273, -31650, -32487, -32767,
	-32487, -31650, -30273, -28378, -25996, -23170, -19948, -16383,
	-12539, -8480, -4276,
};

static short buf[PERIOD * 2];

void __attribute__((noreturn)) c_main(long argc, char **argv)
{
	long fd, amp, secs, r, i, n, p;

	if (argc < 4) {
		put("použití: pcm-sine ZAŘÍZENÍ AMPLITUDA SEKUNDY\n");
		sys(SYS_exit, 2, 0, 0);
	}
	amp = num(argv[2]);
	secs = num(argv[3]);
	if (amp > 8000 || secs < 1 || secs > 30) {
		put("amplituda 0 až 8000, sekundy 1 až 30\n");
		sys(SYS_exit, 2, 0, 0);
	}
	fd = sys(SYS_openat, AT_FDCWD, (long)argv[1], O_RDWR);
	if (fd < 0) {
		put("open selhal: ");
		putnum(fd);
		put("\n");
		sys(SYS_exit, 1, 0, 0);
	}

	for (i = 0; i < SNDRV_PCM_HW_PARAM_LAST_MASK - SNDRV_PCM_HW_PARAM_FIRST_MASK + 1; i++) {
		hw.masks[i].bits[0] = ~0U;
		hw.masks[i].bits[1] = ~0U;
	}
	for (i = 0; i < SNDRV_PCM_HW_PARAM_LAST_INTERVAL - SNDRV_PCM_HW_PARAM_FIRST_INTERVAL + 1; i++) {
		hw.intervals[i].min = 0;
		hw.intervals[i].max = ~0U;
	}
	hw.rmask = ~0U;
	hw.masks[SNDRV_PCM_HW_PARAM_ACCESS - SNDRV_PCM_HW_PARAM_FIRST_MASK].bits[0] =
		1U << SNDRV_PCM_ACCESS_RW_INTERLEAVED;
	hw.masks[SNDRV_PCM_HW_PARAM_FORMAT - SNDRV_PCM_HW_PARAM_FIRST_MASK].bits[0] =
		1U << SNDRV_PCM_FORMAT_S16_LE;
	hw.masks[SNDRV_PCM_HW_PARAM_SUBFORMAT - SNDRV_PCM_HW_PARAM_FIRST_MASK].bits[0] = 1U;
	fix_interval(&hw.intervals[SNDRV_PCM_HW_PARAM_CHANNELS - SNDRV_PCM_HW_PARAM_FIRST_INTERVAL], 2);
	fix_interval(&hw.intervals[SNDRV_PCM_HW_PARAM_RATE - SNDRV_PCM_HW_PARAM_FIRST_INTERVAL], RATE);
	fix_interval(&hw.intervals[SNDRV_PCM_HW_PARAM_PERIOD_SIZE - SNDRV_PCM_HW_PARAM_FIRST_INTERVAL], PERIOD);
	fix_interval(&hw.intervals[SNDRV_PCM_HW_PARAM_PERIODS - SNDRV_PCM_HW_PARAM_FIRST_INTERVAL], PERIODS);
	r = sys(SYS_ioctl, fd, SNDRV_PCM_IOCTL_HW_PARAMS, (long)&hw);
	if (r < 0) {
		put("HW_PARAMS selhal: ");
		putnum(r);
		put("\n");
		sys(SYS_exit, 1, 0, 0);
	}

	sw.tstamp_mode = SNDRV_PCM_TSTAMP_NONE;
	sw.period_step = 1;
	sw.avail_min = PERIOD;
	sw.start_threshold = PERIOD * PERIODS;
	sw.stop_threshold = 0x7fffffff;
	r = sys(SYS_ioctl, fd, SNDRV_PCM_IOCTL_SW_PARAMS, (long)&sw);
	if (r < 0) {
		put("SW_PARAMS selhal: ");
		putnum(r);
		put("\n");
		sys(SYS_exit, 1, 0, 0);
	}
	r = sys(SYS_ioctl, fd, SNDRV_PCM_IOCTL_PREPARE, 0);
	if (r < 0) {
		put("PREPARE selhal: ");
		putnum(r);
		put("\n");
		sys(SYS_exit, 1, 0, 0);
	}

	p = 0;
	for (n = 0; n < secs * RATE / PERIOD; n++) {
		for (i = 0; i < PERIOD; i++) {
			short v = (short)((long)sine[p] * amp / 32767);

			p = (p + 1) % 48;
			buf[2 * i] = buf[2 * i + 1] = v;
		}
		r = sys(SYS_write, fd, (long)buf, sizeof(buf));
		if (r < 0) {
			put("write selhal: ");
			putnum(r);
			put(" po ");
			putnum(n);
			put(" periodách\n");
			break;
		}
	}
	sys(SYS_ioctl, fd, SNDRV_PCM_IOCTL_DRAIN, 0);
	sys(SYS_close, fd, 0, 0);
	put("hotovo\n");
	sys(SYS_exit, 0, 0, 0);
	for (;;)
		;
}

__asm__(".global _start\n"
	"_start:\n"
	"	ldr x0, [sp]\n"
	"	add x1, sp, #8\n"
	"	bl c_main\n");
