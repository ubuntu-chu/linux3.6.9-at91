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


u16 g_foreground_color = 0x0000;
u16 g_background_color = 0xffff;
struct st7735_chip *paint_device;
struct font_set *pfont = NULL;
struct list_head font_list_head;

void paint_device_set(struct st7735_chip *pd){
	paint_device	= pd;
}

struct font_set *font_find(const char *name)
{
	struct font_set *font_set  = NULL;
	struct font_node *font_node  = NULL;
	struct list_head  *pnode;

	list_for_each(pnode, &font_list_head){
		font_node	= list_entry(pnode, struct font_node, node);
		font_set	= font_node->font_set;
		printk("font name: %s\n", font_set->name);
		if (0 == strcmp(font_set->name, name)){
			break;
		}
	}
	if (pnode == &font_list_head){
		return NULL;
	}else{
		return font_set;
	}
}

int font_set(const char *name){

	struct font_set *font_set	= font_find(name);

	if (font_set == NULL){
		return -1;
	}
	pfont			= font_set;
	return 0;
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

static int write_data(u8 *data, u16 len)
{
	CD_PIN_HIGH();
	return spi_write(paint_device->spi, data, len);
}

static int write_data8(u8 data)
{
	return write_data(&data, sizeof(u8));
}

static int write_data16(u16 data)
{
	u8 out[2];

	CD_PIN_HIGH();
	out[0] = data >> 8;
	out[1] = data & 0x00ff;
	return write_data(out, sizeof(out));
}

static int write_reg_addr(u8 reg)
{
	u8 out[1];

	CD_PIN_LOW();
	out[0] = reg;
	return spi_write(paint_device->spi, out, sizeof(out));
}

#if 0
static int write_reg_data16(u8 reg, u16 data)
{
	write_reg_addr(reg);
	return write_data16(data);
}
#endif

static void address_set(u16 x1, u16 y1, u16 x2, u16 y2)
{
	u8 out[4];

	write_reg_addr(0x2a);
	out[0] = x1 >> 8;
	out[1] = x1 & 0x00ff;
	out[2] = x2 >> 8;
	out[3] = x2 & 0x00ff;
	write_data(out, sizeof(out));

	write_reg_addr(0x2b);
	out[0] = y1 >> 8;
	out[1] = y1 & 0x00ff;
	out[2] = y2 >> 8;
	out[3] = y2 & 0x00ff;
	write_data(out, sizeof(out));
	
	write_reg_addr(0x2c);
}

void lcd_init(void)
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
#if (LCD_ROTATE > 0)
	write_data8(0x60);
#else 
	write_data8(0xC0);
#endif
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
	write_data8(0x55);
	LCD_DisplayOn();
}

void LCD_DisplayOn(void){
	write_reg_addr(0x29); //Display on
}

void LCD_DisplayOff(void){
	write_reg_addr(0x28); //Display off
}

