#include <reg52.h>
#include <intrins.h>
#define MAIN_Fosc 11059200UL
typedef unsigned char uchar;
typedef unsigned int uint;
typedef unsigned char INT8U;
typedef unsigned int INT16U;
sbit RS = P3 ^ 5;
sbit RW = P3 ^ 6;
sbit EN = P3 ^ 4;
sbit cc = P2 ^ 3;
sbit DS = P2 ^ 2;	// DS18B20单总线
sbit key1 = P3 ^ 0; // 开关0
sbit key2 = P3 ^ 1; // 开关1
sbit key3 = P3 ^ 2; // 开关2
sbit key4 = P3 ^ 3; // 开关3
/*====================================
函数：void Delay_Ms(INT16U ms)
参数：ms，毫秒延时形参
描述：12T 51单片机自适应主时钟毫秒级延时函数
====================================*/
void Delay_Ms(INT16U ms)
{
	INT16U i;
	do
	{
		i = MAIN_Fosc / 96000;
		while (--i)
			; // 96T per loop
	} while (--ms);
}

/*us延时函数，执行一次US--所需6.5us进入一次函数需要11.95us*/
void Delay_us(uchar us)
{
	while (us--)
		;
}

/*单总线初始化时序*/
bit ds_init()
{
	bit i;
	DS = 1;
	_nop_();
	DS = 0;
	Delay_us(75); // 拉低总线499.45us 挂接在总线上的18B20将会全部被复位
	DS = 1;		  // 释放总线
	Delay_us(4);  // 延时37.95us 等待18B20发回存在信号
	i = DS;
	Delay_us(20); // 141.95us
	DS = 1;
	_nop_();
	return (i);
}

/*写一个字节*/
void write_byte(uchar dat)
{
	uchar i;
	for (i = 0; i < 8; i++)
	{
		DS = 0;
		_nop_(); // 产生些时序
		DS = dat & 0x01;
		Delay_us(10); // 76.95us
		DS = 1;		  // 释放总线准备下一次数据写入
		_nop_();
		dat >>= 1;
	}
}

uchar read_byte()
{
	uchar i, j, dat;
	for (i = 0; i < 8; i++)
	{
		DS = 0;
		_nop_(); // 产生读时序
		DS = 1;
		_nop_(); // 释放总线
		j = DS;
		Delay_us(10); // 76.95us
		DS = 1;
		_nop_();
		dat = (j << 7) | (dat >> 1);
	}
	return (dat);
}

void read_wait()
{

	uchar wait;
	P0 = 0xff;
	RS = 0;
	RW = 1;
	for (;;)
	{
		EN = 1;
		wait = P0;
		EN = 0;
		if (!(wait & 0x80))
			break;
	}
}

void write_zl(uchar yjwtzl)
{
	read_wait();
	RS = 0;
	RW = 0;
	P0 = yjwtzl;
	EN = 1;
	EN = 0;
}

void write_sj(uchar yjwtsj)
{
	read_wait();
	RS = 1;
	RW = 0;
	P0 = yjwtsj;
	EN = 1;
	EN = 0;
}

void timer0inte1()
{
	EA = 1;
	ET0 = 0;
	TR0 = 1;
	TMOD = 0X01;
	TH0 = 0x00;
	TL0 = 0x00; // 定时15ms
	P1 = 0xff;
}
void timer1inte1()
{
	EA = 1;
	EX0 = 1;
	EX1 = 1;
	IT0 = 1;
	IT1 = 1;
}

void main()
{
	uint i;
	uint a;
	uint b;
	uint c;
	uchar L, M, HT, LT, HT1, HT2, LT1, LT2, A;
	HT = 30;
	LT = 15;
	timer0inte1();	  // 初始化定时器
	ds_init();		  // 初始化DS18B20
	write_byte(0xcc); // 发送跳跃ROM指令
	write_byte(0x4e); // 写暂存器指令
	write_byte(0x7f);
	write_byte(0xf7);
	write_byte(0x1f); // 配置工作在9位模式下
	ds_init();		  // 初始化DS18B20
	write_byte(0xcc); // 发送跳跃ROM指令
	write_byte(0x48);

	read_wait();
	write_zl(0x38);
	write_zl(0xc);
	//	write_zl(0x06);
	//	write_zl(0x01);
	while (1)
	{
		ds_init();		  // 初始化DS18B20
		write_byte(0xcc); // 发送跳跃ROM指令
		write_byte(0x44); // 发送温度转换指令
		ds_init();		  // 初始化DS18B20
		write_byte(0xcc); // 发送跳跃ROM指令
		write_byte(0xbe); // 读取DS18B20暂存器值
		L = read_byte();
		M = read_byte();
		i = M;
		i <<= 8;
		i |= L;
		i = i * 10 * 0.0625;

		a = i / 100;
		b = (i / 10) - (a * 10);
		c = i - (a * 100) - (b * 10);

		HT1 = HT / 10;
		HT2 = HT - HT1 * 10;
		LT1 = LT / 10;
		LT2 = LT - LT1 * 10;

		write_zl(0x01);
		write_sj(a + '0');
		write_sj(b + '0');
		write_sj(0x2e); // 符号.
		write_sj(c + '0');
		write_sj(0xdf); // 符号°

		write_zl(0x80 + 0x0D); // 指针
		write_sj(HT1 + '0');
		write_sj(HT2 + '0');
		write_sj(0xdf); // 符号°

		write_zl(0x80 + 0x4D); // 指针
		write_sj(LT1 + '0');
		write_sj(LT2 + '0');
		write_sj(0xdf); // 符号

		//	write_zl(0x80+0x0e);//指针
		//	write_zl(0xf);

		/*如果低温高于高,则调换数值*/
		if (LT > HT)
		{
			A = LT;
			LT = HT;
			HT = A;
		}

		if (i / 10 > HT || i / 10 < LT)
		{
			ET0 = 1;
		}
		else
		{
			ET0 = 0;
			P1 = 0xff;
			cc = 1;
		}
		
		/*开关按下,数值为0*/
		if (key1 == 0)
		{
			Delay_Ms(20); //防抖动
			if (key1 == 0)
			{
				HT++;//hight temperature升高1度
				while (!key1)
					;
			}
		}
		if (key2 == 0)
		{
			Delay_Ms(20);//防抖动
			if (key2 == 0)
			{
				HT--;//hight temperature降低1度
				while (!key2)
					;
			}
		}
		if (key3 == 0)
		{
			Delay_Ms(20);//防抖动
			if (key3 == 0)
			{
				LT++;//low temperature降低1度
				while (!key3)
					;
			}
		}
		if (key4 == 0)
		{
			Delay_Ms(20);//防抖动
			if (key4 == 0)
			{
				LT--;//low temperature降低1度
				while (!key4)
					;
			}
		}
	}
}

void timer0() interrupt 1
{
	TH0 = 0x00;
	TL0 = 0x00; // 定时15ms
	P1 = ~P1;
	cc = ~cc;
}