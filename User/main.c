#include "sys.h"
#include "delay.h"
#include "lcd1602.h"
#include "ds18b20.h"
#include "timer.h"
#include "adc.h"
#include "usart1.h"
#include "gpio.h"
#include "stdbool.h"
#include "stdio.h"
#include <stdlib.h>
#include "string.h"

#define STM32_RX1_BUF       Usart1RecBuf 
#define STM32_Rx1Counter    RxCounter
#define STM32_RX1BUFF_SIZE  USART1_RXBUFF_SIZE

#define FLASH_SAVE_ADDR  ((u32)0x0800F000)  				//设置FLASH 保存地址(必须为偶数)

bool Mode = 0;//0是自动模式，1是手动模式
u16 water =0;//水位
u16 water_low=200; //水位下限
u8 temp_hight = 35,temp_low = 10;//温度上限下限
u8 setn=0;  //设置计数
short temperature;//温度
bool Twinkle = 0;  //缺水闪烁
u16 openTime = 0;//出水时间
u16 volume = 200;  //单次出水
bool openFlag = 0;//出水标志
bool beepFlag = 0;//蜂鸣器标志
bool heatFlag =0;  //加热标志
bool shanshuo=0;  //设置闪烁
u16 miao=0;  //出水时间


void STM32_FlashWriteData(void)  //STM32 Flash写入数据
{
		u16 temp_buf[4];
	  
	  temp_buf[0] = temp_low;
	  temp_buf[1] = temp_hight;
	temp_buf[2] = volume;
    temp_buf[3] = water_low;
	  STMFLASH_Write(FLASH_SAVE_ADDR + 0x60,temp_buf,4); //存入数据
		delay_ms(100);
}

void STM32_FlashReadData(void)  //STM32 Flash读出数据
{
		u16 temp_buf[4];
	  
	  STM32F10x_Read(FLASH_SAVE_ADDR + 0x60,temp_buf,4); //读出数据
	
	  temp_low   = temp_buf[0];
	  temp_hight = temp_buf[1];
       volume = temp_buf[2];
    water_low = temp_buf[3];
		delay_ms(100);
}

void STM32_FlashCheck(void)  // 检查是否是新的单片机，是的话清空存储区，否则保留
{
	  u8 comper_str[6];
		
	  STM32F10x_Read(FLASH_SAVE_ADDR + 0x10,(u16*)comper_str,5);
	  comper_str[5] = '\0';
	  if(strstr((char *)comper_str,"FDYDZ") == NULL)  //新的单片机
		{
			 STMFLASH_Write(FLASH_SAVE_ADDR + 0x10,(u16*)"FDYDZ",5); //写入“FDYDZ”，方便下次校验
			 delay_ms(50);
			 STM32_FlashWriteData();
	  }
		STM32_FlashReadData();

		delay_ms(100);
}


