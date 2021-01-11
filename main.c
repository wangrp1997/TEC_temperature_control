/*********************************************************************************************
��������	NTC10K��������ADC��ȡ����
��дʱ�䣺	2020��11��17��
Ӳ��֧�֣�	STM32F103C8   �ⲿ����8MHz RCC����������Ƶ72MHz��  

�޸���־������							
˵����
 # ��ģ�������STM32F103�ڲ���RCCʱ�����ã������������õδ�ʱ������ʱ������
 # �ɸ����Լ�����Ҫ���ӻ�ɾ����

*********************************************************************************************/
#include "stm32f10x.h" //STM32ͷ�ļ�
#include "sys.h"
//#include "usart.h"	//��ADC��ͻ
#include "delay.h"
#include "touch_key.h"
#include "relay.h"
#include "oled0561.h"
#include "pwm.h" 
#include "adc.h"
#include "pid.h"

void temp_pid(int a);
void disp_temp(); //�¶���ʾ����
float temp_adc(); // ADC��������
extern vu16 ADC_DMA_IN5;//�����ⲿ����
u16 ad_value;
float temp;  //��������¶���ֵ
float real_temp; // ��ֵ���õ�ʵ���¶�ֵ
//int pwm_h=600; //pwm1�ߵ�ƽʱ��



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
};//�µ�NTCֵ��

