/*
 * Atmel Nand Flash Controller (NFC) - System peripherals regsters.
 * Based on SAMA5D3 datasheet.
 *
 * Copyright (C) 2012 Atmel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef ATMEL_NAND_NFC_H
#define ATMEL_NAND_NFC_H

/*
 * NFC definitions
 */
#define NFC_HSMC_REG_BASE	0xffffc000	/* HSMC base address */
#define NFC_SRAM_BASE_ADDRESS	0x200000	/* Base address of NFC SRAM */

#define ATMEL_HSMC_NFC_CFG		0x00	/* NFC Configuration Register */
#define		ATMEL_HSMC_PAGESIZE_512		(0)
#define		ATMEL_HSMC_PAGESIZE_1024	(1)
#define		ATMEL_HSMC_PAGESIZE_2048	(2)
#define		ATMEL_HSMC_PAGESIZE_4096	(3)
#define		ATMEL_HSMC_PAGESIZE_8192	(4)
#define		ATMEL_HSMC_WSPARE		(1 << 8)
#define		ATMEL_HSMC_RSPARE		(1 << 9)
#define		ATMEL_HSMC_EDGE_CTRL_RISING	(0 << 12)
#define		ATMEL_HSMC_EDGE_CTRL_FALLING	(1 << 12)
#define		ATMEL_HSMC_RBEDGE		(1 << 13)
#define		ATMEL_HSMC_NFC_DTOCYC		(0xf << 16)
#define		ATMEL_HSMC_NFC_DTOMUL		(0x7 << 20)
#define		ATMEL_HSMC_NFC_SPARESIZE	(0x7f << 24)

#define ATMEL_HSMC_NFC_CTRL		0x04	/* NFC Control Register */
#define		ATMEL_HSMC_NFC_ENABLE		(1 << 0)
#define		ATMEL_HSMC_NFC_DISABLE		(1 << 1)

#define ATMEL_HSMC_NFC_SR		0x08	/* NFC Status Register */
#define		ATMEL_HSMC_NFC_STATUS		(1 << 0)
#define		ATMEL_HSMC_NFC_RB_RISE		(1 << 4)
#define		ATMEL_HSMC_NFC_RB_FALL		(1 << 5)
#define		ATMEL_HSMC_NFC_BUSY		(1 << 8)
#define		ATMEL_HSMC_NFC_WR		(1 << 11)
#define		ATMEL_HSMC_NFC_CSID		(7 << 12)
#define		ATMEL_HSMC_NFC_XFR_DONE		(1 << 16)
#define		ATMEL_HSMC_NFC_CMD_DONE		(1 << 17)
#define		ATMEL_HSMC_NFC_DTOE		(1 << 20)
#define		ATMEL_HSMC_NFC_UNDEF		(1 << 21)
#define		ATMEL_HSMC_NFC_AWB		(1 << 22)
#define		ATMEL_HSMC_NFC_ASE		(1 << 23)
#define		ATMEL_HSMC_NFC_RB_EDGE		(1 << 24)

#define ATMEL_HSMC_NFC_IER		0x0c	/* NFC Interrupt Enable Register */
#define ATMEL_HSMC_NFC_IDR		0x10	/* NFC Interrupt Disable Register */
#define ATMEL_HSMC_NFC_IMR		0x14	/* NFC Interrupt Mask Register */
#define ATMEL_HSMC_NFC_CYCLE0		0x18	/* NFC Address Cycle Zero Register */
#define		ATMEL_HSMC_NFC_ADDR_CYCLE0	(0xff)

#define ATMEL_HSMC_NFC_BANK		0x1c	/* NFC Bank Register */
#define		ATMEL_HSMC_NFC_BANK0	(0 << 0)
#define		ATMEL_HSMC_NFC_BANK1	(1 << 0)

#define nfc_writel(addr, reg, value) \
	writel((value), (addr) + ATMEL_HSMC_NFC_##reg)

#define nfc_readl(addr, reg) \
	readl((addr) + ATMEL_HSMC_NFC_##reg)

/*
 * NFC Address Command definitions
 */
#define NFC_COMMAND_BASE_ADDR	0x70000000	/* Base for NFC Address Command */

#define NFCADDR_CMD_CMD1	(0xff << 2)	/* Command for Cycle 1 */
#define NFCADDR_CMD_CMD2	(0xff << 10)	/* Command for Cycle 2 */
#define NFCADDR_CMD_VCMD2	(0x1 << 18)	/* Valid Cycle 2 Command */
#define NFCADDR_CMD_ACYCLE	(0x7 << 19)	/* Number of Address required */
#define   NFCADDR_CMD_ACYCLE_NONE	(0x0 << 19)
#define   NFCADDR_CMD_ACYCLE_1		(0x1 << 19)
#define   NFCADDR_CMD_ACYCLE_2		(0x2 << 19)
#define   NFCADDR_CMD_ACYCLE_3		(0x3 << 19)
#define   NFCADDR_CMD_ACYCLE_4		(0x4 << 19)
#define   NFCADDR_CMD_ACYCLE_5		(0x5 << 19)
#define NFCADDR_CMD_CSID	(0x7 << 22)	/* Chip Select Identifier */
#define   NFCADDR_CMD_CSID_0		(0x0 << 22)
#define   NFCADDR_CMD_CSID_1		(0x1 << 22)
#define   NFCADDR_CMD_CSID_2		(0x2 << 22)
#define   NFCADDR_CMD_CSID_3		(0x3 << 22)
#define   NFCADDR_CMD_CSID_4		(0x4 << 22)
#define   NFCADDR_CMD_CSID_5		(0x5 << 22)
#define   NFCADDR_CMD_CSID_6		(0x6 << 22)
#define   NFCADDR_CMD_CSID_7		(0x7 << 22)
#define NFCADDR_CMD_DATAEN	(0x1 << 25)	/* Data Transfer Enable */
#define NFCADDR_CMD_DATADIS	(0x0 << 25)	/* Data Transfer Disable */
#define NFCADDR_CMD_NFCRD	(0x0 << 26)	/* NFC Read Enable */
#define NFCADDR_CMD_NFCWR	(0x1 << 26)	/* NFC Write Enable */
#define NFCADDR_CMD_NFCBUSY	(0x1 << 27)	/* NFC Busy */

#define nfc_cmd_addr1234_writel(cmd, addr1234, nfc_base) \
	writel((addr1234), (cmd) + nfc_base)

#define nfc_cmd_readl(bitstatus, nfc_base) \
	readl((bitstatus) + nfc_base)

#endif
