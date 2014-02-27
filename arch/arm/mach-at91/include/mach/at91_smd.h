/*
 * arch/arm/mach-at91/include/mach/at91_smd.h
 *
 * Copyright (C) SAN People
 *
 * Software Modem Device (SMD) registers.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef AT91_SMD_H
#define AT91_SMD_H

#define AT91_PMC_SMDCK          (1<<4)


#define AT91_SMD_BASE_ADDR      0x00400000
/*Wrap value 1:0 250 bits in a frame, and power 7:2 */
#define AT91_SMD_WRAPVALUE      0xFC
/*set duty cycle 5/7-2/7 synchronization, 1/6 power clock, bit[1:0]= 10, (24000000/6/250=16000Hz) */
#define AT91_SMD_ADJVALUE      0x2A

/* regs */
#define AT91_SMD_CONTROL        (0x00)
#define AT91_SMD_POPR           (0x00) /* Banked */
#define AT91_SMD_STATUS         (0x04)
#define AT91_SMD_REV            (0x04) /* Banked */
#define AT91_SMD_WRAP           (0x08)
#define AT91_SMD_DRIVE          (0x0C)
#define AT91_SMD_ADJ            (0x10)
#define AT91_SMD_VOLC           (0x14)
#define AT91_SMD_SAMCO          (0x14) /* Banked */
#define AT91_SMD_AUX            (0x18)
#define AT91_SMD_PULSE          (0x2C)
#define AT91_SMD_DATA1L         (0x20)
#define AT91_SMD_DATA1M         (0x24)
#define AT91_SMD_CTRL1L         (0x28)
#define AT91_SMD_CTRL1M         (0x2C)
#define AT91_SMD_DATA2L         (0x30)
#define AT91_SMD_DATA2M         (0x34)
#define AT91_SMD_CTRL2L         (0x38)
#define AT91_SMD_PARAM          (0x3C)
#define AT91_SMD_TRIM           (0x3C) /* Banked */
/* --- */

#define AT91_SMD_FIFOCTRL       (0x40)
#define AT91_SMD_FIFO_IRQ_LVL   (0x44)
#define AT91_SMD_LPWR_CLK_ENB   (0x48)

#define AT91_SMD_FIFO_INT_STAT          0x200
#define AT91_SMD_FIFO_INT_ENA           0x100

#define AT91_SMD_TX_FIFO_RESET          0x80
#define AT91_SMD_RX_FIFO_RESET          0x40

#define AT91_SMD_TX_DMA_CONT_ENA        0x08
#define AT91_SMD_RX_DMA_CONT_ENA        0x04

#define AT91_SMD_CONTROL_SERIAL_DMA_ENA 0x01

/* AT91_SMD_AUX_ register bit definitions */
#define AT91_SMD_AUX_DMA_ACCESS_ENA             0x01
#define AT91_SMD_AUX_HW_RING_DETECT_ENA         0x02
#define AT91_SMD_AUX_HW_RING_DETECT_WAKEUP_ENA  0x04
#define AT91_SMD_AUX_HW_RING_DETECT_INT_ENA     0x08
#define AT91_SMD_AUX_EXT_PICKUP_INT_ENA         0x20
#define AT91_SMD_AUX_HW_RING_DETECT_STAT        0x40
#define AT91_SMD_AUX_EXT_PICKUP_STAT            0x80


/* AT91_SMD_LPWR_CLK_ENB register bit definitions */
#define AT91_SMD_LPWR_CLK_ENA                   0x01
#define SSD_LPWR_DIFF_DIS                       0x04

/* AT91_SMD_STATUS register bit definitions */
#define AT91_SMD_STATUS_CTRL_COMP_SUCCESS       0x08
#define AT91_SMD_STATUS_CTRL_COMP_ERR           0x04
#define AT91_SMD_STATUS_RX_FIFO_OVERRUN         0x10
#define AT91_SMD_STATUS_TX_FIFO_UNDERRUN        0x20
#define AT91_SMD_STATUS_FIFO_ALL_ERROR          (AT91_SMD_STATUS_TX_FIFO_UNDERRUN | AT91_SMD_STATUS_RX_FIFO_OVERRUN)
#define AT91_SMD_STATUS_RX_FIFO_NOT_EMPTY       0x40
#define AT91_SMD_STATUS_TX_FIFO_NOT_FULL        0x80


/* AT91_SMD_ADJ register bit definitons */
/* Select banked registers at offset 0x00, 0x04, 0x14, 0x3C */
#define AT91_SMD_BANK1_REG_SELECT               0x40

/* Low Power Defs */
#define AT91_SMD_LWPR_CLK_EN                    0x01
#define AT91_SMD_LWPR_DIFF_EN                   0x04

#endif
