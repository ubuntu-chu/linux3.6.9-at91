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
	
#if (LCD_ROTATE > 0)
	write_data16(y1);
	write_data16(y2);
#else
	write_data16(x1);
	write_data16(x2);
#endif

	write_reg_addr(0x2b);
#if (LCD_ROTATE > 0)
	write_data16(LCD_H-1-x1);
	write_data16(LCD_H-1-x2);
#else
	write_data16(y1);
	write_data16(y2);
#endif
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

	#if !(LCD_ROTATE > 0)
		address_set(xsta,ysta,xend,yend);
	#endif
	for(i=ysta;i<=yend;i++) {			
		for(j=xsta;j<=xend;j++)
		#if (LCD_ROTATE > 0)
			LCD_DrawPoint(i, j);
		#else
			write_data16(g_foreground_color);		    
		#endif
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

//在指定位置画一个指定大小的圆
//(x,y):中心点
//r    :半径
void LCD_DrawCircle(u16 x0,u16 y0,u8 r)
{
	int a,b;
	int di;
	a=0;b=r;  
	di=3-(r<<1);             //判断下个点位置的标志
	while(a<=b)
	{
		LCD_DrawPoint(x0-b,y0-a);             //3           
		LCD_DrawPoint(x0+b,y0-a);             //0           
		LCD_DrawPoint(x0-a,y0+b);             //1       
		LCD_DrawPoint(x0-b,y0-a);             //7           
		LCD_DrawPoint(x0-a,y0-b);             //2             
		LCD_DrawPoint(x0+b,y0+a);             //4               
		LCD_DrawPoint(x0+a,y0-b);             //5
		LCD_DrawPoint(x0+a,y0+b);             //6 
		LCD_DrawPoint(x0-b,y0+a);             
		a++;
		//使用Bresenham算法画圆     
		if(di<0)di +=4*a+6;  
		else
		{
			di+=10+4*(a-b);   
			b--;
		} 
		LCD_DrawPoint(x0+a,y0+b);
	}
} 
//在指定位置显示一个字符

//num:要显示的字符:" "--->"~"
//mode:叠加方式(1)还是非叠加方式(0)
//在指定位置显示一个字符

//num:要显示的字符:" "--->"~"

//mode:叠加方式(1)还是非叠加方式(0)
void LCD_ShowChar(u16 x,u16 y,u8 num,u8 mode)
{
	u8 temp;
	u8 pos,t;
	u16 colortemp=g_foreground_color;      

#define FONT_WIDTH   (8)
#define FONT_HEIGHT   (16)

	//if(x>LCD_W-16||y>LCD_H-16)return;    
	//设置窗口   
	num=num-' ';//得到偏移后的值
#if !(LCD_ROTATE > 0)

	address_set(x,y,x+FONT_WIDTH-1,y+FONT_HEIGHT-1);      //设置光标位置 
#endif
	if(!mode) //非叠加方式
	{
		for(pos=0;pos<FONT_HEIGHT;pos++)
		{ 
			temp=asc2_1608[(u16)num*FONT_HEIGHT+pos]; //调用1608字体
			for(t=0;t<FONT_WIDTH;t++)
			{                 
				if(temp&0x01)g_foreground_color=colortemp;
				else g_foreground_color=g_background_color;

			#if (LCD_ROTATE > 0)

				LCD_DrawPoint(x+t,y+pos);
			#else
				write_data16(g_foreground_color);
			#endif
				temp>>=1; 
			}
		}
	}else//叠加方式
	{
		for(pos=0;pos<FONT_HEIGHT;pos++)
		{
			temp=asc2_1608[(u16)num*FONT_HEIGHT+pos]; //调用1608字体
			for(t=0;t<FONT_WIDTH;t++)
			{                 
				if(temp&0x01)LCD_DrawPoint(x+t,y+pos);//画一个点     
					temp>>=1; 
			}
		}
	}
	g_foreground_color=colortemp;				  
}   
//m^n函数
u32 mypow(u8 m,u8 n)
{
	u32 result=1; 
	while(n--)result*=m;    
	return result;
} 
//显示2个数字
//x,y :起点坐标 
//len :数字的位数
//color:颜色
//num:数值(0~4294967295);
void LCD_ShowNum(u16 x,u16 y,u32 num,u8 len)
{			
	u8 t,temp;
	u8 enshow=0;
	num=(u16)num;
	for(t=0;t<len;t++)
	{
		temp=(num/mypow(10,len-t-1))%10;
		if(enshow==0&&t<(len-1))
		{
			if(temp==0)
			{
			LCD_ShowChar(x+8*t,y,' ',0);
			continue;
			}else enshow=1; 
		}
		LCD_ShowChar(x+8*t,y,temp+48,0); 
	}
} 
//显示2个数字
//x,y:起点坐标
//num:数值(0~99); 
void LCD_Show2Num(u16 x,u16 y,u16 num,u8 len)
{			
	u8 t,temp;   
	for(t=0;t<len;t++)
	{
		temp=(num/mypow(10,len-t-1))%10;
		LCD_ShowChar(x+8*t,y,temp+'0',0); 
	}
} 
//显示字符串
//x,y:起点坐标  
//*p:字符串起始地址
//用16字体
void LCD_ShowString(u16 x,u16 y,const u8 *p)
{         
	while(*p!='\0')
	{       
		if(x>LCD_W-16){x=0;y+=16;}
		if(y>LCD_H-16){y=x=0;}
		LCD_ShowChar(x,y,*p,0);
		x+=8;
		p++;
	}  
}



