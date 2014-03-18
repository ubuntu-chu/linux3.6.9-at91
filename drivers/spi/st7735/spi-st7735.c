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

struct class  *dev_class	= NULL;  
struct device *dev_device	= NULL; 

static int st7735_gpio_request(struct st7735_chip*sc);

static ssize_t lcd_clear_store(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t size)
{
	ssize_t			status;
	long		value;
	struct spi_device *spi = container_of(dev, struct spi_device, dev);
	struct st7735_chip *sc = spi_get_drvdata(spi);

	mutex_lock(&sc->mutex);
	status = strict_strtol(buf, 0, &value);
	if (status == 0) {
		background_color_set(value);	
		P_DEBUG_SIMPLE("vaule = %ld\n", value);
		printk("start\n");
		status = size;
		LCD_Clear();
		printk("end\n");
	}
	mutex_unlock(&sc->mutex);

	return status;
}

static const DEVICE_ATTR(lcd_clear, 0222,
	NULL, lcd_clear_store);

#if 0
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

#endif
#if 1

static ssize_t backlight_value_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	ssize_t			status;
	int value;
	struct spi_device *spi = container_of(dev, struct spi_device, dev);
	struct st7735_chip *sc = spi_get_drvdata(spi);

	mutex_lock(&(sc->mutex));
	value = !!gpio_get_value_cansleep(paint_device->backlight_gpio);
	status = sprintf(buf, "%d\n", value);
	mutex_unlock(&(sc->mutex));

	return status;
}

static ssize_t backlight_value_store(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t size)
{
	ssize_t			status;
	long		value;
	struct spi_device *spi = container_of(dev, struct spi_device, dev);
	struct st7735_chip *sc = spi_get_drvdata(spi);

	mutex_lock(&(sc->mutex));
	status = strict_strtol(buf, 0, &value);
	if (status == 0) {
		P_DEBUG_SIMPLE("vaule = %ld\n", value);
		gpio_set_value_cansleep(paint_device->backlight_gpio, !(value != 0));
		status = size;
	}
	mutex_unlock(&(sc->mutex));

	return status;
}

static const DEVICE_ATTR(backlight_value, 0644,
	backlight_value_show, backlight_value_store);
#endif

static ssize_t lcd_display_store(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t size)
{
	ssize_t			status;
	long		value;
	struct spi_device *spi = container_of(dev, struct spi_device, dev);
	struct st7735_chip *sc = spi_get_drvdata(spi);

	mutex_lock(&(sc->mutex));
	status = strict_strtol(buf, 0, &value);
	if (status == 0) {
		P_DEBUG_SIMPLE("vaule = %ld\n", value);
		if (!!value){
			LCD_DisplayOn();
		}else {
			LCD_DisplayOff();
		}
		status = size;
	}
	mutex_unlock(&(sc->mutex));

	return status;
}

static const DEVICE_ATTR(lcd_display, 0222,
	NULL, lcd_display_store);

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

static int st7735_gpio_request(struct st7735_chip*sc){

	int ret;
	struct spi_device *spi	= sc->spi;
	struct device_node *np = spi->dev.of_node;

	//reset pin
	ret = create_gpio(np, &(sc->rst_gpio), RST_PROP_NAME, DRIVER_NAME":"RST_PROP_NAME, 0);
	if (ret)
		goto exit;
#if 0
	ret = device_create_file(&(spi->dev), &dev_attr_rst_value);
	if (ret != 0)
		printk(KERN_INFO "device file rst_value create failed\n");
#endif

	//cd pin
	ret = create_gpio(np, &(sc->cd_gpio), CD_PROP_NAME, DRIVER_NAME":"CD_PROP_NAME, 0);
	if (ret)
		goto exit_1;
#if 0
	ret = device_create_file(&(spi->dev), &dev_attr_cd_value);
	if (ret != 0)
		printk(KERN_INFO "device file cd_value create failed\n");
#endif

	//backlight pin
	ret = create_gpio(np, &(sc->backlight_gpio), BACKLIGHT_PROP_NAME, DRIVER_NAME":"BACKLIGHT_PROP_NAME, 1);
	if (ret)
		goto exit_2;
#if 1
	ret = device_create_file(&(spi->dev), &dev_attr_backlight_value);
	if (ret != 0)
		printk(KERN_INFO "device file backlight_value create failed\n");
#endif

	ret = device_create_file(&(spi->dev), &dev_attr_lcd_clear);
	if (ret != 0){
		printk(KERN_INFO "device lcd_clear create failed\n");
		goto exit_3;
	}

	ret = device_create_file(&(spi->dev), &dev_attr_lcd_display);
	if (ret != 0){
		printk(KERN_INFO "device lcd_display create failed\n");
		goto exit_4;
	}

	return 0;

exit_4:
	device_remove_file(&(spi->dev), &dev_attr_lcd_display);
exit_3:
	device_remove_file(&(spi->dev), &dev_attr_backlight_value);
	gpio_free(sc->backlight_gpio);
exit_2:
	gpio_free(sc->cd_gpio);
exit_1:
	gpio_free(sc->rst_gpio);
exit:
	return ret;
}

static int st7735_gpio_release(struct st7735_chip*sc){

	struct spi_device *spi;

	if (NULL == sc){
		return -EINVAL;
	}
	spi	= sc->spi;
#if 0
	device_remove_file(&(spi->dev), &dev_attr_rst_value);
	device_remove_file(&(spi->dev), &dev_attr_cd_value);
#endif
	mutex_lock(&(sc->mutex));
	device_remove_file(&(spi->dev), &dev_attr_backlight_value);
	device_remove_file(&(spi->dev), &dev_attr_lcd_display);
	device_remove_file(&(spi->dev), &dev_attr_lcd_clear);
	mutex_unlock(&(sc->mutex));
	gpio_free(sc->cd_gpio);
	gpio_free(sc->rst_gpio);
	gpio_free(sc->backlight_gpio);

	return 0;
}

