/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * `dfu` shell command: reboot into the STM32 ROM DFU bootloader without
 * touching BOOT0. A magic value survives the warm reset in noinit RAM; the
 * reset hook runs before kernel init, sees it, and jumps into system memory
 * exactly as if BOOT0 had been held. The chip then waits in DFU until
 * flashed or reset, so the USB cable can be moved to the host at leisure.
 */

#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/toolchain.h>

#define RDD2_DFU_MAGIC 0xB00710ADu
/* STM32F4 system memory: the ROM bootloader's vector table. */
#define RDD2_SYSMEM_BASE 0x1FFF0000u

static uint32_t __noinit g_dfu_magic;

/* Runs from the reset vector before RAM sections are initialized: only the
 * noinit magic and locals may be touched here. */
void soc_reset_hook(void)
{
	if (g_dfu_magic != RDD2_DFU_MAGIC) {
		return;
	}
	g_dfu_magic = 0U;

	__asm__ volatile("msr msp, %0\n"
			 "bx %1\n"
			 :
			 : "r"(*(volatile uint32_t *)RDD2_SYSMEM_BASE),
			   "r"(*(volatile uint32_t *)(RDD2_SYSMEM_BASE + 4U)));
	CODE_UNREACHABLE;
}

static int cmd_dfu(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "rebooting into the ROM DFU bootloader; flash over USB-C");
	k_msleep(100);
	g_dfu_magic = RDD2_DFU_MAGIC;
	sys_reboot(SYS_REBOOT_WARM);
	return 0;
}

SHELL_CMD_REGISTER(dfu, NULL, "Reboot into the STM32 ROM DFU bootloader.", cmd_dfu);