void Display()//显示
{
    water = Get_Adc_Average(ADC_Channel_8,10);  //获取水位数据
    temperature=ReadTemperature();  //获取温度
    if(Mode==0)  //自动模式
    {
        LCD_Write_String(0,0,"Temperature: "); //显示温度
        LCD_Write_String(15,0,"C");

        LCD_Write_Char(13,0,temperature/10+48);
        LCD_Write_Char(14,0,temperature%10+48);
        
        if(water>=water_low&&openFlag==1)  //显示自动
        {
            LCD_Write_String(0,1,"   Open     ");
            LCD_Write_String(14,1,"ZD");
        }
        else if(water>=water_low&&openFlag==0)
        {
            //显示水量
            LCD_Write_Char(0,1,water/1000+48);
            LCD_Write_Char(1,1,water/100%10+48);
			LCD_Write_Char(2,1,water/10%10+48);
			LCD_Write_Char(3,1,water%10+48);
            LCD_Write_String(4,1,"ml        ");
            LCD_Write_String(14,1,"ZD");
        }
        else if(water<water_low)  //显示缺失
        {
            beepFlag=1;
            if(Twinkle == 0)
            {
                LCD_Write_String(0,1," Without Water! ");
              
            }
            else
            {
                LCD_Write_String(0,1,"                ");   
            }
        }
    }
    else if(Mode==1)
    {
        LCD_Write_String(0,0,"Temperature: ");     //显示温度
        LCD_Write_String(15,0,"C");
  

        LCD_Write_Char(13,0,temperature/10+48);
        LCD_Write_Char(14,0,temperature%10+48);
        
        if(water>=water_low&&relay2==1)
        {
             LCD_Write_String(0,1,"   Open     ");//显示手动
            LCD_Write_String(14,1,"SD");
        }
        else if(water>=water_low&&relay2==0)
        {
             //显示水量
            LCD_Write_Char(0,1,water/1000+48);
            LCD_Write_Char(1,1,water/100%10+48);
			LCD_Write_Char(2,1,water/10%10+48);
			LCD_Write_Char(3,1,water%10+48);
            LCD_Write_String(4,1,"ml        ");
            LCD_Write_String(14,1,"SD");
        }
        else if(water<water_low)  //显示缺失
        {
            beepFlag=1;
            if(Twinkle == 0)
            {
                LCD_Write_String(0,1," Without Water! ");
             
            }
            else
            {
                LCD_Write_String(0,1,"                ");
               
            }
        }
    }
    
		
}
void DisplaySet()//显示设置
{
    if(setn == 1)//显示设置页面
		{
            if(shanshuo==1)
            {
            LCD_Write_String(0,0," Set The Volume "); //显示设置单次出水
            LCD_Write_String(0,1,"     ");
            
            LCD_Write_Char(5,1,volume/100+48);  //显示百位
			LCD_Write_Char(6,1,volume/10%10+48);//显示十位
			LCD_Write_Char(7,1,volume%10+48);   //显示个位
            LCD_Write_String(8,1,"ml      ");
            }
            else
            {
			LCD_Write_String(7,1," ");
                
            }
            
				
            
		}
        
		
		if(setn == 2)//显示设置页面
		{
            if(shanshuo==1)
            {
            LCD_Write_String(0,0,"  Set The Temp  ");  //显示设置温度下限
            LCD_Write_String(0,1,"T-L:");
            LCD_Write_String(6,1,"C  T-H:");
            LCD_Write_String(15,1,"C");
            LCD_Write_Char(4,1,temp_low/10+48);//显示十位
            LCD_Write_Char(5,1,temp_low%10+48);//显示个位

            LCD_Write_Char(13,1,temp_hight/10+48);
            LCD_Write_Char(14,1,temp_hight%10+48);
            }
            else
            {
            LCD_Write_String(5,1," ");
            }
		}
		if(setn == 3)
		{
             if(shanshuo==1)
            {
            LCD_Write_String(0,0,"  Set The Temp  ");//显示设置温度上限
            LCD_Write_String(0,1,"T-L:");
            LCD_Write_String(6,1,"C  T-H:");
            LCD_Write_String(15,1,"C");
            LCD_Write_Char(4,1,temp_low/10+48);//显示十位
            LCD_Write_Char(5,1,temp_low%10+48);//显示个位

            LCD_Write_Char(13,1,temp_hight/10+48);
            LCD_Write_Char(14,1,temp_hight%10+48);
            }
            else
            {
            LCD_Write_String(14,1," ");
            }
		}
        if(setn == 4)//显示设置页面
		{
            if(shanshuo==1)
            {
            LCD_Write_String(0,0," Set Volume Min ");  //显示水位下限
            LCD_Write_String(0,1,"     ");
             LCD_Write_Char(5,1,water_low/1000+48);        //显示千位 
            LCD_Write_Char(6,1,water_low/100%10+48);    //显示百位
			LCD_Write_Char(7,1,water_low/10%10+48);//显示十位
			LCD_Write_Char(8,1,water_low%10+48);    //显示个位
            LCD_Write_String(9,1,"ml     ");
            }
            else
            {
                LCD_Write_String(8,1," ");
            }
				
            
		}
        
}


