// SPDX-License-Identifier: GPL-2.0
/*
 * Měření rychlosti jádra bez libc: připne se na zadané jádro a po dobu 1 s
 * počítá smyčky podle čítače ARM (cntvct_el0, frekvenci čte z cntfrq_el0).
 * Nic nezapisuje.
 *
 *   cpu-bench JÁDRO [FREKVENCE_ČÍTAČE]   vypíše: tisíce smyček za sekundu
 *
 * Na malých jádrech A53 firmware čítač cntfrq_el0 nenastavil (čte se 0), proto
 * jde frekvenci čítače (26000000) zadat druhým argumentem.
 */
#define SYS_write		64
#define SYS_exit		93
#define SYS_sched_setaffinity	122

static long sys(long nr, long a, long b, long c)
{
	register long x8 asm("x8") = nr;
	register long x0 asm("x0") = a;
	register long x1 asm("x1") = b;
	register long x2 asm("x2") = c;

	asm volatile("svc #0" : "+r"(x0) : "r"(x8), "r"(x1), "r"(x2) : "memory");
	return x0;
}

static void putnum(unsigned long v)
{
	char b[24];
	int i = 22;

	b[23] = '\n';
	do {
		b[i--] = '0' + v % 10;
		v /= 10;
	} while (v);
	sys(SYS_write, 1, (long)&b[i + 1], 23 - i);
}

void __attribute__((noreturn)) c_main(long argc, char **argv)
{
	unsigned long freq, t0, t, n = 0, mask = 1;
	int cpu = 0;

	if (argc > 1)
		cpu = argv[1][0] - '0';
	mask <<= cpu;
	sys(SYS_sched_setaffinity, 0, sizeof(mask), (long)&mask);
	asm volatile("mrs %0, cntfrq_el0" : "=r"(freq));
	if (argc > 2) {
		char *a = argv[2];

		freq = 0;
		while (*a >= '0' && *a <= '9')
			freq = freq * 10 + *a++ - '0';
	}
	asm volatile("mrs %0, cntvct_el0" : "=r"(t0));
	do {
		unsigned long i;

		for (i = 0; i < 1000; i++)
			asm volatile("" : "+r"(n));
		n += 1000;
		asm volatile("mrs %0, cntvct_el0" : "=r"(t));
	} while (t - t0 < freq);
	putnum(n / 1000);
	sys(SYS_exit, 0, 0, 0);
	for (;;)
		;
}

__asm__(".global _start\n"
	"_start:\n"
	"	ldr x0, [sp]\n"
	"	add x1, sp, #8\n"
	"	bl c_main\n");
