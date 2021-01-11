/*********************************************************************************************
程序名：	NTC10K热敏电阻ADC读取程序
编写时间：	2020年11月17日
硬件支持：	STM32F103C8   外部晶振8MHz RCC函数设置主频72MHz　  

修改日志：　　							
说明：
 # 本模板加载了STM32F103内部的RCC时钟设置，并加入了利用滴答定时器的延时函数。
 # 可根据自己的需要增加或删减。

*********************************************************************************************/
#include "stm32f10x.h" //STM32头文件
#include "sys.h"
//#include "usart.h"	//与ADC冲突
#include "delay.h"
#include "touch_key.h"
#include "relay.h"
#include "oled0561.h"
#include "pwm.h" 
#include "adc.h"
#include "pid.h"

void temp_pid(int a);
void disp_temp(); //温度显示函数
float temp_adc(); // ADC采样函数
extern vu16 ADC_DMA_IN5;//声明外部变量
u16 ad_value;
float temp;  //查表所得温度数值
float real_temp; // 插值法得到实际温度值
//int pwm_h=600; //pwm1高电平时间



static u16 NTCTAB[241]=
{
0xf71, 0xf6a, 0xf63, 0xf5a, 0xf51, 0xf47, 0xf3d, 0xf31, 0xf25, 0xf19,
0xf0b, 0xefd, 0xeee, 0xede, 0xece, 0xebc, 0xeab, 0xe98, 0xe84, 0xe70,
0xe5c, 0xe47, 0xe31, 0xe1a, 0xe03, 0xdeb, 0xdd3, 0xdba, 0xda0, 0xd85,
0xd6a, 0xd4d, 0xd30, 0xd12, 0xcf4, 0xcd4, 0xcb4, 0xc93, 0xc71, 0xc4f,
0xc2b, 0xc06, 0xbe1, 0xbba, 0xb93, 0xb6c, 0xb43, 0xb1a, 0xaf1, 0xac7,
0xa9c, 0xa71, 0xa45, 0xa1a, 0x9ee, 0x9c1, 0x995, 0x968, 0x93b, 0x90e,
0x8e1, 0x8b3, 0x887, 0x859, 0x82d, 0x800, 0x7d4, 0x7a7, 0x77b, 0x750,
0x725, 0x6fa, 0x6cf, 0x6a6, 0x67c, 0x653, 0x62b, 0x603, 0x5dc, 0x5b6,
0x590, 0x56a, 0x546, 0x522, 0x4ff, 0x4dc, 0x4ba, 0x499, 0x479, 0x459,
0x43a, 0x41c, 0x3ff, 0x3e2, 0x3c6, 0x3aa, 0x390, 0x376, 0x35c, 0x344,
0x32c, 0x315, 0x2fe, 0x2e8, 0x2d2, 0x2bd, 0x2a9, 0x295, 0x282, 0x270,
0x25e, 0x24c, 0x23b, 0x22b, 0x21b, 0x20b, 0x1fc, 0x1ee, 0x1e0, 0x1d2,
0x1c5, 0x1b8, 0x1ab, 0x19f, 0x194, 0x188, 0x17d, 0x173, 0x168, 0x15f,
0x155, 0x14c, 0x143, 0x13a, 0x131, 0x129, 0x121, 0x119, 0x111, 0x10a,
0x103, 0xfc, 0xf6, 0xef, 0xe9, 0xe3, 0xdd, 0xd7, 0xd1, 0xcc,
0xc6, 0xc1, 0xbc, 0xb7, 0xb2, 0xad, 0xa8, 0xa4, 0xa0, 0x9b,
0x97, 0x93, 0x8f, 0x8c, 0x88, 0x84, 0x81, 0x7e, 0x7a, 0x77,
0x74, 0x72, 0x6e, 0x6c, 0x69, 0x67, 0x64, 0x62, 0x5f, 0x5d,
0x5b, 0x58, 0x56, 0x54, 0x52, 0x50, 0x4e, 0x4c, 0x4b, 0x49,
0x47, 0x46, 0x44, 0x42, 0x41, 0x40, 0x3e, 0x3d, 0x3b, 0x3a,
0x39, 0x38, 0x37, 0x35, 0x34, 0x33, 0x32, 0x31, 0x30, 0x2f,
0x2e, 0x2d, 0x2c, 0x2b, 0x2b, 0x2a, 0x29, 0x28, 0x27, 0x27,
0x26, 0x25, 0x24, 0x23, 0x23, 0x22, 0x21, 0x21, 0x20, 0x1f,
0x1f, 0x1e, 0x1e, 0x1d, 0x1c, 0x1c, 0x1b, 0x1b, 0x1a, 0x1a,
0x19
};//新的NTC值表

