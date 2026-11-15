// SPDX-License-Identifier: GPL-2.0
/*
 * Zkouška Bluetooth přes surový soket HCI, bez libc (busybox nemá bluez):
 * zapne hci0, přečte adresu a verzi řadiče, pak SEKUND sekund hledá okolní
 * zařízení (inquiry BR/EDR a pasivní skenování LE) a vypíše je.
 *
 *   hci-scan SEKUND
 *
 * Nic nezapisuje do konfigurace; jen vysílá výzvy hledání. Na konci hci0 vypne.
 */
#define __EXPORTED_HEADERS__
#include <linux/types.h>

#define SYS_ioctl	29
#define SYS_close	57
#define SYS_write	64
#define SYS_read	63
#define SYS_ppoll	73
#define SYS_setsockopt	208
#define SYS_socket	198
#define SYS_bind	200
#define SYS_exit	93

#define AF_BLUETOOTH	31
#define SOCK_RAW	3
#define BTPROTO_HCI	1
#define SOL_HCI		0
#define HCI_FILTER	2
#define HCIDEVUP	0x400448c9UL
#define HCIDEVDOWN	0x400448caUL

static long sys(long nr, long a, long b, long c, long d, long e)
{
	register long x8 asm("x8") = nr;
	register long x0 asm("x0") = a;
	register long x1 asm("x1") = b;
	register long x2 asm("x2") = c;
	register long x3 asm("x3") = d;
	register long x4 asm("x4") = e;

	asm volatile("svc #0" : "+r"(x0) : "r"(x8), "r"(x1), "r"(x2), "r"(x3), "r"(x4) : "memory");
	return x0;
}

static void put(const char *s)
{
	long n = 0;

	while (s[n])
		n++;
	sys(SYS_write, 1, (long)s, n, 0, 0);
}

