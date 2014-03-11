/**
 * drivers/serial/st7735s.c
 *
 * Copyright (C) 2009 Manuel Stahl <manuel.stahl@iis.fraunhofer.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * The st7735s device is a SPI driven dual UART with GPIOs.
 *
 * The driver exports two uarts and a gpiochip interface.
 */

/* #define DEBUG */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>

#include <linux/delay.h>
#include <linux/device.h>
#include <linux/spi/spi.h>
#include <linux/freezer.h>

#include <linux/spi/spi.h>
#include <linux/spi/spi_bitbang.h>

#define DRIVER_NAME	"spi_lcd_st7725s"

#define REG_READ	0x80
#define REG_WRITE	0x00

/* Special registers */
#define REG_TXLVL	0x08	/* Transmitter FIFO Level register */
#define REG_RXLVL	0x09	/* Receiver FIFO Level register */
#define REG_IOD		0x0A	/* IO Direction register */
#define REG_IOS		0x0B	/* IO State register */
#define REG_IOI		0x0C	/* IO Interrupt Enable register */
#define REG_IOC		0x0E	/* IO Control register */

#define IOC_SRESET	0x08    /* Software reset */
#define IOC_GPIO30	0x04    /* GPIO 3:0 unset: as IO, set: as modem pins */
#define IOC_GPIO74	0x02    /* GPIO 7:4 unset: as IO, set: as modem pins */
#define IOC_IOLATCH	0x01    /* Unset: input unlatched, set: input latched */

/* ******************************** SPI ********************************* */

static inline u8 write_cmd(u8 reg, u8 ch)
{
	return REG_WRITE | (reg & 0xf) << 3 | (ch & 0x1) << 1;
}

static inline u8 read_cmd(u8 reg, u8 ch)
{
	return REG_READ  | (reg & 0xf) << 3 | (ch & 0x1) << 1;
}

/*
 * st7735s_write - Write a new register content (sync)
 * @reg: Register offset
 * @ch:  Channel (0 or 1)
 */
static int st7735s_write(struct st7735s_chip *ts, u8 reg, u8 ch, u8 val)
{
	u8 out[2];

	out[0] = write_cmd(reg, ch);
	out[1] = val;
	return spi_write(ts->spi, out, sizeof(out));
}

/**
 * st7735s_read - Read back register content
 * @reg: Register offset
 * @ch:  Channel (0 or 1)
 *
 * Returns positive 8 bit value from the device if successful or a
 * negative value on error
 */
static int st7735s_read(struct st7735s_chip *ts, unsigned reg, unsigned ch)
{
	return spi_w8r8(ts->spi, read_cmd(reg, ch));
}

static int __devinit st7735s_probe(struct spi_device *spi)
{
	int				status;


	return status;
}

static int __devexit st7735s_remove(struct spi_device *spi)
{

	return 0;
}

/* Spi driver data */
static struct spi_driver st7735s_spi_driver = {
	.driver = {
		.name		= DRIVER_NAME,
		.bus		= &spi_bus_type,
		.owner		= THIS_MODULE,
	},
	.probe		=  st7735s_probe,
	//.shutdown        = st7735s_shutdown,
	.remove		= __devexit_p(st7735s_remove),	
};

/* Driver init function */
static int __init st7735s_init(void)
{
	return spi_register_driver(&st7735s_spi_driver);	
}

/* Driver exit function */
static void __exit st7735s_exit(void)
{
	spi_unregister_driver(&st7735s_spi_driver);
}

/* register after spi postcore initcall and before
 * subsys initcalls that may rely on these GPIOs
 */
subsys_initcall(st7735s_init);
module_exit(st7735s_exit);

MODULE_AUTHOR("jspower");
MODULE_LICENSE("GPL v1");
MODULE_DESCRIPTION("st7735s SPI based tft driver");
MODULE_ALIAS("spi lcd:DRIVER_NAME);