//清屏函数
//Color:要清屏的填充色
void LCD_Clear(void)
{
	u16   cc = foreground_color_set(g_background_color);

	LCD_FillRectangle(0,0,LCD_W-1,LCD_H-1);
	foreground_color_set(cc);
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
void LCD_FillRectangle(u16 xsta,u16 ysta,u16 xend,u16 yend)
{          
	u16 i,j, t; 
	u32 z = 0;
	u16   cc = COLOR_CC(g_foreground_color);

	if (ysta > yend){
		t		= ysta;
		ysta		= yend;
		yend		= t;
	}
	if (xsta > xend){
		t		= xsta;
		xsta		= xend;
		xend		= t;
	}
	address_set(xsta,ysta,xend,yend);
	for(i=ysta;i<=yend;i++) {			
		for(j=xsta;j<=xend;j++)
			paint_device->frame_buff[z++] = cc;
	}							    
	write_data((u8 *)paint_device->frame_buff, z<<1);
}  

void LCD_ClearRectangle(u16 xsta,u16 ysta,u16 xend,u16 yend)
{          
	u16   cc = foreground_color_set(g_background_color);

	LCD_FillRectangle(xsta, ysta, xend, yend);
	foreground_color_set(cc);
}  

#define GUI_HLine(x1, y1, x2, color)  LCD_DrawLine(x1,y1,x2,y1)

//画线
//x1,y1:起点坐标
//x2,y2:终点坐标  
void LCD_DrawLine(u16 x1, u16 y1, u16 x2, u16 y2)
{
	int xerr=0,yerr=0,delta_x,delta_y,distance; 
	int incx,incy,uRow,uCol; 
	u16  t;

	delta_x=x2-x1; //计算坐标增量 
	delta_y=y2-y1; 
	uRow=x1; 
	uCol=y1; 
	if(delta_x>0)
		incx=1; //设置单步方向 
	else if(delta_x==0)
		incx=0;//垂直线 
	else {
		incx=-1;delta_x=-delta_x;
	} 

	if(delta_y>0)
		incy=1; 
	else if(delta_y==0)
		incy=0;//水平线 
	else{
		incy=-1;delta_y=-delta_y;
	} 

	if( delta_x>delta_y)
		distance=delta_x; //选取基本增量坐标轴 
	else 
		distance=delta_y; 

	//vline
	if ((incy == 0) || (incx == 0)){
		LCD_FillRectangle(x1, y1, x2, y2);
	}else {
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
}    

//画矩形
void LCD_DrawRectangle(u16 x1, u16 y1, u16 x2, u16 y2)
{
	LCD_DrawLine(x1,y1,x2,y1);
	LCD_DrawLine(x1,y1,x1,y2);
	LCD_DrawLine(x1,y2,x2,y2);
	LCD_DrawLine(x2,y1,x2,y2);
}

//在指定位置画一个指定大小的圆
//(x,y):中心点
//r    :半径
void  LCD_DrawCircle(u16 x0, u16 y0, u8 r)
{  
#define GUI_Point(x,y,z)    LCD_DrawPoint(x,y)       

	int32  draw_x0, draw_y0;// 刽图点坐标变量
	int32  draw_x1, draw_y1;
	int32  draw_x2, draw_y2;
	int32  draw_x3, draw_y3;
	int32  draw_x4, draw_y4;
	int32  draw_x5, draw_y5;
	int32  draw_x6, draw_y6;
	int32  draw_x7, draw_y7;
	int32  xx, yy;// 画圆控制变量

	int32  di;// 决策变量
	//u16   color = COLOR_CC(g_foreground_color);

	/* 参数过滤 */
	if(0==r) return;

	/* 计算出8个特殊点(0、45、90、135、180、225、270度)，进行显示 */
	draw_x0 = draw_x1 = x0;
	draw_y0 = draw_y1 = y0 + r;
	if(draw_y0<GUI_LCM_YMAX) GUI_Point(draw_x0, draw_y0, color);// 90度

	draw_x2 = draw_x3 = x0;
	draw_y2 = draw_y3 = y0 - r;
	if(draw_y2>=0) GUI_Point(draw_x2, draw_y2, color);// 270度


	draw_x4 = draw_x6 = x0 + r;
	draw_y4 = draw_y6 = y0;
	if(draw_x4<GUI_LCM_XMAX) GUI_Point(draw_x4, draw_y4, color);// 0度

	draw_x5 = draw_x7 = x0 - r;
	draw_y5 = draw_y7 = y0;
	if(draw_x5>=0) GUI_Point(draw_x5, draw_y5, color);// 180度   
	if(1==r) return;// 若半径为1，则已圆画完


	/* 使用Bresenham法进行画圆 */
	di = 3 - 2*r;// 初始化决策变量

	xx = 0;
	yy = r;
	while(xx<yy)
	{  
		if(di<0)
		{  di += 4*xx + 6;      
		}
		else
		{  di += 4*(xx - yy) + 10;

		yy--;  
		draw_y0--;
		draw_y1--;
		draw_y2++;
		draw_y3++;
		draw_x4--;
		draw_x5++;
		draw_x6--;
		draw_x7++;		
	}

	xx++;   
	draw_x0++;
	draw_x1--;
	draw_x2++;
	draw_x3--;
	draw_y4++;
	draw_y5++;
	draw_y6--;
	draw_y7--;


	/* 要判断当前点是否在有效范围内 */
	if( (draw_x0<=GUI_LCM_XMAX)&&(draw_y0>=0) )
	{  GUI_Point(draw_x0, draw_y0, color);
	}    
	if( (draw_x1>=0)&&(draw_y1>=0) )
	{  GUI_Point(draw_x1, draw_y1, color);
	}
	if( (draw_x2<=GUI_LCM_XMAX)&&(draw_y2<=GUI_LCM_YMAX) )
	{  GUI_Point(draw_x2, draw_y2, color);   
	}
	if( (draw_x3>=0)&&(draw_y3<=GUI_LCM_YMAX) )
	{  GUI_Point(draw_x3, draw_y3, color);
	}
	if( (draw_x4<=GUI_LCM_XMAX)&&(draw_y4>=0) )
	{  GUI_Point(draw_x4, draw_y4, color);
	}
	if( (draw_x5>=0)&&(draw_y5>=0) )
	{  GUI_Point(draw_x5, draw_y5, color);
	}
	if( (draw_x6<=GUI_LCM_XMAX)&&(draw_y6<=GUI_LCM_YMAX) )
	{  GUI_Point(draw_x6, draw_y6, color);
	}
	if( (draw_x7>=0)&&(draw_y7<=GUI_LCM_YMAX) )
	{  GUI_Point(draw_x7, draw_y7, color);
	}
	}
}

/****************************************************************************
* 名称：GUI_CircleFill()
* 功能：指定圆心位置及半径，画圆并填充，填充色与边框色一样。
* 入口参数： x0		圆心的x坐标值
*           y0		圆心的y坐标值
*           r       圆的半径
*           color	填充颜色
* 出口参数：无
* 说明：操作失败原因是指定地址超出有效范围。
****************************************************************************/

void  LCD_FillCircle(u16 x0, u16 y0, u8 r)
{  int32  draw_x0, draw_y0;			// 刽图点坐标变量
   int32  draw_x1, draw_y1;	
   int32  draw_x2, draw_y2;	
   int32  draw_x3, draw_y3;	
   int32  draw_x4, draw_y4;	
   int32  draw_x5, draw_y5;	
   int32  draw_x6, draw_y6;	
   int32  draw_x7, draw_y7;	
   int32  fill_x0, fill_y0;			// 填充所需的变量，使用垂直线填充
   int32  fill_x1;
   int32  xx, yy;					// 画圆控制变量
 
   int32  di;						// 决策变量
   
   /* 参数过滤 */
   if(0==r) return;
   
   /* 计算出4个特殊点(0、90、180、270度)，进行显示 */
   draw_x0 = draw_x1 = x0;
   draw_y0 = draw_y1 = y0 + r;
   if(draw_y0<GUI_LCM_YMAX)
   {  GUI_Point(draw_x0, draw_y0, color);	// 90度
   }
    	
   draw_x2 = draw_x3 = x0;
   draw_y2 = draw_y3 = y0 - r;
   if(draw_y2>=0)
   {  GUI_Point(draw_x2, draw_y2, color);	// 270度
   }
  	
   draw_x4 = draw_x6 = x0 + r;
   draw_y4 = draw_y6 = y0;
   if(draw_x4<GUI_LCM_XMAX) 
   {  GUI_Point(draw_x4, draw_y4, color);	// 0度
      fill_x1 = draw_x4;
   }
   else
   {  fill_x1 = GUI_LCM_XMAX;
   }
   fill_y0 = y0;							// 设置填充线条起始点fill_x0
   fill_x0 = x0 - r;						// 设置填充线条结束点fill_y1
   if(fill_x0<0) fill_x0 = 0;
   GUI_HLine(fill_x0, fill_y0, fill_x1, color);
   
   draw_x5 = draw_x7 = x0 - r;
   draw_y5 = draw_y7 = y0;
   if(draw_x5>=0) 
   {  GUI_Point(draw_x5, draw_y5, color);	// 180度
   }
   if(1==r) return;
   
   
   /* 使用Bresenham法进行画圆 */
   di = 3 - 2*r;							// 初始化决策变量
   
   xx = 0;
   yy = r;
   while(xx<yy)
   {  if(di<0)
	  {  di += 4*xx + 6;
	  }
	  else
	  {  di += 4*(xx - yy) + 10;
	  
	     yy--;	  
		 draw_y0--;
		 draw_y1--;
		 draw_y2++;
		 draw_y3++;
		 draw_x4--;
		 draw_x5++;
		 draw_x6--;
		 draw_x7++;		 
	  }
	  
	  xx++;   
	  draw_x0++;
	  draw_x1--;
	  draw_x2++;
	  draw_x3--;
	  draw_y4++;
	  draw_y5++;
	  draw_y6--;
	  draw_y7--;
		
	
	  /* 要判断当前点是否在有效范围内 */
	  if( (draw_x0<=GUI_LCM_XMAX)&&(draw_y0>=0) )	
	  {  GUI_Point(draw_x0, draw_y0, color);
	  }	    
	  if( (draw_x1>=0)&&(draw_y1>=0) )	
	  {  GUI_Point(draw_x1, draw_y1, color);
	  }
	  
	  /* 第二点水直线填充(下半圆的点) */
	  if(draw_x1>=0)
	  {  /* 设置填充线条起始点fill_x0 */
	     fill_x0 = draw_x1;
	     /* 设置填充线条起始点fill_y0 */
	     fill_y0 = draw_y1;
         if(fill_y0>GUI_LCM_YMAX) fill_y0 = GUI_LCM_YMAX;
         if(fill_y0<0) fill_y0 = 0; 
         /* 设置填充线条结束点fill_x1 */									
         fill_x1 = x0*2 - draw_x1;				
         if(fill_x1>GUI_LCM_XMAX) fill_x1 = GUI_LCM_XMAX;
         GUI_HLine(fill_x0, fill_y0, fill_x1, color);
      }
	  
	  
	  if( (draw_x2<=GUI_LCM_XMAX)&&(draw_y2<=GUI_LCM_YMAX) )	
	  {  GUI_Point(draw_x2, draw_y2, color);   
	  }
	    	  
	  if( (draw_x3>=0)&&(draw_y3<=GUI_LCM_YMAX) )	
	  {  GUI_Point(draw_x3, draw_y3, color);
	  }
	  
	  /* 第四点垂直线填充(上半圆的点) */
	  if(draw_x3>=0)
	  {  /* 设置填充线条起始点fill_x0 */
	     fill_x0 = draw_x3;
	     /* 设置填充线条起始点fill_y0 */
	     fill_y0 = draw_y3;
         if(fill_y0>GUI_LCM_YMAX) fill_y0 = GUI_LCM_YMAX;
         if(fill_y0<0) fill_y0 = 0;
         /* 设置填充线条结束点fill_x1 */									
         fill_x1 = x0*2 - draw_x3;				
         if(fill_x1>GUI_LCM_XMAX) fill_x1 = GUI_LCM_XMAX;
         GUI_HLine(fill_x0, fill_y0, fill_x1, color);
      }
	  
	  	  
	  if( (draw_x4<=GUI_LCM_XMAX)&&(draw_y4>=0) )	
	  {  GUI_Point(draw_x4, draw_y4, color);
	  }
	  if( (draw_x5>=0)&&(draw_y5>=0) )	
	  {  GUI_Point(draw_x5, draw_y5, color);
	  }
	  
	  /* 第六点垂直线填充(上半圆的点) */
	  if(draw_x5>=0)
	  {  /* 设置填充线条起始点fill_x0 */
	     fill_x0 = draw_x5;
	     /* 设置填充线条起始点fill_y0 */
	     fill_y0 = draw_y5;
         if(fill_y0>GUI_LCM_YMAX) fill_y0 = GUI_LCM_YMAX;
         if(fill_y0<0) fill_y0 = 0;
         /* 设置填充线条结束点fill_x1 */									
         fill_x1 = x0*2 - draw_x5;				
         if(fill_x1>GUI_LCM_XMAX) fill_x1 = GUI_LCM_XMAX;
         GUI_HLine(fill_x0, fill_y0, fill_x1, color);
      }
	  
	  
	  if( (draw_x6<=GUI_LCM_XMAX)&&(draw_y6<=GUI_LCM_YMAX) )	
	  {  GUI_Point(draw_x6, draw_y6, color);
	  }
	  
	  if( (draw_x7>=0)&&(draw_y7<=GUI_LCM_YMAX) )	
	  {  GUI_Point(draw_x7, draw_y7, color);
	  }
	  
	  /* 第八点垂直线填充(上半圆的点) */
	  if(draw_x7>=0)
	  {  /* 设置填充线条起始点fill_x0 */
	     fill_x0 = draw_x7;
	     /* 设置填充线条起始点fill_y0 */
	     fill_y0 = draw_y7;
         if(fill_y0>GUI_LCM_YMAX) fill_y0 = GUI_LCM_YMAX;
         if(fill_y0<0) fill_y0 = 0;
         /* 设置填充线条结束点fill_x1 */									
         fill_x1 = x0*2 - draw_x7;				
         if(fill_x1>GUI_LCM_XMAX) fill_x1 = GUI_LCM_XMAX;
         GUI_HLine(fill_x0, fill_y0, fill_x1, color);
      }
	  
   }
}

/****************************************************************************
* 名称：GUI_Ellipse()
* 功能：画正椭圆。给定椭圆的四个点的参数，最左、最右点的x轴坐标值为x0、x1，最上、最下点
*      的y轴坐标为y0、y1。
* 入口参数： x0		最左点的x坐标值
*           x1		最右点的x坐标值
*           y0		最上点的y坐标值
*           y1      最下点的y坐标值
*           color	显示颜色
* 出口参数：无
* 说明：操作失败原因是指定地址超出有效范围。
****************************************************************************/
void  LCD_DrawEllipse(uint32 x0, uint32 x1, uint32 y0, uint32 y1)
{  int32  draw_x0, draw_y0;			// 刽图点坐标变量
   int32  draw_x1, draw_y1;
   int32  draw_x2, draw_y2;
   int32  draw_x3, draw_y3;
   int32  xx, yy;					// 画图控制变量
    
   int32  center_x, center_y;		// 椭圆中心点坐标变量
   int32  radius_x, radius_y;		// 椭圆的半径，x轴半径和y轴半径
   int32  radius_xx, radius_yy;		// 半径乘平方值
   int32  radius_xx2, radius_yy2;	// 半径乘平方值的两倍
   int32  di;						// 定义决策变量
	
   /* 参数过滤 */
   if( (x0==x1) || (y0==y1) ) return;
   	
   /* 计算出椭圆中心点坐标 */
   center_x = (x0 + x1) >> 1;			
   center_y = (y0 + y1) >> 1;
   
   /* 计算出椭圆的半径，x轴半径和y轴半径 */
   if(x0 > x1)
   {  radius_x = (x0 - x1) >> 1;
   }
   else
   {  radius_x = (x1 - x0) >> 1;
   }
   if(y0 > y1)
   {  radius_y = (y0 - y1) >> 1;
   }
   else
   {  radius_y = (y1 - y0) >> 1;
   }
		
   /* 计算半径平方值 */
   radius_xx = radius_x * radius_x;
   radius_yy = radius_y * radius_y;
	
   /* 计算半径平方值乘2值 */
   radius_xx2 = radius_xx<<1;
   radius_yy2 = radius_yy<<1;
	
   /* 初始化画图变量 */
   xx = 0;
   yy = radius_y;
  
   di = radius_yy2 + radius_xx - radius_xx2*radius_y ;	// 初始化决策变量 
	
   /* 计算出椭圆y轴上的两个端点坐标，作为作图起点 */
   draw_x0 = draw_x1 = draw_x2 = draw_x3 = center_x;
   draw_y0 = draw_y1 = center_y + radius_y;
   draw_y2 = draw_y3 = center_y - radius_y;
  
	 
   GUI_Point(draw_x0, draw_y0, color);					// 画y轴上的两个端点 
   GUI_Point(draw_x2, draw_y2, color);
	
   while( (radius_yy*xx) < (radius_xx*yy) ) 
   {  if(di<0)
	  {  di+= radius_yy2*(2*xx+3);
	  }
	  else
	  {  di += radius_yy2*(2*xx+3) + 4*radius_xx - 4*radius_xx*yy;
	 	  
	     yy--;
		 draw_y0--;
		 draw_y1--;
		 draw_y2++;
		 draw_y3++;				 
	  }
	  
	  xx ++;						// x轴加1
	 		
	  draw_x0++;
	  draw_x1--;
	  draw_x2++;
	  draw_x3--;
		
	  GUI_Point(draw_x0, draw_y0, color);
	  GUI_Point(draw_x1, draw_y1, color);
	  GUI_Point(draw_x2, draw_y2, color);
	  GUI_Point(draw_x3, draw_y3, color);
   }
  
   di = radius_xx2*(yy-1)*(yy-1) + radius_yy2*xx*xx + radius_yy + radius_yy2*xx - radius_xx2*radius_yy;
   while(yy>=0) 
   {  if(di<0)
	  {  di+= radius_xx2*3 + 4*radius_yy*xx + 4*radius_yy - 2*radius_xx2*yy;
	 	  
	     xx ++;						// x轴加1	 		
	     draw_x0++;
	     draw_x1--;
	     draw_x2++;
	     draw_x3--;  
	  }
	  else
	  {  di += radius_xx2*3 - 2*radius_xx2*yy;	 	 		     			 
	  }
	  
	  yy--;
 	  draw_y0--;
	  draw_y1--;
	  draw_y2++;
	  draw_y3++;	
		
	  GUI_Point(draw_x0, draw_y0, color);
	  GUI_Point(draw_x1, draw_y1, color);
	  GUI_Point(draw_x2, draw_y2, color);
	  GUI_Point(draw_x3, draw_y3, color);
   }     
}


/****************************************************************************
* 名称：GUI_EllipseFill()
* 功能：画正椭圆，并填充。给定椭圆的四个点的参数，最左、最右点的x轴坐标值为x0、x1，最上、最下点
*      的y轴坐标为y0、y1。
* 入口参数： x0		最左点的x坐标值
*           x1		最右点的x坐标值
*           y0		最上点的y坐标值
*           y1      最下点的y坐标值
*           color	填充颜色
* 出口参数：无
* 说明：操作失败原因是指定地址超出有效范围。
****************************************************************************/
void  LCD_FillEllipse(uint32 x0, uint32 x1, uint32 y0, uint32 y1)
{  int32  draw_x0, draw_y0;			// 刽图点坐标变量
   int32  draw_x1, draw_y1;
   int32  draw_x2, draw_y2;
   int32  draw_x3, draw_y3;
   int32  xx, yy;					// 画图控制变量
    
   int32  center_x, center_y;		// 椭圆中心点坐标变量
   int32  radius_x, radius_y;		// 椭圆的半径，x轴半径和y轴半径
   int32  radius_xx, radius_yy;		// 半径乘平方值
   int32  radius_xx2, radius_yy2;	// 半径乘平方值的两倍
   int32  di;						// 定义决策变量
	
   /* 参数过滤 */
   if( (x0==x1) || (y0==y1) ) return;
   
   /* 计算出椭圆中心点坐标 */
   center_x = (x0 + x1) >> 1;			
   center_y = (y0 + y1) >> 1;
   
   /* 计算出椭圆的半径，x轴半径和y轴半径 */
   if(x0 > x1)
   {  radius_x = (x0 - x1) >> 1;
   }
   else
   {  radius_x = (x1 - x0) >> 1;
   }
   if(y0 > y1)
   {  radius_y = (y0 - y1) >> 1;
   }
   else
   {  radius_y = (y1 - y0) >> 1;
   }
		
   /* 计算半径乘平方值 */
   radius_xx = radius_x * radius_x;
   radius_yy = radius_y * radius_y;
	
   /* 计算半径乘4值 */
   radius_xx2 = radius_xx<<1;
   radius_yy2 = radius_yy<<1;
   
    /* 初始化画图变量 */
   xx = 0;
   yy = radius_y;
  
   di = radius_yy2 + radius_xx - radius_xx2*radius_y ;	// 初始化决策变量 
	
   /* 计算出椭圆y轴上的两个端点坐标，作为作图起点 */
   draw_x0 = draw_x1 = draw_x2 = draw_x3 = center_x;
   draw_y0 = draw_y1 = center_y + radius_y;
   draw_y2 = draw_y3 = center_y - radius_y;
  
	 
   GUI_Point(draw_x0, draw_y0, color);					// 画y轴上的两个端点
   GUI_Point(draw_x2, draw_y2, color);
	
   while( (radius_yy*xx) < (radius_xx*yy) ) 
   {  if(di<0)
	  {  di+= radius_yy2*(2*xx+3);
	  }
	  else
	  {  di += radius_yy2*(2*xx+3) + 4*radius_xx - 4*radius_xx*yy;
	 	  
	     yy--;
		 draw_y0--;
		 draw_y1--;
		 draw_y2++;
		 draw_y3++;				 
	  }
	  
	  xx ++;						// x轴加1
	 		
	  draw_x0++;
	  draw_x1--;
	  draw_x2++;
	  draw_x3--;
		
	  GUI_Point(draw_x0, draw_y0, color);
	  GUI_Point(draw_x1, draw_y1, color);
	  GUI_Point(draw_x2, draw_y2, color);
	  GUI_Point(draw_x3, draw_y3, color);
	  
	  /* 若y轴已变化，进行填充 */
	  if(di>=0)
	  {  GUI_HLine(draw_x0, draw_y0, draw_x1, color);
	     GUI_HLine(draw_x2, draw_y2, draw_x3, color);
	  }
   }
  
   di = radius_xx2*(yy-1)*(yy-1) + radius_yy2*xx*xx + radius_yy + radius_yy2*xx - radius_xx2*radius_yy;
   while(yy>=0) 
   {  if(di<0)
	  {  di+= radius_xx2*3 + 4*radius_yy*xx + 4*radius_yy - 2*radius_xx2*yy;
	 	  
	     xx ++;						// x轴加1	 		
	     draw_x0++;
	     draw_x1--;
	     draw_x2++;
	     draw_x3--;  
	  }
	  else
	  {  di += radius_xx2*3 - 2*radius_xx2*yy;	 	 		     			 
	  }
	  
	  yy--;
 	  draw_y0--;
	  draw_y1--;
	  draw_y2++;
	  draw_y3++;	
		
	  GUI_Point(draw_x0, draw_y0, color);
	  GUI_Point(draw_x1, draw_y1, color);
	  GUI_Point(draw_x2, draw_y2, color);
	  GUI_Point(draw_x3, draw_y3, color);
	  
	  /* y轴已变化，进行填充 */
	  GUI_HLine(draw_x0, draw_y0, draw_x1, color);
	  GUI_HLine(draw_x2, draw_y2, draw_x3, color); 
   }     
}

void  LCD_DrawArc(uint32 x, uint32 y, uint32 r, uint32 stangle, uint32 endangle)
{  int32  draw_x, draw_y;					// 画图坐标变量
   int32  op_x, op_y;						// 操作坐标
   int32  op_2rr;							// 2*r*r值变量
   
   int32  pno_angle;						// 度角点的个数
   uint8  draw_on;							// 画点开关，为1时画点，为0时不画
   
   
   /* 参数过滤 */
   if(r==0) return;							// 半径为0则直接退出
   if(stangle==endangle) return;			// 起始角度与终止角度相同，退出
   if( (stangle>=360) || (endangle>=360) ) return;

   op_2rr = 2*r*r;							// 计算r平方乖以2
   pno_angle = 0;
   /* 先计算出在此半径下的45度的圆弧的点数 */       
   op_x = r;
   op_y = 0;
   while(1)
   {  pno_angle++; 							// 画点计数         
      /* 计算下一点 */
      op_y++;
      if( (2*op_x*op_x + 2*op_y*op_y - op_2rr - 2*op_x +1)>0 ) 	// 使用逐点比较法实现画圆弧
      {  op_x--;
      }
      if(op_y>=op_x) break;
   }
   
   draw_on = 0;								// 最开始关画点开关
   /* 设置起始点及终点 */
   if(endangle>stangle) draw_on = 1;		// 若终点大于起点，则从一开始即画点(359)
   stangle = (360-stangle)*pno_angle/45;
   endangle = (360-endangle)*pno_angle/45;
   if(stangle==0) stangle=1;
   if(endangle==0) endangle=1;
   
   /* 开始顺时针画弧，从359度开始(第4像限) */
   pno_angle = 0;
   
   draw_x = x+r;
   draw_y = y;         
   op_x = r;
   op_y = 0;
   while(1)
   {  /* 计算下一点 */
      op_y++;
      draw_y--;
      if( (2*op_x*op_x + 2*op_y*op_y - op_2rr - 2*op_x +1)>0 ) 	// 使用逐点比较法实现画圆弧
      {  op_x--;
         draw_x--;
      }
      if(draw_on==1) GUI_Point(draw_x, draw_y, color);			// 开始画图
      pno_angle++;
      if( (pno_angle==stangle)||(pno_angle==endangle) )			// 若遇到起点或终点，画点开关取反
      {  draw_on = 1-draw_on;
         if(draw_on==1) GUI_Point(draw_x, draw_y, color);
      } 
      if(op_y>=op_x)
      {  if(draw_on==1) GUI_Point(draw_x, draw_y, color);
         break;
      }
   }
   
   while(1)
   {  /* 计算下一点 */
      op_x--;
      draw_x--;
      if( (2*op_x*op_x + 2*op_y*op_y - op_2rr + 2*op_y +1)<=0 ) // 使用逐点比较法实现画圆弧
      {  op_y++;
         draw_y--;
      }
      if(draw_on==1) GUI_Point(draw_x, draw_y, color);			// 开始画图
      pno_angle++;
      if( (pno_angle==stangle)||(pno_angle==endangle) )			// 若遇到起点或终点，画点开关取反
      {  draw_on = 1-draw_on;
         if(draw_on==1) GUI_Point(draw_x, draw_y, color);
      } 
      
      if(op_x<=0)
      {  if(draw_on==1) GUI_Point(draw_x, draw_y, color);		// 开始画图
         break;
      }
   }
    
    
   /* 开始顺时针画弧，从269度开始(第3像限) */
   draw_y = y-r;
   draw_x = x;         
   op_y = r;
   op_x = 0;
   while(1)
   {  /* 计算下一点 */
      op_x++;
      draw_x--;
      if( (2*op_x*op_x + 2*op_y*op_y - op_2rr - 2*op_y +1)>0 ) // 使用逐点比较法实现画圆弧
      {  op_y--;
         draw_y++;
      }
      if(draw_on==1) GUI_Point(draw_x, draw_y, color);			// 开始画图
      pno_angle++;
      if( (pno_angle==stangle)||(pno_angle==endangle) )			// 若遇到起点或终点，画点开关取反
      {  draw_on = 1-draw_on;
         if(draw_on==1) GUI_Point(draw_x, draw_y, color);
      } 
      
      if(op_x>=op_y)
      {  if(draw_on==1) GUI_Point(draw_x, draw_y, color);		// 开始画图
         break;
      }
   }
   
   while(1)
   {  /* 计算下一点 */
      op_y--;
      draw_y++;
      if( (2*op_x*op_x + 2*op_y*op_y - op_2rr + 2*op_x +1)<=0 ) // 使用逐点比较法实现画圆弧
      {  op_x++;
         draw_x--;
      }
      if(draw_on==1) GUI_Point(draw_x, draw_y, color);			// 开始画图
      pno_angle++;
      if( (pno_angle==stangle)||(pno_angle==endangle) )			// 若遇到起点或终点，画点开关取反
      {  draw_on = 1-draw_on;
         if(draw_on==1) GUI_Point(draw_x, draw_y, color);
      } 
      if(op_y<=0)
      {  if(draw_on==1) GUI_Point(draw_x, draw_y, color);
         break;
      }
   }
   
   
   /* 开始顺时针画弧，从179度开始(第2像限) */
   draw_x = x-r;
   draw_y = y;         
   op_x = r;
   op_y = 0;
   while(1)
   {  /* 计算下一点 */
      op_y++;
      draw_y++;
      if( (2*op_x*op_x + 2*op_y*op_y - op_2rr - 2*op_x +1)>0 ) 	// 使用逐点比较法实现画圆弧
      {  op_x--;
         draw_x++;
      }
      if(draw_on==1) GUI_Point(draw_x, draw_y, color);			// 开始画图
      pno_angle++;
      if( (pno_angle==stangle)||(pno_angle==endangle) )			// 若遇到起点或终点，画点开关取反
      {  draw_on = 1-draw_on;
         if(draw_on==1) GUI_Point(draw_x, draw_y, color);
      } 
      if(op_y>=op_x)
      {  if(draw_on==1) GUI_Point(draw_x, draw_y, color);
         break;
      }
   }
   
   while(1)
   {  /* 计算下一点 */
      op_x--;
      draw_x++;
      if( (2*op_x*op_x + 2*op_y*op_y - op_2rr + 2*op_y +1)<=0 ) // 使用逐点比较法实现画圆弧
      {  op_y++;
         draw_y++;
      }
      if(draw_on==1) GUI_Point(draw_x, draw_y, color);			// 开始画图
      pno_angle++;
      if( (pno_angle==stangle)||(pno_angle==endangle) )			// 若遇到起点或终点，画点开关取反
      {  draw_on = 1-draw_on;
         if(draw_on==1) GUI_Point(draw_x, draw_y, color);
      } 
      
      if(op_x<=0)
      {  if(draw_on==1) GUI_Point(draw_x, draw_y, color);		// 开始画图
         break;
      }
   }
  
  
   /* 开始顺时针画弧，从89度开始(第1像限) */
   draw_y = y+r;
   draw_x = x;         
   op_y = r;
   op_x = 0;
   while(1)
   {  /* 计算下一点 */
      op_x++;
      draw_x++;
      if( (2*op_x*op_x + 2*op_y*op_y - op_2rr - 2*op_y +1)>0 ) // 使用逐点比较法实现画圆弧
      {  op_y--;
         draw_y--;
      }
      if(draw_on==1) GUI_Point(draw_x, draw_y, color);			// 开始画图
      pno_angle++;
      if( (pno_angle==stangle)||(pno_angle==endangle) )			// 若遇到起点或终点，画点开关取反
      {  draw_on = 1-draw_on;
         if(draw_on==1) GUI_Point(draw_x, draw_y, color);
      } 
      
      if(op_x>=op_y)
      {  if(draw_on==1) GUI_Point(draw_x, draw_y, color);		// 开始画图
         break;
      }
   }
   
   while(1)
   {  /* 计算下一点 */
      op_y--;
      draw_y--;
      if( (2*op_x*op_x + 2*op_y*op_y - op_2rr + 2*op_x +1)<=0 ) // 使用逐点比较法实现画圆弧
      {  op_x++;
         draw_x++;
      }
      if(draw_on==1) GUI_Point(draw_x, draw_y, color);			// 开始画图
      pno_angle++;
      if( (pno_angle==stangle)||(pno_angle==endangle) )			// 若遇到起点或终点，画点开关取反
      {  draw_on = 1-draw_on;
         if(draw_on==1) GUI_Point(draw_x, draw_y, color);
      } 
      if(op_y<=0)
      {  if(draw_on==1) GUI_Point(draw_x, draw_y, color);
         break;
      }
   }
   
}

//-----------------------------------------------------------------------------//
//
void LCD_DrawBitmap(u16 x, u16 y, GUI_BITMAP *pict)
{
	u16 endx, endy;
	//u16 i = 0, j = 0, z = 0;
	u16 *data;
	u16  x_size;
	
	if (pict == NULL){
		return;
	}
	if (x >= LCD_W){
		x				= 0;
	}
	if (y >= LCD_H){
		y				= 0;
	}
	x_size				= pict->x_size;
	endx	= x + x_size;
	endy    = y + pict->y_size;
	//printk("endy= %d  endx= %d\n", endy, endx);
	if (endx > LCD_W){
		endx	= LCD_W;
	}
	if (endy > LCD_H){
		endy	= LCD_H;
	}
	data		= (u16 *)pict->data;
	address_set(x, y, endx-1, endy-1);

#if 0
	//printk("endy-y = %d  endx-x = %d\n", endy-y, endx-x);
	for (j = 0; j < endy-y; ++j)
	{
		for (i = 0; i < endx-x; ++i){
			paint_device->frame_buff[z++] = COLOR_CC(data[j*x_size+i]);
		}
	}
	write_data((u8 *)paint_device->frame_buff, z<<1);
#else
	write_data((u8 *)paint_device->frame_buff,
			((uint16)(endy-y))*((uint16)(endx-x))<<1);
#endif
}

//--------------------------------------------------------------------//

//在指定位置显示一个字符

//num:要显示的字符:" "--->"~"
//mode:叠加方式(1)还是非叠加方式(0)
//在指定位置显示一个字符

//num:要显示的字符:" "--->"~"

//mode:叠加方式(1)还是非叠加方式(0)

uint8 const  DCB2HEX_TAB[8] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};

void LCD_ShowChar(u16 x,u16 y,u8 num,u8 mode)
{
	u8 temp;
	u8 pos,t;
	u8 *data;
	u32  i = 0;
	u16 color;
	u16 width, height, bytesperline;

	if (pfont == NULL){
		return;
	}
	width	= pfont->width;
	height  = pfont->height;
	bytesperline	= pfont->byteperline;
	if(x>LCD_W-width || y>LCD_H-height) return;    
	if( (num<0x20) || (num>0x7f) ) num = 0x20;

	//设置窗口   
	num=num-' ';//得到偏移后的值
	address_set(x,y,x+width-1,y+height-1);      //设置光标位置 
	data	= pfont->data;

	if(!mode) //非叠加方式
	{
		for(pos=0;pos<height;pos++)
		{ 
			for(t=0;t<width;t++)
			{                 
				if ((t & 0x07) == 0)
					temp = data[num*height*bytesperline+
								pos*bytesperline+(t>>3)]; 
				if(temp & DCB2HEX_TAB[t])
					color = g_foreground_color;
				else 
					color = g_background_color;

				paint_device->frame_buff[i++] = COLOR_CC(color);
			}
		}
		write_data((u8 *)paint_device->frame_buff, i<<1);
	}else//叠加方式
	{
	#if 0
		for(pos=0;pos<height;pos++)
		{
			temp = data[(u16)num*height*bytesperline+pos]; 
			for(t=0;t<width;t++)
			{                 
				if(temp&0x01)LCD_DrawPoint(x+t,y+pos);//画一个点     
					temp>>=1; 
			}
		}
	#endif
	}
}   

//显示字符串
//x,y:起点坐标  
//*p:字符串起始地址
//用16字体
void LCD_ShowString(u16 x,u16 y,const u8 *p)
{         
	u16 width, height;

	if (pfont == NULL){
		return;
	}
	width	= pfont->width;
	height  = pfont->height;

	while(*p!='\0')
	{       
		if(x>LCD_W-width)
		{
			x=0;y+=height;
		}
		if(y>LCD_H-height)
		{
			y=x=0;
		}
		LCD_ShowChar(x,y,*p,0);
		x+=width;
		p++;
	}  
}

void coordinate_check(struct coordinate *pcoordinate)
{
	if (pcoordinate->m_x >= LCD_W){
		pcoordinate->m_x	= LCD_W - 1;
	}
	if (pcoordinate->m_y >= LCD_H){
		pcoordinate->m_y	= LCD_H - 1;
	}
}

void coordinate_pair_check(struct coordinate_pair *pcoordinate_pair)
{
	if (pcoordinate_pair->m_x1 >= LCD_W){
		pcoordinate_pair->m_x1	= LCD_W - 1;
	}
	if (pcoordinate_pair->m_y1 >= LCD_H){
		pcoordinate_pair->m_y1	= LCD_H - 1;
	}
	if (pcoordinate_pair->m_x2 >= LCD_W){
		pcoordinate_pair->m_x2	= LCD_W - 1;
	}
	if (pcoordinate_pair->m_y2 >= LCD_H){
		pcoordinate_pair->m_y2	= LCD_H - 1;
	}
}
#if 0

#define B_BITS 5
#define G_BITS 6
#define R_BITS 5

#define R_MASK ((1 << R_BITS) -1)
#define G_MASK ((1 << G_BITS) -1)
#define B_MASK ((1 << B_BITS) -1)

uint16  LCD_Color2Index_565(uint32 Color) 
{  
	int r,g,b;
	r = (Color>> (8  - R_BITS)) & R_MASK;
	g = (Color>> (16 - G_BITS)) & G_MASK;
	b = (Color>> (24 - B_BITS)) & B_MASK;
	//return r + (g << R_BITS) + (b << (G_BITS + R_BITS));
	//rgb 
	return b + (g << R_BITS) + (r << (G_BITS + R_BITS));
}
#endif