int main (void){//主程序


//
	delay_ms(500); //上电时等待其他器件就绪
	RCC_Configuration(); //系统时钟初始化 
	TOUCH_KEY_Init();//触摸按键初始化
	RELAY_Init();//继电器初始化

	ADC_Configuration(); //ADC初始化设置

	I2C_Configuration();//I2C初始化
	OLED0561_Init(); //OLED初始化
//	OLED_DISPLAY_16x16(0,2*16,0);//汉字显示	 厦门大学
//	OLED_DISPLAY_16x16(0,3*16,1);
//	OLED_DISPLAY_16x16(0,4*16,2);
//	OLED_DISPLAY_16x16(0,5*16,3);
	
	OLED_DISPLAY_8x16_BUFFER(0,"  TEMP CONTROL  ");
	//OLED_DISPLAY_8x16_BUFFER(2," PWM1:   PWM2: ");	//显示字符串

	OLED_DISPLAY_8x16_BUFFER(6," PV: "); //显示字符串
	TIM3_PWM_Init(2999,23); //设置频率为1kHz，公式为：溢出时间Tout（单位秒）=(arr+1)(psc+1)/Tclk	 1MS = (2999+1)*(23+1)/72000000
                          //Tclk为通用定时器的时钟，如果APB1没有分频，则就为系统时钟，72MHZ
                          //PWM时钟频率=72000000/(2999+1)*(23+1) = 1kHZ (1ms),设置自动装载值3000,预分频系数24


	while(1){
		//TIM_SetCompare3(TIM3,pwm_h);        //改变比较值TIM3->CCR2达到调节占空比的效果
		//TIM_SetCompare4(TIM3,pwm_h);        //改变比较值TIM3->CCR2达到调节占空比的效果
		
		//将热敏电阻NTC10K的温度数据显示在OLED上		

//		OLED_DISPLAY_8x16(2,6*8,(pwm_h*100)/3000/10+0x30);//显示pwm1占空比十位
//		OLED_DISPLAY_8x16(2,7*8,(pwm_h*100)/3000%10+0x30);//显示pwm1占空比个位
//		OLED_DISPLAY_8x16(2,14*8,(3000-pwm_h)*100/3000/10+0x30);//显示pwm2占空比十位
//		OLED_DISPLAY_8x16(2,15*8,(3000-pwm_h)*100/3000%10+0x30);
		
 //显示字符串
		OLED_DISPLAY_8x16_BUFFER(4," SV: 50");
		temp_pid(60); //50摄氏度温度pid控制，这边参数要比设定值+10
		OLED_DISPLAY_8x16_BUFFER(4," SV: 90");
		temp_pid(90); //90度温度pid控制

		//delay_ms(500); //延时
	}
}
void temp_pid(int a)
{	
	float c_temp;
	int i =120;
	PID  pid;
	pid.Set_temperature=a;    //用户设定值
	pid.proportion=30;
	pid.integral=3000;   //积分系数
	pid.differential=1200;   //微分系数
	pid.T=1000;    //temperature计算周期（采样周期)
  pid.error_current=0.0; //当前误差
	pid.error_last=0;  // 上一时刻误差
	pid.error_last2=0; // 上上时刻误差
	pid.pid_proportion_out=0;
	pid.pid_integral_out=0;
	pid.pid_differential_out=0;
	pid.pid_out=0;

	while(c_temp!=(a-10)){
			c_temp =temp_adc(); //ADC 采样
			pid_control(&pid,c_temp);//根据设定值进行温度pid控制
		  disp_temp();//oled温度显示
			}
	while(i-->0 )//恒温控制，时间可以设定
		{
			c_temp =temp_adc();
			pid_control(&pid,c_temp);
			disp_temp();
		}

}

void disp_temp(void)
{
		if(real_temp<0)
		{
			OLED_DISPLAY_8x16(6,5*8,'-'); //负温度
		}
		OLED_DISPLAY_8x16(6,6*8,(int)real_temp/10+0x30);//十位
		OLED_DISPLAY_8x16(6,7*8,(int)real_temp%10+0x30);//个位
		OLED_DISPLAY_8x16(6,8*8,'.');//
		OLED_DISPLAY_8x16(6,9*8,(int)(real_temp*10)%10+0x30);//十分位
		OLED_DISPLAY_8x16(6,10*8,(int)(real_temp*100)%10+0x30);//百分位
		OLED_DISPLAY_8x16(6,11*8,(int)(real_temp*1000)%10+0x30);//千分位
		OLED_DISPLAY_16x16(6,6*16,4);
		delay_ms(500); //延时//
}
float temp_adc(void)
{
		if( (ADC_DMA_IN5<=NTCTAB[0]) && (ADC_DMA_IN5>=NTCTAB[240]) )
    {
        ad_value = look_up_table(NTCTAB, 241, ADC_DMA_IN5);//根据数值查表得到序号
        temp = num_to_temperature(ad_value);//序号对应温度值
        real_temp =(float)(ADC_DMA_IN5-NTCTAB[ad_value])
			/(NTCTAB[ad_value+1]-NTCTAB[ad_value])+temp;//线性插值
        //printf("temp_ntc:%4.2f\r\n",tx);    
    }
		return temp;
}
/*********************************************************************************************
*********************************************************************************************/
/*

【变量定义】
u32     a; //定义32位无符号变量a
u16     a; //定义16位无符号变量a
u8     a; //定义8位无符号变量a
vu32     a; //定义易变的32位无符号变量a
vu16     a; //定义易变的 16位无符号变量a
vu8     a; //定义易变的 8位无符号变量a
uc32     a; //定义只读的32位无符号变量a
uc16     a; //定义只读 的16位无符号变量a
uc8     a; //定义只读 的8位无符号变量a

#define ONE  1   //宏定义

delay_us(1); //延时1微秒
delay_ms(1); //延时1毫秒
delay_s(1); //延时1秒

GPIO_WriteBit(LEDPORT,LED1,(BitAction)(1)); //LED控制

*/



