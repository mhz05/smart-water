#ifndef __GPIO_H
#define __GPIO_H	 
#include "sys.h"
//////////////////////////////////////////////////////////////////////////////////	 

#define LED1 PAout(11)
#define LED2 PAout(12)

#define relay1 PBout(7)
#define relay2 PBout(8)
#define beep   PBout(9)



#define key1 PBin(12)
#define key2 PBin(13)
#define key3 PBin(14)
#define key4 PBin(15)
#define key5 PAin(8)

#define HW PAin(15)

void KEY_AND_RELAY_GPIO_Init(void);//≥ı ºªØ
	 				    
#endif