static void puthex(unsigned long v, int digits)
{
	static const char hex[] = "0123456789abcdef";
	char b[17];
	int i;

	for (i = 0; i < digits; i++)
		b[i] = hex[(v >> ((digits - 1 - i) * 4)) & 15];
	b[digits] = 0;
	put(b);
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

static void put_addr(const unsigned char *a)	/* HCI order: LSB first */
{
	int i;

	for (i = 5; i >= 0; i--) {
		puthex(a[i], 2);
		if (i)
			put(":");
	}
}

struct sockaddr_hci { unsigned short family, dev, channel; };
struct hci_filter { unsigned int type_mask, event_mask[2]; unsigned short opcode; };
struct pollfd { int fd; short events, revents; };
struct timespec { long sec, nsec; };

static long fd;

static void cmd(unsigned short opcode, const unsigned char *par, int plen)
{
	unsigned char b[64];
	int i;

	b[0] = 1;
	b[1] = opcode & 0xff;
	b[2] = opcode >> 8;
	b[3] = plen;
	for (i = 0; i < plen; i++)
		b[4 + i] = par[i];
	sys(SYS_write, fd, (long)b, 4 + plen, 0, 0);
}

/* Read one packet within MS milliseconds; returns its length or 0 */
static long rd(unsigned char *buf, long ms)
{
	struct pollfd p = { (int)fd, 1, 0 };
	struct timespec ts = { ms / 1000, (ms % 1000) * 1000000 };

	if (sys(SYS_ppoll, (long)&p, 1, (long)&ts, 0, 0) <= 0)
		return 0;
	return sys(SYS_read, fd, (long)buf, 260, 0, 0);
}

static unsigned char ev[270];

/* Send CMD and wait for its Command Complete; leaves the packet in ev */
static int cmd_wait(unsigned short opcode, const unsigned char *par, int plen)
{
	int tries;

	cmd(opcode, par, plen);
	for (tries = 0; tries < 20; tries++) {
		long n = rd(ev, 1000);

		if (n < 7)
			continue;
		if (ev[0] == 4 && ev[1] == 0x0e && (ev[4] | ev[5] << 8) == opcode)
			return ev[6];	/* status */
		if (ev[0] == 4 && ev[1] == 0x0f && (ev[5] | ev[6] << 8) == opcode)
			return ev[3] ? -ev[3] : 0;
	}
	return -1000;
}

void __attribute__((noreturn)) c_main(long argc, char **argv)
{
	struct sockaddr_hci a = { AF_BLUETOOTH, 0, 0 };
	struct hci_filter f = { 0xffffffff, { 0xffffffff, 0xffffffff }, 0 };
	unsigned char par[16];
	long secs = 8, n, t, found = 0;
	int st, i;

	if (argc > 1) {
		secs = 0;
		for (i = 0; argv[1][i] >= '0' && argv[1][i] <= '9'; i++)
			secs = secs * 10 + argv[1][i] - '0';
	}
	fd = sys(SYS_socket, AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI, 0, 0);
	if (fd < 0) {
		put("socket HCI selhal: ");
		putnum(fd);
		put("\n");
		sys(SYS_exit, 1, 0, 0, 0, 0);
	}
	st = sys(SYS_ioctl, fd, HCIDEVUP, 0, 0, 0);
	put("HCIDEVUP hci0: ");
	putnum(st);
	put("\n");
	st = sys(SYS_bind, fd, (long)&a, sizeof(a), 0, 0);
	if (st < 0) {
		put("bind selhal: ");
		putnum(st);
		put("\n");
		sys(SYS_exit, 1, 0, 0, 0, 0);
	}
	sys(SYS_setsockopt, fd, SOL_HCI, HCI_FILTER, (long)&f, sizeof(f));

	st = cmd_wait(0x1009, par, 0);	/* Read BD_ADDR */
	put("Read BD_ADDR: stav ");
	putnum(st);
	if (!st) {
		put(" adresa ");
		put_addr(&ev[7]);
	}
	put("\n");
	st = cmd_wait(0x1001, par, 0);	/* Read Local Version */
	put("Read Local Version: stav ");
	putnum(st);
	if (!st) {
		put(" HCI ver ");
		putnum(ev[7]);
		put(" rev ");
		puthex(ev[8] | ev[9] << 8, 4);
		put(" LMP ver ");
		putnum(ev[10]);
		put(" vyrobce ");
		puthex(ev[11] | ev[12] << 8, 4);
	}
	put("\n");

	/* Event mask: all events, LE meta included (Set Event Mask 0x0c01) */
	for (i = 0; i < 8; i++)
		par[i] = 0xff;
	par[7] = 0x3f;
	cmd_wait(0x0c01, par, 8);

	/* LE passive scan: parameters (0x200b) then enable (0x200c) */
	par[0] = 0;			/* passive */
	par[1] = 0x10; par[2] = 0;	/* interval 10 ms */
	par[3] = 0x10; par[4] = 0;	/* window 10 ms */
	par[5] = 0;			/* own address public */
	par[6] = 0;			/* accept all */
	st = cmd_wait(0x200b, par, 7);
	put("LE Set Scan Parameters: ");
	putnum(st);
	par[0] = 1;
	par[1] = 0;
	st = cmd_wait(0x200c, par, 2);
	put("LE Set Scan Enable: ");
	putnum(st);
	put("\n");

	/* BR/EDR inquiry: GIAC, length in 1.28 s units */
	par[0] = 0x33; par[1] = 0x8b; par[2] = 0x9e;
	par[3] = secs * 100 / 128 + 1;
	par[4] = 0;
	cmd(0x0401, par, 5);

	for (t = 0; t < secs * 1000 + 2000;) {
		n = rd(ev, 500);
		t += 500;
		if (n < 3 || ev[0] != 4)
			continue;
		if (ev[1] == 0x02 || ev[1] == 0x22 || ev[1] == 0x2f) {
			put("BR/EDR ");
			put_addr(&ev[4]);
			put("\n");
			found++;
		} else if (ev[1] == 0x3e && ev[3] == 2) {
			put("LE ");
			put_addr(&ev[7]);
			put(ev[6] ? " (nahodna adresa)" : "");
			put("\n");
			found++;
		} else if (ev[1] == 0x01) {
			put("inquiry hotovo\n");
		}
	}
	par[0] = 0;
	par[1] = 0;
	cmd_wait(0x200c, par, 2);
	put("nalezeno udalosti: ");
	putnum(found);
	put("\n");
	sys(SYS_ioctl, fd, HCIDEVDOWN, 0, 0, 0);
	sys(SYS_close, fd, 0, 0, 0, 0);
	sys(SYS_exit, 0, 0, 0, 0, 0);
	for (;;)
		;
}

__asm__(".global _start\n"
	"_start:\n"
	"	ldr x0, [sp]\n"
	"	add x1, sp, #8\n"
	"	bl c_main\n");
