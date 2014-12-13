#include "usb_printf.h"
#include "globals.h"

void PPD_print_last(){
  float dutyF = 100.0f * PPD_last_10_duty;
  int dutyI = (int)dutyF;
  
  DEBUG("avg duty %%:%d.%d\r\n", dutyI, 
	(int)(10.0f*(dutyF - dutyI)));
}
