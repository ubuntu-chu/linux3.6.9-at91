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

#include <linux/ioctl.h>


/* 定义幻数 */
#define ST7735IOC_MAGIC  'k'

typedef unsigned int			uint32;
typedef int						int32;
typedef unsigned short int		uint16;
typedef short int				int16;
typedef unsigned char			uint8;
typedef char					int8;

struct font{
	uint8       name[10];
	uint8       width;
	uint8       height;
	uint8       byteperline;
	uint16      size;
	uint8       *data;
};
typedef struct font  GUI_FONT; 

struct font_set{
	uint8       name[10];
	uint8       width;
	uint8       height;
	uint8       byteperline;
	uint16      size;
	uint8       data[0];
};

#define  GUI_CONST_STORAGE
#define  GUI_DRAW_BMPM565   0

struct bitmap{
	uint16		x_size;
	uint16		y_size;
	uint16		bytesperline;
	uint16		bitsperPixel;
	uint8		*data;
	uint8		*palette;
	uint8		*methods;
};
typedef struct bitmap  GUI_BITMAP; 

enum{
	NR_INIT = 0,
	NR_UNINT,
	NR_SET_FOREGROUND_COLOR,
	NR_SET_BACKGROUND_COLOR,
	NR_CLEAR,
	NR_DRAW_POINT,
	NR_DRAW_LINE,
	NR_CLEAR_RECTANGLE,
	NR_DRAW_RECTANGLE,
	NR_FILL_RECTANGLE,
	NR_SHOW_STRING,
	NR_DRAW_CIRCLE,
	NR_FILL_CIRCLE,
	NR_DRAW_ELLIPSE,
	NR_FILL_ELLIPSE,
	NR_DRAW_ARC,
	//NR_DISPLAY_ON,
	//NR_DISPLAY_OFF,
	NR_SET_FONT_HEAD,
	NR_SET_FONT,
	NR_ADD_FONT,
	NR_DRAW_BITMAP,
	NR_BACKLIGHT_ON,
	NR_BACKLIGHT_OFF,
	NR_LCD_INFO,


	NR_IOC_MAX,
};
struct lcd_info
{
	uint32			m_width;
	uint32			m_height;
	uint32			m_bpp;
};

struct color
{
	uint16		m_color;
};
struct coordinate                                     
{                                                        
	uint16		m_x;
	uint16		m_y;
};
struct coordinate_pair                                     
{                                                        
	uint16		m_x1;
	uint16		m_y1;
	uint16		m_x2;
	uint16		m_y2;
};
struct circle                                     
{                                                        
	uint16		m_x;
	uint16		m_y;
	uint8		m_r;
};
struct arc                                     
{                                                        
	uint16		m_x;
	uint16		m_y;
	uint16		m_r;
	uint16		m_stangle;
	uint16		m_endangle;
};

struct pict
{
	uint16		m_x;
	uint16		m_y;
	uint16		m_x_size;
	uint16		m_y_size;
};
struct string
{
	uint16		m_x;
	uint16		m_y;
	uint8      *m_data;
	uint16		m_len;
};

/* 定义命令 */
#define ST7735IOC_INIT			_IO(ST7735IOC_MAGIC, NR_INIT)
#define ST7735IOC_UNINIT		_IO(ST7735IOC_MAGIC, NR_UNINT)
#define ST7735IOC_BACKLIGHT_ON	_IO(ST7735IOC_MAGIC, NR_BACKLIGHT_ON)
#define ST7735IOC_BACKLIGHT_OFF	_IO(ST7735IOC_MAGIC, NR_BACKLIGHT_OFF)
//#define ST7735IOC_DISPLAY_ON	_IO(ST7735IOC_MAGIC, NR_DISPLAY_ON)
//#define ST7735IOC_DISPLAY_OFF	_IO(ST7735IOC_MAGIC, NR_DISPLAY_OFF)
#define ST7735IOC_SET_FOREGROUND_COLOR	_IOW(ST7735IOC_MAGIC, \
									NR_SET_FOREGROUND_COLOR, struct color)
#define ST7735IOC_SET_BACKGROUND_COLOR	_IOW(ST7735IOC_MAGIC, \
									NR_SET_BACKGROUND_COLOR, struct color)
#define ST7735IOC_CLEAR			_IO(ST7735IOC_MAGIC, NR_CLEAR)
#define ST7735IOC_LCD_INFO	_IOR(ST7735IOC_MAGIC, \
											NR_LCD_INFO, struct lcd_info)
#define ST7735IOC_DRAW_POINT	_IOW(ST7735IOC_MAGIC, \
									NR_DRAW_POINT, struct coordinate)
#define ST7735IOC_DRAW_LINE	_IOW(ST7735IOC_MAGIC, \
									NR_DRAW_LINE, struct coordinate_pair)
#define ST7735IOC_CLEAR_RECTANGLE	_IOW(ST7735IOC_MAGIC, \
									NR_CLEAR_RECTANGLE, struct coordinate_pair)
#define ST7735IOC_DRAW_RECTANGLE	_IOW(ST7735IOC_MAGIC, \
									NR_DRAW_RECTANGLE, struct coordinate_pair)
#define ST7735IOC_FILL_RECTANGLE	_IOW(ST7735IOC_MAGIC, \
									NR_FILL_RECTANGLE, struct coordinate_pair)
#define ST7735IOC_DRAW_CIRCLE	_IOW(ST7735IOC_MAGIC, \
									NR_DRAW_CIRCLE, struct circle)
#define ST7735IOC_FILL_CIRCLE	_IOW(ST7735IOC_MAGIC, \
									NR_FILL_CIRCLE, struct circle)
#define ST7735IOC_DRAW_ELLIPSE	_IOW(ST7735IOC_MAGIC, \
									NR_DRAW_ELLIPSE, struct coordinate_pair)
#define ST7735IOC_FILL_ELLIPSE	_IOW(ST7735IOC_MAGIC, \
									NR_FILL_ELLIPSE, struct coordinate_pair)
#define ST7735IOC_DRAW_ARC		_IOW(ST7735IOC_MAGIC, \
									NR_DRAW_ARC, struct arc)

#define ST7735IOC_DRAW_BITMAP	_IOW(ST7735IOC_MAGIC, \
									NR_DRAW_BITMAP, struct pict)

#define ST7735IOC_SET_FONT		_IO(ST7735IOC_MAGIC, \
									NR_SET_FONT)
#define ST7735IOC_ADD_FONT		_IO(ST7735IOC_MAGIC, \
									NR_ADD_FONT)
#define ST7735IOC_SHOW_STRING	_IOW(ST7735IOC_MAGIC, \
									NR_SHOW_STRING, struct string)





#endif