void keyscan()//按键扫描
{
    static uint16_t key1_long = 0;  // KEY1 长按计数
    static uint16_t key2_long = 0;  // KEY2 长按计数
    if(key4 == 0&&setn==0)//自动和手动切换
    {
        delay_ms(20);
        if(key4 == 0&&setn==0)
        {
             while(key4 == 0&&setn==0); 
             Mode = !Mode; //模式切换
            if(Mode==0)  //自动模式
            {
              
            }
            else //手动模式
            {
                   if((relay1==1||relay2==1)&&setn==0)
                {
                    beepFlag=1;
                    relay1=0;  //先关闭继电器
                    relay2=0;  
                    heatFlag=0;
                }
              
            }
        }
    }
    if(key3 == 0)//设置键按下
	{
        delay_ms(20);
        if(key3 == 0)
        {
            while(key3 == 0);//等待按键松开
            if(Mode==0)
            {
			 
                if((relay1==1||relay2==1)&&setn==0)
                {
                    beepFlag=1;
                    relay1=0;  //先关闭继电器
                    relay2=0;  
                    heatFlag=0;
                }
				setn ++; //设置计数加
                if(setn > 4)//退出设置
				{
					setn = 0;				
                    STM32_FlashWriteData();//存储 
				}
            } 
        }	
	}
 	if(key2 == 0)//加键按下
	{
	   delay_ms(20);
	   if(key2 == 0)
	   {
	   	  key2_long++;
			if(key2_long == 1)  // 第一次按下，单次减
            {
                if(setn == 0 && Mode==1&&temperature < temp_hight)//手动模式
			 {
					relay1=~relay1;  //继电器开关
                 if(relay1==1)
                    heatFlag=1;
                else
                    heatFlag=0;
           
                    beepFlag=1;//蜂鸣器报警提醒
			 }
			  if(setn == 1)//调出水量（在设置状态下）
				{
						if(volume < 999)volume++;   //单次出水加
				}
                
			  if(setn == 2)
				{
						if(temp_hight - temp_low > 1)//上限必须大于下限，下限才能加
						{
							 temp_low++;	
						}	
			  }
				if(setn == 3)
				{
						if(temp_hight < 99)  //温度上限加
						{
							 temp_hight++;	
						}	
						
			  }
                if(setn == 4)
				{
						if(water_low < 2000)  //水位下限加
						{
							 water_low++;	
						}	
						
			  }
          }
            else if(key2_long > 20) // 长按进入连加（20*20ms ≈ 400ms）
            {
                if(key2_long % 2 == 0) // 每 5*20ms = 100ms 连加一次
                {
            if(setn == 1)//调出水量（在设置状态下）
				{
						if(volume < 999)volume++;    //单次出水加
				}
                
			  if(setn == 2)
				{
						if(temp_hight - temp_low > 1)//上限必须大于下限，下限才能加
						{
							 temp_low++;	
						}	
			  }
				if(setn == 3)
				{
						if(temp_hight < 99) //温度上限加
						{
							 temp_hight++;	
						}	
						
			  }
                if(setn == 4)
				{
						if(water_low < 2000)//水位下限加
						{
							 water_low++;	
						}	
						
			  }
          }
      }
	   }	
	}
     else key2_long=0;  // 松开复位
	if(key1 == 0)//减键按下
	{
	   delay_ms(20);
	   if(key1 == 0)
	   {
	   	  key1_long++;
           
           if(setn == 0 &&water>=water_low && HW==0)//出水按键，必须在检测到有杯子的情况下才有效
            {
                    
                            relay2=~relay2;
                    
            }
            else 
            {
                    relay2=0;
            }
			if(key1_long == 1)  // 第一次按下，单次减
            {
           
			  if(setn == 1)//调出水量（在设置状态下）
				{
						if(volume > 0)volume--;
							
				}
				if(setn == 2)
				{
						if(temp_low > 0)//下限最小到0
						{
							 temp_low--;	
						}	
						
			  }
				if(setn == 3)
				{
						if(temp_hight - temp_low > 1)//上限必须大于下限,上限才能减
						{
							 temp_hight--;	
						}	
						
			  }
                if(setn == 4)
				{
						if(water_low> 0)//下限最小到0
						{
							 water_low--;	
						}	
						
			  }
                 }
            else if(key1_long > 20) // 长按进入连减
            {
                if(key1_long % 2 == 0)
                {
                    if(setn == 1)//调出水量（在设置状态下）
				{
						if(volume > 0)volume--;
							
				}
				if(setn == 2)
				{
						if(temp_low > 0)//下限最小到0
						{
							 temp_low--;	
						}	
						
			  }
				if(setn == 3)
				{
						if(temp_hight - temp_low > 1)//上限必须大于下限,上限才能减
						{
							 temp_hight--;	
						}	
						
			  }
                if(setn == 4)
				{
						if(water_low> 0)//下限最小到0
						{
							 water_low--;	
						}	
						
			  }
          }
      }
	   }	
	}
	else key1_long=0;  // 松开复位
	
}


