/*
 * reboot-arg ARGUMENT: restartuje telefon s argumentem pro kernel
 * (reboot(2), LINUX_REBOOT_CMD_RESTART2), např. "bootloader". Busybox
 * argument předat neumí. Bez libc, jen pro arm64.
 */
	.text
	.globl	_start
_start:
	ldr	x9, [sp]		/* argc */
	cmp	x9, #2
	b.lt	1f
	ldr	x3, [sp, #16]		/* argv[1] */
	movz	x0, #0xdead		/* LINUX_REBOOT_MAGIC1 0xfee1dead */
	movk	x0, #0xfee1, lsl #16
	movz	x1, #0x1969		/* LINUX_REBOOT_MAGIC2 0x28121969 */
	movk	x1, #0x2812, lsl #16
	movz	x2, #0xc3d4		/* LINUX_REBOOT_CMD_RESTART2 0xa1b2c3d4 */
	movk	x2, #0xa1b2, lsl #16
	mov	x8, #142		/* __NR_reboot */
	svc	#0
1:	mov	x0, #2
	mov	x8, #93			/* __NR_exit */
	svc	#0
