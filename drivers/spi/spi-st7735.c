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

/* #define DEBUG */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>

#include <linux/delay.h>
#include <linux/device.h>
#include <linux/spi/spi.h>
#include <linux/freezer.h>

#include <linux/delay.h>

#include <linux/spi/spi.h>
#include <linux/spi/spi_bitbang.h>

#define DRIVER_NAME	"lcd_st7725"

#define    P_DEBUG_SWITCH        (1)
 
#if    (P_DEBUG_SWITCH > 0)
    #define P_DEBUG_DEV(dev, fmt, args...)   printk("<1>" "<kernel>[%s %s %s(%d):%s]"fmt,\
			                        dev_driver_string(dev), \
			                        dev_name(dev), __FILE__, __LINE__,__FUNCTION__, ##args)
    #define P_DEBUG(fmt, args...)   printk("<0>" "<kernel>[%s(%d):%s]"fmt, __FILE__, __LINE__, \
			                        __FUNCTION__, ##args)
    #define P_DEBUG_SIMPLE(fmt, args...)   printk("<0>" "<kernel>[%s]"fmt,  \
			__FUNCTION__, ##args)

	#define P_DEBUG_PORT(port, fmt, args...)    do{ if (port->line > 1){\
				printk("<1>" "<kernel>[%s(%d):%s]"fmt, __FILE__, __LINE__, \
						__FUNCTION__, ##args);}\
				}while(0)
#else
    #define P_DEBUG_DEV(dev, fmt, args...)
    #define P_DEBUG(fmt, args...)
	#define P_DEBUG_SIMPLE(fmt, args...)
	#define P_DEBUG_PORT(port, fmt, args...)    
#endif

struct st7735_chip {
	struct spi_device *spi;
	unsigned 	rst_gpio;
	unsigned	rst_active_low;

	unsigned 	cd_gpio;
	unsigned	cd_active_low;

	unsigned 	backlight_gpio;
	unsigned	backlight_active_low;
};

#define RESET_PIN_HIGH()	gpio_set_value_cansleep(paint_device->rst_gpio, 1)
#define RESET_PIN_LOW() 	gpio_set_value_cansleep(paint_device->rst_gpio, 0)

#define CD_PIN_HIGH()		gpio_set_value_cansleep(paint_device->cd_gpio, 1)
#define CD_PIN_LOW()		gpio_set_value_cansleep(paint_device->cd_gpio, 0)

#define BACKLIGHT_PIN_ON()	gpio_set_value_cansleep(paint_device->backlight_gpio, 0)
#define BACKLIGHT_PIN_OFF()	gpio_set_value_cansleep(paint_device->backlight_gpio, 1)

#define RST_PROP_NAME      "rst-gpios"
#define CD_PROP_NAME       "cd-gpios"
#define BACKLIGHT_PROP_NAME      "backlight-gpios"

//定义LCD的尺寸
#define LCD_W			128
#define LCD_H			160

//画笔颜色
#define WHITE				 0xFFFF
#define BLACK				 0  
#define BLUE			 0x001F  
#define BRED             0XF81F
#define GRED			 0XFFE0
#define GBLUE 0X07FF
#define RED				 0xF800
#define MAGENTA			 0xF81F
#define GREEN				 0x07E0
#define CYAN				 0x7FFF
#define YELLOW			 0xFFE0
#define BROWN				 0XBC40 //棕色
#define BRRED				 0XFC07 //棕红色
#define GRAY				 0X8430 //灰色
//GUI颜色

#define DARKBLUE			 0X01CF//深蓝色
#define LIGHTBLUE			 0X7D7C//浅蓝色  
#define GRAYBLUE			 0X5458 //灰蓝色
//以上三色为PANEL的颜色 
 
#define LIGHTGREEN		 0X841F //浅绿色
#define LGRAY			 0XC618 //浅灰色(PANNEL),窗体背景色

#define LGRAYBLUE        0XA651 //浅灰蓝色(中间层颜色)
#define LBBLUE           0X2B12 //浅棕蓝色(选择条目的反色)

u16 g_foreground_color = 0x0000;
u16 g_background_color = 0xffff;
struct st7735_chip *paint_device;

void paint_device_set(struct st7735_chip *pd){
	paint_device	= pd;
}

u16 foreground_color_set(u16 color){
	u16	tmp	= g_foreground_color;

	g_foreground_color	= color;
	return tmp;
}
u16 foreground_color_get(void){
	return g_foreground_color;
}

u16 background_color_set(u16 color){
	u16	tmp	= g_background_color;

	g_background_color	= color;
	return tmp;
}
u16 background_color_get(void){
	return g_background_color;
}
/* ******************************** SPI ********************************* */

static int write_data8(u8 data)
{
	u8 out[1];

	CD_PIN_HIGH();
	out[0] = data;
	return spi_write(paint_device->spi, out, sizeof(out));
}

static int write_data16(u16 data)
{
	u8 out[2];

	CD_PIN_HIGH();
	out[0] = data >> 8;
	out[1] = data & 0x00ff;
	return spi_write(paint_device->spi, out, sizeof(out));
}

static int write_reg_addr(u8 reg)
{
	u8 out[1];

	CD_PIN_LOW();
	out[0] = reg;
	return spi_write(paint_device->spi, out, sizeof(out));
}

static int write_reg_data16(u8 reg, u16 data)
{
	write_reg_addr(reg);
	return write_data16(data);
}

static void address_set(u16 x1, u16 y1, u16 x2, u16 y2)
{
	write_reg_addr(0x2a);
	write_data16(x1);
	write_data16(x2);

	write_reg_addr(0x2b);
	write_data16(y1);
	write_data16(y2);

	write_reg_addr(0x2c);
}

