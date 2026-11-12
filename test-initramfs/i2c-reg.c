// SPDX-License-Identifier: GPL-2.0
/*
 * Čtení a zápis jednoho registru zařízení I2C přes /dev/i2c-N (SMBus byte
 * data). Bez libc, aby šel sestavit toolchainem pro kernel; busybox v
 * testovacím initramfs nástroje i2c* nemá.
 *
 *   i2c-reg BUS ADDR REG          přečte registr a vypíše ho šestnáctkově
 *   i2c-reg BUS ADDR REG w VAL    zapíše registr
 *
 * BUS je desítkově, ostatní šestnáctkově (bez 0x i s ním).
 */

#define SYS_openat	56
#define SYS_ioctl	29
#define SYS_write	64
#define SYS_exit	93

#define AT_FDCWD	(-100)
#define O_RDWR		2

#define I2C_SLAVE	0x0703
#define I2C_SMBUS	0x0720
#define I2C_SMBUS_READ	1
#define I2C_SMBUS_WRITE	0
#define I2C_SMBUS_BYTE_DATA	2

union i2c_smbus_data {
	unsigned char byte;
	unsigned short word;
	unsigned char block[34];
};

struct i2c_smbus_ioctl_data {
	unsigned char read_write;
	unsigned char command;
	unsigned int size;
	union i2c_smbus_data *data;
};

static long sys(long nr, long a, long b, long c, long d)
{
	register long x8 asm("x8") = nr;
	register long x0 asm("x0") = a;
	register long x1 asm("x1") = b;
	register long x2 asm("x2") = c;
	register long x3 asm("x3") = d;

	asm volatile("svc #0" : "+r"(x0) : "r"(x8), "r"(x1), "r"(x2), "r"(x3)
		     : "memory");
	return x0;
}

static void put(const char *s)
{
	long n = 0;

	while (s[n])
		n++;
	sys(SYS_write, 1, (long)s, n, 0);
}

static void die(const char *s)
{
	put(s);
	put("\n");
	sys(SYS_exit, 1, 0, 0, 0);
}

static long num(const char *s, int base)
{
	long v = 0;

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
		if (d >= base)
			die("spatne cislo");
		v = v * base + d;
	}
	return v;
}

void start_c(long *sp)
{
	static const char hex[] = "0123456789abcdef";
	long argc = sp[0];
	char **argv = (char **)(sp + 1);
	char path[] = "/dev/i2c-00";
	char out[] = "0x00\n";
	struct i2c_smbus_ioctl_data args;
	union i2c_smbus_data data;
	long bus, addr, reg, fd;
	int write;

	if (argc != 4 && argc != 6)
		die("pouziti: i2c-reg BUS ADDR REG [w VAL]");
	write = argc == 6;
	if (write && (argv[4][0] != 'w' || argv[4][1]))
		die("pouziti: i2c-reg BUS ADDR REG [w VAL]");

	bus = num(argv[1], 10);
	addr = num(argv[2], 16);
	reg = num(argv[3], 16);
	if (bus > 99 || addr > 0x7f || reg > 0xff)
		die("hodnota mimo rozsah");

	if (bus < 10) {
		path[9] = '0' + bus;
		path[10] = 0;
	} else {
		path[9] = '0' + bus / 10;
		path[10] = '0' + bus % 10;
	}

	fd = sys(SYS_openat, AT_FDCWD, (long)path, O_RDWR, 0);
	if (fd < 0)
		die("nejde otevrit /dev/i2c-N");
	if (sys(SYS_ioctl, fd, I2C_SLAVE, addr, 0) < 0)
		die("I2C_SLAVE selhalo (adresu drzi ovladac?)");

	args.read_write = write ? I2C_SMBUS_WRITE : I2C_SMBUS_READ;
	args.command = reg;
	args.size = I2C_SMBUS_BYTE_DATA;
	args.data = &data;
	if (write) {
		long val = num(argv[5], 16);

		if (val > 0xff)
			die("hodnota mimo rozsah");
		data.byte = val;
	}
	if (sys(SYS_ioctl, fd, I2C_SMBUS, (long)&args, 0) < 0)
		die("prenos selhal (zarizeni neodpovida)");

	if (!write) {
		out[2] = hex[data.byte >> 4];
		out[3] = hex[data.byte & 0xf];
		put(out);
	}
	sys(SYS_exit, 0, 0, 0, 0);
}

asm(".global _start\n"
    "_start:\n"
    "	mov x0, sp\n"
    "	bl start_c\n");