int main (void){//������


//
	delay_ms(500); //�ϵ�ʱ�ȴ�������������
	RCC_Configuration(); //ϵͳʱ�ӳ�ʼ�� 
	TOUCH_KEY_Init();//����������ʼ��
	RELAY_Init();//�̵�����ʼ��

	ADC_Configuration(); //ADC��ʼ������

	I2C_Configuration();//I2C��ʼ��
	OLED0561_Init(); //OLED��ʼ��
//	OLED_DISPLAY_16x16(0,2*16,0);//������ʾ	 ���Ŵ�ѧ
//	OLED_DISPLAY_16x16(0,3*16,1);
//	OLED_DISPLAY_16x16(0,4*16,2);
//	OLED_DISPLAY_16x16(0,5*16,3);
	
	OLED_DISPLAY_8x16_BUFFER(0,"  TEMP CONTROL  ");
	//OLED_DISPLAY_8x16_BUFFER(2," PWM1:   PWM2: ");	//��ʾ�ַ���

	OLED_DISPLAY_8x16_BUFFER(6," PV: "); //��ʾ�ַ���
	TIM3_PWM_Init(2999,23); //����Ƶ��Ϊ1kHz����ʽΪ�����ʱ��Tout����λ�룩=(arr+1)(psc+1)/Tclk	 1MS = (2999+1)*(23+1)/72000000
                          //TclkΪͨ�ö�ʱ����ʱ�ӣ����APB1û�з�Ƶ�����Ϊϵͳʱ�ӣ�72MHZ
                          //PWMʱ��Ƶ��=72000000/(2999+1)*(23+1) = 1kHZ (1ms),�����Զ�װ��ֵ3000,Ԥ��Ƶϵ��24


	while(1){
		//TIM_SetCompare3(TIM3,pwm_h);        //�ı�Ƚ�ֵTIM3->CCR2�ﵽ����ռ�ձȵ�Ч��
		//TIM_SetCompare4(TIM3,pwm_h);        //�ı�Ƚ�ֵTIM3->CCR2�ﵽ����ռ�ձȵ�Ч��
		
		//����������NTC10K���¶�������ʾ��OLED��		

//		OLED_DISPLAY_8x16(2,6*8,(pwm_h*100)/3000/10+0x30);//��ʾpwm1ռ�ձ�ʮλ
//		OLED_DISPLAY_8x16(2,7*8,(pwm_h*100)/3000%10+0x30);//��ʾpwm1ռ�ձȸ�λ
//		OLED_DISPLAY_8x16(2,14*8,(3000-pwm_h)*100/3000/10+0x30);//��ʾpwm2ռ�ձ�ʮλ
//		OLED_DISPLAY_8x16(2,15*8,(3000-pwm_h)*100/3000%10+0x30);
		
 //��ʾ�ַ���
		OLED_DISPLAY_8x16_BUFFER(4," SV: 50");
		temp_pid(60); //50���϶��¶�pid���ƣ���߲���Ҫ���趨ֵ+10
		OLED_DISPLAY_8x16_BUFFER(4," SV: 90");
		temp_pid(90); //90���¶�pid����

		//delay_ms(500); //��ʱ
	}
}
void temp_pid(int a)
{	
	float c_temp;
	int i =120;
	PID  pid;
	pid.Set_temperature=a;    //�û��趨ֵ
	pid.proportion=30;
	pid.integral=3000;   //����ϵ��
	pid.differential=1200;   //΢��ϵ��
	pid.T=1000;    //temperature�������ڣ���������)
  pid.error_current=0.0; //��ǰ���
	pid.error_last=0;  // ��һʱ�����
	pid.error_last2=0; // ����ʱ�����
	pid.pid_proportion_out=0;
	pid.pid_integral_out=0;
	pid.pid_differential_out=0;
	pid.pid_out=0;

	while(c_temp!=(a-10)){
			c_temp =temp_adc(); //ADC ����
			pid_control(&pid,c_temp);//�����趨ֵ�����¶�pid����
		  disp_temp();//oled�¶���ʾ
			}
	while(i-->0 )//���¿��ƣ�ʱ������趨
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
			OLED_DISPLAY_8x16(6,5*8,'-'); //���¶�
		}
		OLED_DISPLAY_8x16(6,6*8,(int)real_temp/10+0x30);//ʮλ
		OLED_DISPLAY_8x16(6,7*8,(int)real_temp%10+0x30);//��λ
		OLED_DISPLAY_8x16(6,8*8,'.');//
		OLED_DISPLAY_8x16(6,9*8,(int)(real_temp*10)%10+0x30);//ʮ��λ
		OLED_DISPLAY_8x16(6,10*8,(int)(real_temp*100)%10+0x30);//�ٷ�λ
		OLED_DISPLAY_8x16(6,11*8,(int)(real_temp*1000)%10+0x30);//ǧ��λ
		OLED_DISPLAY_16x16(6,6*16,4);
		delay_ms(500); //��ʱ//
}
float temp_adc(void)
{
		if( (ADC_DMA_IN5<=NTCTAB[0]) && (ADC_DMA_IN5>=NTCTAB[240]) )
    {
        ad_value = look_up_table(NTCTAB, 241, ADC_DMA_IN5);//������ֵ���õ����
        temp = num_to_temperature(ad_value);//��Ŷ�Ӧ�¶�ֵ
        real_temp =(float)(ADC_DMA_IN5-NTCTAB[ad_value])
			/(NTCTAB[ad_value+1]-NTCTAB[ad_value])+temp;//���Բ�ֵ
        //printf("temp_ntc:%4.2f\r\n",tx);    
    }
		return temp;
}
/*********************************************************************************************
*********************************************************************************************/
/*

���������塿
u32     a; //����32λ�޷��ű���a
u16     a; //����16λ�޷��ű���a
u8     a; //����8λ�޷��ű���a
vu32     a; //�����ױ��32λ�޷��ű���a
vu16     a; //�����ױ�� 16λ�޷��ű���a
vu8     a; //�����ױ�� 8λ�޷��ű���a
uc32     a; //����ֻ����32λ�޷��ű���a
uc16     a; //����ֻ�� ��16λ�޷��ű���a
uc8     a; //����ֻ�� ��8λ�޷��ű���a

#define ONE  1   //�궨��

delay_us(1); //��ʱ1΢��
delay_ms(1); //��ʱ1����
delay_s(1); //��ʱ1��

GPIO_WriteBit(LEDPORT,LED1,(BitAction)(1)); //LED����

*/