static void lcd_init(void)
{
	RESET_PIN_HIGH();
	mdelay(5);
	RESET_PIN_LOW();
	mdelay(5);
	RESET_PIN_HIGH();
	mdelay(5);

	write_reg_addr(0x11); //Sleep out
	mdelay(120); //Delay 120ms
	//----------------ST7735S Frame Rate--------------------------//
	write_reg_addr(0xB1);
	write_data8(0x05);
	write_data8(0x3C);
	write_data8(0x3C);
	write_reg_addr(0xB2);
	write_data8(0x05);
	write_data8(0x3C);
	write_data8(0x3C);
	write_reg_addr(0xB3);
	write_data8(0x05);
	write_data8(0x3C);
	write_data8(0x3C);
	write_data8(0x05);
	write_data8(0x3C);
	write_data8(0x3C);
	//--------------END ST7735S Frame Rate--------------------------//
	write_reg_addr(0xB4); //Dot inversion
	write_data8(0x03);
	write_reg_addr(0xC0);
	write_data8(0x28);
	write_data8(0x08);
	write_data8(0x04);
	write_reg_addr(0xC1);
	write_data8(0XC0);
	write_reg_addr(0xC2);
	write_data8(0x0D);
	write_data8(0x00);
	write_reg_addr(0xC3);
	write_data8(0x8D);
	write_data8(0x2A);
	write_reg_addr(0xC4);
	write_data8(0x8D);
	write_data8(0xEE);
	//------------------End ST7735S Power Sequence-------------------//
	write_reg_addr(0xC5); //VCOM
	write_data8(0x1A);
	write_reg_addr(0x36); //MX, MY, RGB mode
	write_data8(0xC0);
	//--------------------ST7735S Gamma Sequence---------------//
	write_reg_addr(0xE0);
	write_data8(0x04);
	write_data8(0x22);
	write_data8(0x07);
	write_data8(0x0A);
	write_data8(0x2E);
	write_data8(0x30);
	write_data8(0x25);
	write_data8(0x2A);
	write_data8(0x28);
	write_data8(0x26);
	write_data8(0x2E);
	write_data8(0x3A);
	write_data8(0x00);
	write_data8(0x01);
	write_data8(0x03);
	write_data8(0x13);
	write_reg_addr(0xE1);
	write_data8(0x04);
	write_data8(0x16);
	write_data8(0x06);
	write_data8(0x0D);
	write_data8(0x2D);
	write_data8(0x26);
	write_data8(0x23);
	write_data8(0x27);
	write_data8(0x27);
	write_data8(0x25);
	write_data8(0x2D);
	write_data8(0x3B);
	write_data8(0x00);
	write_data8(0x01);
	write_data8(0x04);
	write_data8(0x13);
	//----------------End ST7735S Gamma Sequence------------------//
	write_reg_addr(0x3A); //65k mode
	write_data8(0x05);
	write_reg_addr(0x29); //Display on
}

//清屏函数
//Color:要清屏的填充色
void LCD_Clear(void)
{
	u16 i,j;

	address_set(0,0,LCD_W-1,LCD_H-1);
	for(i=0;i<LCD_W;i++)
	{
		for (j=0;j<LCD_H;j++)
		{
			write_data16(g_background_color);
		}
	}
}

//画点
//POINT_COLOR:此点的颜色
void LCD_DrawPoint(u16 x,u16 y)
{
	address_set(x,y,x,y);//设置光标位置 
	write_data16(g_foreground_color);		    
}		 

//在指定区域内填充指定颜色
//区域大小:
//  (xend-xsta)*(yend-ysta)
void LCD_Fill(u16 xsta,u16 ysta,u16 xend,u16 yend)
{          
	u16 i,j; 

	address_set(xsta,ysta,xend,yend);      //设置光标位置 
	for(i=ysta;i<=yend;i++) {			
		for(j=xsta;j<=xend;j++)
			write_data16(g_foreground_color);//设置光标位置	    
	}							    
}  

//画线
//x1,y1:起点坐标
//x2,y2:终点坐标  
void LCD_DrawLine(u16 x1, u16 y1, u16 x2, u16 y2)
{
	u16 t; 
	int xerr=0,yerr=0,delta_x,delta_y,distance; 
	int incx,incy,uRow,uCol; 

	delta_x=x2-x1; //计算坐标增量 
	delta_y=y2-y1; 
	uRow=x1; 
	uCol=y1; 
	if(delta_x>0)incx=1; //设置单步方向 
	else if(delta_x==0)incx=0;//垂直线 
	else {incx=-1;delta_x=-delta_x;} 
	if(delta_y>0)incy=1; 
	else if(delta_y==0)incy=0;//水平线 
	else{incy=-1;delta_y=-delta_y;} 
	if( delta_x>delta_y)distance=delta_x; //选取基本增量坐标轴 
	else distance=delta_y; 
	for(t=0;t<=distance+1;t++ )//画线输出 
	{  
		LCD_DrawPoint(uRow,uCol);//画点 
		xerr+=delta_x ; 
		yerr+=delta_y ; 
		if(xerr>distance) 
		{ 
			xerr-=distance; 
			uRow+=incx; 
		} 
		if(yerr>distance) 
		{ 
			yerr-=distance; 
			uCol+=incy; 
		} 
	}  
}    

//画矩形
void LCD_DrawRectangle(u16 x1, u16 y1, u16 x2, u16 y2)
{
	LCD_DrawLine(x1,y1,x2,y1);
	LCD_DrawLine(x1,y1,x1,y2);
	LCD_DrawLine(x1,y2,x2,y2);
	LCD_DrawLine(x2,y1,x2,y2);
}

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
	background_color_set(WHITE);	
	lcd_init();
	LCD_Clear();
	BACKLIGHT_PIN_ON();

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
