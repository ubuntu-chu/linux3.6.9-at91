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

#ifndef _ST7735_H_
#define _ST7735_H_

/* #define DEBUG */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/atomic.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/mutex.h>
#include <linux/cdev.h>
#include <linux/fs.h>  
#include <asm/uaccess.h> 

#include <linux/delay.h>
#include <linux/device.h>
#include <linux/spi/spi.h>
#include <linux/freezer.h>

#include <linux/delay.h>
#include <linux/slab.h>

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
	struct cdev cdev;
	dev_t devno;
	struct mutex mutex;

	u16        *frame_buff;

	atomic_t    open_cnt;

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

#define LCD_ROTATE		(1)

#if (LCD_ROTATE > 0)
//定义LCD的尺寸
#define LCD_W			160
#define LCD_H			128
#else
//定义LCD的尺寸
#define LCD_W			128
#define LCD_H			160
#endif

#define LCD_FRAME_BUFF_SIZE   (LCD_W*LCD_H*2)

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

#define COLOR_CC(x)   ((x >> 8) | ((x&0xff) << 8))

extern u16 g_foreground_color;
extern u16 g_background_color;
extern struct st7735_chip *paint_device;

extern unsigned char image[];
extern unsigned char hanzi[];
extern unsigned char asc2_1608[1520];

void paint_device_set(struct st7735_chip *pd);
u16 foreground_color_set(u16 color);
u16 foreground_color_get(void);
u16 background_color_set(u16 color);
u16 background_color_get(void);


void lcd_init(void);
//清屏函数
//Color:要清屏的填充色
void LCD_Clear(void);
//画点
//POINT_COLOR:此点的颜色
void LCD_DrawPoint(u16 x,u16 y);

//在指定区域内填充指定颜色
//区域大小:
//  (xend-xsta)*(yend-ysta)
void LCD_Fill(u16 xsta,u16 ysta,u16 xend,u16 yend);
//画线
//x1,y1:起点坐标
//x2,y2:终点坐标  
void LCD_DrawLine(u16 x1, u16 y1, u16 x2, u16 y2);
//画矩形
void LCD_DrawRectangle(u16 x1, u16 y1, u16 x2, u16 y2);
void LCD_ShowChar(u16 x,u16 y,u8 num,u8 mode);//显示一个字符
void LCD_ShowNum(u16 x,u16 y,u32 num,u8 len);//显示数字
void LCD_Show2Num(u16 x,u16 y,u16 num,u8 len);//显示2个数字
void LCD_ShowString(u16 x,u16 y,const u8 *p); //显示一个字符串,16字体

void LCD_DrawCircle(u16 x0,u16 y0,u8 r);
void LCD_DisplayOn(void);
void LCD_DisplayOff(void);

#endif


