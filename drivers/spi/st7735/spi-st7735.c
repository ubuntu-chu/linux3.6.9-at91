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

#include "includes.h"

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
	background_color_set(YELLOW);	
	foreground_color_set(BLACK);	
	lcd_init();

	return nonseekable_open(inode, filp);
} 

int st7735_release(struct inode *inode, struct file *filp)  
{  
	struct st7735_chip *sc = filp->private_data;
	struct font_set *font_set  = NULL;
	struct font_node *font_node  = NULL;
	struct list_head  *pnode;

	list_for_each(pnode, &font_list_head){
		font_node	= list_entry(pnode, struct font_node, node);
		font_set	= font_node->font_set;
		P_DEBUG_SIMPLE("font_set: %s release\n", font_set->name);
		kfree(font_set);
		kfree(font_node);
	}
	atomic_inc(&(sc->open_cnt));  
	P_DEBUG_SIMPLE("\n");
	RESET_PIN_LOW();
	BACKLIGHT_PIN_OFF();
	st7735_gpio_release(sc);
	return 0;  
}  

const struct lcd_info t_lcd_info_st7735 = {
	LCD_W,
	LCD_H,
	16,
};

long st7735_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct st7735_chip *sc = filp->private_data;
	int err = 0;
	int ret = 0;

	/* 检测命令的有效性 */
	if (_IOC_TYPE(cmd) != ST7735IOC_MAGIC) 
		return -EINVAL;
	if (_IOC_NR(cmd) > NR_IOC_MAX) 
		return -EINVAL;

	/* 根据命令类型，检测参数空间是否可以访问 */
	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, (void *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err = !access_ok(VERIFY_READ, (void *)arg, _IOC_SIZE(cmd));
	if (err) 
		return -EFAULT;

	mutex_lock(&sc->mutex);
	switch(cmd) {
		case ST7735IOC_INIT:
			lcd_init();
			break;
		case ST7735IOC_UNINIT:
			break;
		case ST7735IOC_LCD_INFO:
			if (copy_to_user((void *)arg, &t_lcd_info_st7735, 
						sizeof(struct lcd_info))){  
				ret =  -EFAULT;  
				break;
			}
			break;
		case ST7735IOC_SET_FOREGROUND_COLOR:
			{
				struct color t_color;
				if (copy_from_user(&t_color, (struct color *)arg, 
							sizeof(struct color))){  
					ret =  -EFAULT;  
					break;
				}
				foreground_color_set(t_color.m_color);	
				//printk("color = 0x%x\n", t_color.m_color);
			}
			break;
		case ST7735IOC_SET_BACKGROUND_COLOR:
			{
				struct color t_color;
				if (copy_from_user(&t_color, (struct color *)arg, 
							sizeof(struct color))){  
					ret =  -EFAULT;  
					break;
				}
				background_color_set(t_color.m_color);	
				//printk("back color = 0x%x\n", t_color.m_color);
			}
			break;
		case ST7735IOC_CLEAR:
			LCD_Clear();
			break;
		case ST7735IOC_BACKLIGHT_ON:
			BACKLIGHT_PIN_ON();
			break;
		case ST7735IOC_BACKLIGHT_OFF:
			BACKLIGHT_PIN_OFF();
			break;
		case ST7735IOC_DRAW_POINT:
			{
				struct coordinate t_coordinate;
				if (copy_from_user(&t_coordinate, (struct coordinate *)arg, 
							sizeof(struct coordinate))){  
					ret =  -EFAULT;  
					break;
				}
				coordinate_check(&t_coordinate);
				LCD_DrawPoint(t_coordinate.m_x, t_coordinate.m_y);
			}
			break;
		case ST7735IOC_DRAW_LINE:
			{
				struct coordinate_pair t_coordinate_pair;
				if (copy_from_user(&t_coordinate_pair, 
							(struct coordinate_pair *)arg, 
							sizeof(struct coordinate_pair))){  
					ret =  -EFAULT;  
					break;
				}
				coordinate_pair_check(&t_coordinate_pair);
				LCD_DrawLine(t_coordinate_pair.m_x1, t_coordinate_pair.m_y1,
						t_coordinate_pair.m_x2, t_coordinate_pair.m_y2);
			}
			break;
		case ST7735IOC_DRAW_RECTANGLE:
		case ST7735IOC_FILL_RECTANGLE:
		case ST7735IOC_CLEAR_RECTANGLE:
			{
				struct coordinate_pair t_coordinate_pair;
				if (copy_from_user(&t_coordinate_pair, 
							(struct coordinate_pair *)arg, 
							sizeof(struct coordinate_pair))){  
					ret =  -EFAULT;  
					break;
				}
				coordinate_pair_check(&t_coordinate_pair);
				if (cmd == ST7735IOC_DRAW_RECTANGLE){
					LCD_DrawRectangle(t_coordinate_pair.m_x1, 
							t_coordinate_pair.m_y1, t_coordinate_pair.m_x2, 
							t_coordinate_pair.m_y2);
				}else if (cmd == ST7735IOC_CLEAR_RECTANGLE){
					LCD_ClearRectangle(t_coordinate_pair.m_x1, 
							t_coordinate_pair.m_y1, t_coordinate_pair.m_x2, 
							t_coordinate_pair.m_y2);
				}else {
					LCD_FillRectangle(t_coordinate_pair.m_x1, 
							t_coordinate_pair.m_y1, t_coordinate_pair.m_x2, 
							t_coordinate_pair.m_y2);
				}
			}
			break;
		case ST7735IOC_SHOW_STRING:
			{
				struct string t_string;
				if (copy_from_user(&t_string, 
							(struct string *)arg, 
							sizeof(struct string))){  
					ret =  -EFAULT;  
					break;
				}
//				printk("str = %s\n", t_string.m_data);
//				printk("str addr = %p\n", t_string.m_data);
				LCD_ShowString(t_string.m_x, t_string.m_y, t_string.m_data);
			}
			break;
		case ST7735IOC_DRAW_CIRCLE:
		case ST7735IOC_FILL_CIRCLE:
			{
				struct circle t_circle;
				if (copy_from_user(&t_circle, 
							(struct circle *)arg, 
							sizeof(struct circle))){  
					ret =  -EFAULT;  
					break;
				}
				if (cmd == ST7735IOC_DRAW_CIRCLE){
					LCD_DrawCircle(t_circle.m_x, t_circle.m_y, t_circle.m_r);
				}else {
					LCD_FillCircle(t_circle.m_x, t_circle.m_y, t_circle.m_r);
				}
			}
			break;
		case ST7735IOC_DRAW_ELLIPSE:
		case ST7735IOC_FILL_ELLIPSE:
			{
				struct coordinate_pair t_coordinate_pair;
				if (copy_from_user(&t_coordinate_pair, 
							(struct coordinate_pair *)arg, 
							sizeof(struct coordinate_pair))){  
					ret =  -EFAULT;  
					break;
				}
				coordinate_pair_check(&t_coordinate_pair);
				if (cmd == ST7735IOC_DRAW_ELLIPSE){
					LCD_DrawEllipse(t_coordinate_pair.m_x1, 
							t_coordinate_pair.m_y1, t_coordinate_pair.m_x2, 
							t_coordinate_pair.m_y2);
				}else {
					LCD_FillEllipse(t_coordinate_pair.m_x1, 
							t_coordinate_pair.m_y1, t_coordinate_pair.m_x2, 
							t_coordinate_pair.m_y2);
				}
			}
			break;
		case ST7735IOC_DRAW_ARC:
			{
				struct arc t_arc;
				if (copy_from_user(&t_arc, 
							(struct arc *)arg, 
							sizeof(struct arc))){  
					ret =  -EFAULT;  
					break;
				}
				LCD_DrawArc(t_arc.m_x, t_arc.m_y, t_arc.m_r, t_arc.m_stangle, 
						t_arc.m_endangle);
			}
			break;
		case ST7735IOC_DRAW_BITMAP:
			{
				struct pict t_pict;
				GUI_BITMAP  t_bitmap;

				if (copy_from_user(&t_pict, 
							(struct pict *)arg, 
							sizeof(struct pict))){  
					ret =  -EFAULT;  
					break;
				}
				t_bitmap.x_size		= t_pict.m_x_size;
				t_bitmap.y_size		= t_pict.m_y_size;
				t_bitmap.data		= (uint8 *)(sc->frame_buff);
				LCD_DrawBitmap(t_pict.m_x, t_pict.m_y, &t_bitmap);
			}
			break;
		case ST7735IOC_SET_FONT:
			{
				P_DEBUG_SIMPLE("ST7735IOC_SET_FONT start\n");
				P_DEBUG_SIMPLE("new font name: %s\n", (char *)arg);
				if (font_set((const char *)arg)){
					ret =  -EINVAL;  
					break;
				}
				P_DEBUG_SIMPLE("ST7735IOC_SET_FONT end\n");
			}
			break;
		case ST7735IOC_ADD_FONT:
			{
				struct font t_font;
				struct font_set *font_set;
				struct font_node *node; 

				if (copy_from_user(&t_font, 
							(struct font *)arg, 
							sizeof(struct font))){  
					ret =  -EFAULT;  
					break;
				}
				P_DEBUG_SIMPLE("ST7735IOC_ADD_FONT start\n");
				P_DEBUG_SIMPLE("name: %s size: %d\n", t_font.name, t_font.size);
				if (NULL != font_find(t_font.name)){
					P_DEBUG_SIMPLE("ST7735IOC_ADD_FONT font already added!\n");
					ret =  -EINVAL;  
					break;
				}
				font_set   = (struct font_set *)kmalloc(
								sizeof(struct font_set) + t_font.size, 
								GFP_KERNEL);
				if (font_set == NULL){
					ret = -ENOMEM;
					break;
				}
				if (copy_from_user(font_set, 
							(struct font_set *)arg, 
							sizeof(struct font_set)+t_font.size)){  
					ret =  -EFAULT;  
					kfree(font_set);
					break;
				}
				node	= (struct font_node *)kmalloc(
								sizeof(struct font_node), GFP_KERNEL);
				if (node == NULL){
					ret = -ENOMEM;
					kfree(font_set);
					break;
				}
				node->font_set		= font_set;
				list_add_tail(&(node->node), &font_list_head);
				P_DEBUG_SIMPLE("ST7735IOC_ADD_FONT end\n");
			}
			break;
		default:
			ret		= -EINVAL;
			break;
	}
	mutex_unlock(&sc->mutex);

	return ret;
}