int st7735_open(struct inode *inode, struct file *filp)  
{  
	struct st7735_chip *sc;
	int ret;

	sc = container_of(inode->i_cdev, struct st7735_chip, cdev);
	
	if(!atomic_dec_and_test(&(sc->open_cnt))) {  
		atomic_inc(&(sc->open_cnt));  
		return -EBUSY;  
	}  
	filp->private_data = sc;   
	P_DEBUG_SIMPLE("\n");
	ret = st7735_gpio_request(sc);
	if (ret){
		atomic_inc(&(sc->open_cnt));  
		return -EACCES;  
	}

	foreground_color_set(YELLOW);	
	background_color_set(WHITE);	
	lcd_init();
	background_color_set(RED);	
	LCD_Clear();
	BACKLIGHT_PIN_ON();
	//LCD_DrawLine(0, 0, 160, 128);
	LCD_DrawLine(160, 120, 60, 28);
	LCD_DrawLine(10, 100, 10, 28);
	//LCD_DrawLine(0, 10, 160, 10);
	LCD_DrawLine(160, 10, 100, 10);
	//LCD_DrawCircle(50, 50, 10);
	LCD_FillCircle(50, 50, 10);
	LCD_DrawArc(130, 60, 20, 30, 90);
	LCD_DrawEllipse(110, 130, 45, 90);
	LCD_ShowString(0, 0, "hello st7735 ----HHHHHHHHHHHHHHHHHH--");
	//LCD_ShowChar(0, 0, 'h', 0);
//	LCD_Fill(0, 0, 40, 20);
	//LCD_DrawRectangle(0, 0, 40, 20);
	LCD_DrawRectangle(80, 80, 40, 20);

	return nonseekable_open(inode, filp);
} 

int st7735_release(struct inode *inode, struct file *filp)  
{  
	struct st7735_chip *sc = filp->private_data;

	atomic_inc(&(sc->open_cnt));  
	P_DEBUG_SIMPLE("\n");
	RESET_PIN_LOW();
	BACKLIGHT_PIN_OFF();
	st7735_gpio_release(sc);
	return 0;  
}  

long st7735_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{


	return 0;
}

struct file_operations st7735_fops = {
	.owner = THIS_MODULE,
	.open = st7735_open,
	.release = st7735_release, 
	.unlocked_ioctl = st7735_ioctl,
};

static int __devinit st7735_probe(struct spi_device *spi)
{
	int				ret;
	struct st7735_chip *sc;

	P_DEBUG_SIMPLE("enter\n");
	sc = kzalloc(sizeof(struct st7735_chip), GFP_KERNEL);
	if (!sc)
		return -ENOMEM;        
					
	atomic_set(&(sc->open_cnt), 1);
	spi_set_drvdata(spi, sc);
	sc->spi = spi;
	paint_device_set(sc);

	ret = alloc_chrdev_region(&(sc->devno), 0, 1, "st7735 lcd driver");
	if(ret) {
		printk(KERN_INFO "register_chrdev_region failed\n");
		ret = -EBUSY;
		goto exit_destroy;
	}
	cdev_init(&(sc->cdev), &st7735_fops);
	sc->cdev.owner = THIS_MODULE;
	mutex_init(&(sc->mutex));
	ret = cdev_add(&(sc->cdev), sc->devno, 1);
	if(ret)
	{
		printk(KERN_INFO "cdev add failed\n");
		ret = -ENODEV;
		goto exit_destroy_1;
	}
	dev_class = class_create(THIS_MODULE, "st7735");  
	if(IS_ERR(dev_class)){  
		printk(KERN_INFO "class_create failed!\n");  
		ret = PTR_ERR("dev_class");  
		goto exit_destroy_2;  
	}  
	dev_device = device_create(dev_class, NULL, sc->devno, NULL, "st7735");  
	if(IS_ERR(dev_device)){  
		printk(KERN_INFO "device_create failed!\n");  
		ret = PTR_ERR(dev_device);  
		goto exit_destroy_3;  
	}  
	sc->frame_buff   = (u16 *)kmalloc(LCD_FRAME_BUFF_SIZE, GFP_KERNEL);
	if (sc->frame_buff == NULL){
		goto exit_destroy_3;
	}

	P_DEBUG_SIMPLE("exit\n");
	return 0;

exit_destroy_3:
	class_destroy(dev_class); 
exit_destroy_2:
	cdev_del(&(sc->cdev));  
exit_destroy_1:
	unregister_chrdev_region(sc->devno, 1);
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

	if (sc->frame_buff != NULL){
		kfree(sc->frame_buff);
	}
	mutex_destroy(&(sc->mutex));
	device_destroy(dev_class, sc->devno);  
	class_destroy(dev_class); 
	cdev_del(&(sc->cdev));  
	unregister_chrdev_region(sc->devno, 1);

	kfree(sc);
	return 0;
}

static const struct of_device_id st7735_dt_ids[] = {
	{ .compatible = "st7735" },
	{ },
};

MODULE_DEVICE_TABLE(of, st7735_dt_ids);

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
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("st7735 SPI based tft driver");
MODULE_ALIAS("spi lcd:DRIVER_NAME");
