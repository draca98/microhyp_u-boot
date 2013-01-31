/*
 * Copyright (c) 2011 Samsung Electronics Co., Ltd.
 *
 * "SMC CALL COMMAND"
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <common.h>
#include <config.h>
#include <asm/arch/cpu.h>

#define MOVI_BLKSIZE		(512)

/* partition information - size in bytes */
#define PART_SIZE_BL1		(15 * 1024)
#define PART_SIZE_BL2		(16 * 1024)
#define PART_SIZE_UBOOT		(349 * 1024) /* Value suggested by Insignal */
#define PART_SIZE_TZSW		(156 * 1024)

/* partition information - size in sectors */
#define MOVI_BL1_BLKCNT		(PART_SIZE_BL1 / MOVI_BLKSIZE)
#define MOVI_BL2_BLKCNT		(PART_SIZE_BL2 / MOVI_BLKSIZE)
#define MOVI_ENV_BLKCNT		(CONFIG_ENV_SIZE / MOVI_BLKSIZE) /* 16KB */
#define MOVI_UBOOT_BLKCNT	(PART_SIZE_UBOOT / MOVI_BLKSIZE) /* 328KB */
#define MOVI_TZSW_BLKCNT	(PART_SIZE_TZSW / MOVI_BLKSIZE) /* 160KB */

#define MOVI_UBOOT_POS		(MOVI_BL1_BLKCNT + MOVI_BL2_BLKCNT + 1)
#define MOVI_TZSW_POS		(MOVI_BL1_BLKCNT + MOVI_BL2_BLKCNT \
				+ MOVI_UBOOT_BLKCNT + 1)

#define SMC_CMD_LOAD_UBOOT		(-230)
#define SMC_CMD_COLDBOOT		(-231)
#define SMC_CMD_WARMBOOT		(-232)
#define SMC_CMD_CHECK_SECOND_BOOT	(-233)
#define SMC_CMD_EMMC_ENDBOOTOP		(-234)
#define SMC_CMD_SDMMC_ENUMERATE		(-235)

#define SMC_UBOOT_SIGNATURE_SIZE	0
#define SMC_TZSW_SIGNATURE_SIZE		256

#define CONFIG_IMAGE_INFO_BASE		(CONFIG_SYS_SDRAM_BASE)
#define CONFIG_PHY_UBOOT_BASE		(CONFIG_SYS_TEXT_BASE)
#define SMC_SECURE_CONTEXT_BASE		(CONFIG_PHY_IRAM_BASE + 0x4c00)
#define CONFIG_PHY_TZSW_BASE		(CONFIG_PHY_IRAM_BASE + 0x8000)

typedef struct sdmmc_dev {
	/* for SDMMC */
	u32	image_pos;
	u32	blkcnt;
	u32	base_addr;
} sdmmc_t;

typedef struct emmc_dev {
	/* for eMMC */
	u32	blkcnt;
	u32	base_addr;
} emmc_t;

typedef struct sata_dev {
	/* for SATA */
	u64	read_sector_of_hdd;
	u32	trans_byte;
	u32	*read_buffer;
	u32	position_of_mem;
} sata_t;

typedef struct sfmc_dev {
	/* for SFMC */
	u32	cs;
	u32	byte_offset;
	u32	byte_size;
	void	*dest_addr;
} sfmc_t;

typedef struct spi_sf_dev {
	/* for SPI SERIAL FLASH */
	u32	flash_read_addr;
	u32	read_size;
	u8	*read_buff;
} spi_sf_t;

/* boot device */
typedef union boot_device_u {
	sdmmc_t		sdmmc;
	emmc_t		emmc;
	sata_t		sata;
	sfmc_t		sfmc;
	spi_sf_t	spi_sf;
} boot_device_t;

typedef struct ld_image_info {
	/* for Signature */
	u32	image_base_addr;
	u32	size;
	u32	secure_context_base;
	u32	signature_size;
	boot_device_t bootdev;

} image_info;

unsigned int load_uboot_image(u32 boot_device);
unsigned int coldboot(u32 boot_device);
void warmboot(void);
unsigned int find_second_boot(void);
void emmc_endbootop(void);
void sdmmc_enumerate(void);