static void st7735_vm_open(struct vm_area_struct *vma)
{
	P_DEBUG_SIMPLE("virt: 0x%1lx, offset: 0x%2lx\n", vma->vm_start, 
			vma->vm_pgoff<<PAGE_SHIFT);
}

static void st7735_vm_close(struct vm_area_struct *vma)
{
	P_DEBUG_SIMPLE("\n");
}

struct vm_operations_struct vm_ops = 
{
	.open = st7735_vm_open,
	.close = st7735_vm_close,
};

static int st7735_mmap(struct file * filp, struct vm_area_struct * vma) 
{
	struct st7735_chip *sc = filp->private_data;
	unsigned long vmsize = vma->vm_end-vma->vm_start;
	unsigned long offset = vma->vm_pgoff << PAGE_SHIFT; 
	unsigned long available_size = LCD_FRAME_BUFF_SIZE - offset;

	if (vmsize > available_size)
		return -ENXIO;

	if (remap_pfn_range(vma, vma->vm_start,
			virt_to_phys((void*)((unsigned long)sc->frame_buff)) >> PAGE_SHIFT, 
			vmsize, vma->vm_page_prot)){
		return -EAGAIN;
	}
	vma->vm_ops = &vm_ops;
	st7735_vm_open(vma);

	return 0;
}

struct file_operations st7735_fops = {
	.owner = THIS_MODULE,
	.open = st7735_open,
	.release = st7735_release, 
	.mmap	= st7735_mmap,
	.unlocked_ioctl = st7735_ioctl,
};

static int __devinit st7735_probe(struct spi_device *spi)
{
	int				ret;
	struct st7735_chip *sc;
	unsigned long  virt_addr;

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
	for (virt_addr = (unsigned long)(sc->frame_buff); 
			virt_addr < (unsigned long)(sc->frame_buff) + LCD_FRAME_BUFF_SIZE;
			virt_addr += PAGE_SIZE){
		SetPageReserved(virt_to_page(virt_addr));
	}
	INIT_LIST_HEAD(&font_list_head);

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
		unsigned long virt_addr;

		for (virt_addr = (unsigned long)(sc->frame_buff); 
				virt_addr < (unsigned long)(sc->frame_buff) + LCD_FRAME_BUFF_SIZE;
				virt_addr += PAGE_SIZE){
			ClearPageReserved(virt_to_page(virt_addr));
		}
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