int main(void)
{	
    delay_init();	    //延时函数初始化	  
    delay_ms(500);       //上电瞬间加入一定延时在初始化
    STM32_FlashCheck();
    DS18B20_GPIO_Init(); //ds18b20引脚初始化
    DS18B20_Init();			//初始化DS18B20
    ReadTemperature();
    Adc_Init();  //adc初始化
    uart1_Init(9600);
    KEY_AND_RELAY_GPIO_Init();  //GPIO初始化
    LCD_Init();         //屏幕初始化
    memset(STM32_RX1_BUF,0,STM32_RX1BUFF_SIZE);
    STM32_Rx1Counter = 0;
    delay_ms(500);
    TIM3_Init(499,7199);   //定时器初始化，定时50ms
		//Tout = ((arr+1)*(psc+1))/Tclk ; 
		//Tclk:定时器输入频率(单位MHZ)
		//Tout:定时器溢出时间(单位us)
		while(1)
		{  
			 keyscan();//按键扫描
			 if(setn ==0)
			 {
					Display();//显示参数
							if(Mode == 0)//在自动模式下
							{
                                
									if(key1 == 0 && HW==0&&water>=water_low)//出水按键，必须在检测到有杯子的情况下才有效
									{
											delay_ms(10);
											if(key1 == 0 && HW==0)
											{
													while(key1 == 0);//等待按键松开
                                                if(water>=water_low)
                                                {
												  openFlag=1;//显示Open
												  miao=volume/5;//时间赋值
                                                }
                                                
												  
											}
									}
                                    if(water>=water_low)//水位大于下限
                                    {
                                        if(temperature <= temp_low)//低于下限开启加热继电器
                                        {
                                        relay1=1;//开启加热继电器
                                            if(heatFlag==0)
                                            {
                                                beepFlag=1;  //开启蜂鸣器
                                            }
                                            heatFlag=1;
                                        }
                                        if(temperature >= temp_hight)//高于上限停止
                                        {
                                        relay1=0;//关闭加热继电器
                                            if(heatFlag==1)
                                            {
                                                beepFlag=1;//开启蜂鸣器
                                            }
                                            heatFlag=0;
                                        }
                                }
                                    else
                                    {
                                        
                                        relay1 = 0;//没有水关闭加热继电器
                                    }
							}
							else//在手动模式下
							{
                               if(temperature >= temp_hight)//高于上限停止
                                        {
                                        relay1=0;//关闭加热继电器
                                            if(heatFlag==1)
                                            {
                                                beepFlag=1;//开启蜂鸣器
                                            }
                                            heatFlag=0;
                                        }
									
									
							}
				  
					
			 }
             else//设置参数
                 DisplaySet();  //显示设置参数
			
			 delay_ms(10);
		}	
}

void TIM3_IRQHandler(void)//定时器3中断服务程序，用于记录时间
{ 
	  static u16 timeCount1=0;
	
	static u16 timeCount3=0;
		if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET) //检查指定的TIM中断发生与否:TIM 中断源 
		{ 
				TIM_ClearITPendingBit(TIM3, TIM_IT_Update); //清除中断标志位  

			  timeCount1++;
			  if(timeCount1 >= 10)//500ms
				{
						timeCount1 = 0;
						Twinkle = !Twinkle;
                    shanshuo=!shanshuo;
				}
				
				
                if(Mode==0)
                {
                if(openFlag == 1)//出水标志
                {
                    if(HW == 1){miao=0;openFlag = 0;}; 
                   
                }
                 if(miao > 0){
                        miao--;
                        relay2=1;
                    }
                    else {openFlag = 0;relay2=0;}
                }
                 if(beepFlag==1)  //蜂鸣器报警时常
            {
                timeCount3=10; //500ms
                beepFlag=0;
            }
            if(timeCount3>0)
            {
                timeCount3--;
                beep=1;  //蜂鸣器提醒
                
            }
            else
                beep=0;//关闭蜂鸣器
	  }
}

