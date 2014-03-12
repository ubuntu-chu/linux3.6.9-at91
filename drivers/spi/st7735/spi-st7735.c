/**
 * drivers/serial/st7735.c
 *
 * Copyright (C) 2009 Manuel Stahl <manuel.stahl@iis.fraunhofer.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * The st7735 device is a SPI driven dual UART with GPIOs.
 *
 * The driver exports two uarts and a gpiochip interface.
 */

#include "st7735.h"

static ssize_t lcd_clear_store(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t size)
{
	ssize_t			status;
	long		value;

	status = strict_strtol(buf, 0, &value);
	if (status == 0) {
		background_color_set(value);	
		P_DEBUG_SIMPLE("vaule = %ld\n", value);
		P_DEBUG_SIMPLE("vaule = 0x%x\n", value);
		status = size;
		LCD_Clear();
	}

	return status;
}

static const DEVICE_ATTR(lcd_clear, 0222,
	NULL, lcd_clear_store);

static ssize_t rst_value_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	ssize_t			status;
	int value;

	value = !!gpio_get_value_cansleep(paint_device->rst_gpio);
	status = sprintf(buf, "%d\n", value);

	return status;
}

static ssize_t rst_value_store(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t size)
{
	ssize_t			status;
	long		value;

	status = strict_strtol(buf, 0, &value);
	if (status == 0) {
		P_DEBUG_SIMPLE("vaule = %ld\n", value);
		gpio_set_value_cansleep(paint_device->rst_gpio, value != 0);
		status = size;
	}

	return status;
}

static const DEVICE_ATTR(rst_value, 0644,
	rst_value_show, rst_value_store);


static ssize_t cd_value_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	ssize_t			status;
	int value;

	value = !!gpio_get_value_cansleep(paint_device->cd_gpio);
	status = sprintf(buf, "%d\n", value);

	return status;
}

static ssize_t cd_value_store(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t size)
{
	ssize_t			status;
	long		value;

	status = strict_strtol(buf, 0, &value);
	if (status == 0) {
		P_DEBUG_SIMPLE("vaule = %ld\n", value);
		gpio_set_value_cansleep(paint_device->cd_gpio, value != 0);
		status = size;
	}

	return status;
}

static const DEVICE_ATTR(cd_value, 0644,
	cd_value_show, cd_value_store);


static ssize_t backlight_value_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	ssize_t			status;
	int value;

	value = !!gpio_get_value_cansleep(paint_device->backlight_gpio);
	status = sprintf(buf, "%d\n", value);

	return status;
}

static ssize_t backlight_value_store(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t size)
{
	ssize_t			status;
	long		value;

	status = strict_strtol(buf, 0, &value);
	if (status == 0) {
		P_DEBUG_SIMPLE("vaule = %ld\n", value);
		gpio_set_value_cansleep(paint_device->backlight_gpio, value != 0);
		status = size;
	}

	return status;
}

static const DEVICE_ATTR(backlight_value, 0644,
	backlight_value_show, backlight_value_store);

static int __devinit create_gpio(struct device_node *np, unsigned *pin, const char *name, const char *gpio_name, int value)
{
	int ret;
	enum of_gpio_flags flags;
	unsigned gpio;

	if (NULL == pin){
		printk(KERN_INFO "pin must not be null\n");
		return 0;
	}
	gpio	= of_get_named_gpio_flags(np, name, 0, &flags);

	/* skip leds that aren't available */
	if (!gpio_is_valid(gpio)) {
		printk(KERN_INFO "unavailable gpio %d (%s)\n", gpio, name);
		return 0;
	}

	ret = gpio_request(gpio, gpio_name);
	if (ret < 0){
		printk(KERN_INFO "gpio %d (%s) request failed\n", gpio, name);
		return ret;
	}
	P_DEBUG_SIMPLE("gpio %d (%s:%s) request sucess\n", gpio, DRIVER_NAME, name);

	ret = gpio_direction_output(gpio, value);
	if (ret < 0)
		goto err;
	*pin	= gpio;
		
	return 0;
err:
	gpio_free(gpio);
	return ret;
}
static int __devinit st7735_probe(struct spi_device *spi)
{
	int				ret;
	struct st7735_chip *sc;
	struct device_node *np = spi->dev.of_node;

	P_DEBUG_SIMPLE("enter\n");
	sc = kzalloc(sizeof(struct st7735_chip), GFP_KERNEL);
	if (!sc)
		return -ENOMEM;        
					
	spi_set_drvdata(spi, sc);
	sc->spi = spi;

	paint_device_set(sc);
	//reset pin
	ret = create_gpio(np, &(sc->rst_gpio), RST_PROP_NAME, DRIVER_NAME":"RST_PROP_NAME, 0);
	if (ret)
		goto exit_destroy;
	ret = device_create_file(&(spi->dev), &dev_attr_rst_value);
	if (ret != 0)
		printk(KERN_INFO "device file rst_value create failed\n");

	//cd pin
	ret = create_gpio(np, &(sc->cd_gpio), CD_PROP_NAME, DRIVER_NAME":"CD_PROP_NAME, 0);
	if (ret)
		goto exit_1;
	ret = device_create_file(&(spi->dev), &dev_attr_cd_value);
	if (ret != 0)

		printk(KERN_INFO "device file cd_value create failed\n");
	//backlight pin
	ret = create_gpio(np, &(sc->backlight_gpio), BACKLIGHT_PROP_NAME, DRIVER_NAME":"BACKLIGHT_PROP_NAME, 1);
	if (ret)
		goto exit_2;
	ret = device_create_file(&(spi->dev), &dev_attr_backlight_value);
	if (ret != 0)
		printk(KERN_INFO "device file backlight_value create failed\n");

	ret = device_create_file(&(spi->dev), &dev_attr_lcd_clear);
	if (ret != 0)
		printk(KERN_INFO "device lcd_clear backlight_value create failed\n");

	foreground_color_set(BLACK);	
	//background_color_set(WHITE);	
	background_color_set(RED);	
	lcd_init();
	LCD_Clear();
	BACKLIGHT_PIN_ON();
	LCD_ShowString(0, 0, "hello st7735");

	P_DEBUG_SIMPLE("exit\n");
	return 0;

exit_2:
	gpio_free(sc->cd_gpio);
exit_1:
	gpio_free(sc->rst_gpio);

exit_destroy:
	dev_set_drvdata(&spi->dev, NULL);
	kfree(sc);
	return ret;
}

static int __devexit st7735_remove(struct spi_device *spi)
{
	struct st7735_chip *sc = spi_get_drvdata(spi);

	if (sc == NULL)
		return -ENODEV;

	gpio_free(sc->backlight_gpio);
	gpio_free(sc->cd_gpio);
	gpio_free(sc->rst_gpio);
	kfree(sc);
	return 0;
}

static const struct of_device_id st7735_dt_ids[] = {
	{ .compatible = "st7735" },
};

MODULE_DEVICE_TABLE(of, atmel_serial_dt_ids);

/* Spi driver data */
static struct spi_driver st7735_spi_driver = {
	.driver = {
		.name		= DRIVER_NAME,
		.bus		= &spi_bus_type,
		.owner		= THIS_MODULE,
		.of_match_table	= of_match_ptr(st7735_dt_ids),
	},
	.probe		=  st7735_probe,
	//.shutdown        = st7735_shutdown,
	.remove		= __devexit_p(st7735_remove),	
};

/* Driver init function */
static int __init st7735_init(void)
{
	P_DEBUG_SIMPLE("\n");
	return spi_register_driver(&st7735_spi_driver);	
}

/* Driver exit function */
static void __exit st7735_exit(void)
{
	P_DEBUG_SIMPLE("\n");
	spi_unregister_driver(&st7735_spi_driver);
}

/* register after spi postcore initcall and before
 * subsys initcalls that may rely on these GPIOs
 */
subsys_initcall(st7735_init);
module_exit(st7735_exit);

MODULE_AUTHOR("jspower");
MODULE_LICENSE("GPL v1");
MODULE_DESCRIPTION("st7735 SPI based tft driver");
MODULE_ALIAS("spi lcd:DRIVER_NAME");
