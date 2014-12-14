#include "usb_printf.h"
#include "globals.h"

void PPD_print_last(){
  float dutyF = 100.0f * PPD_last_10_duty;
  
  DEBUG("avg duty %%:");
  usb_printf_float(dutyF);
  DEBUG("\r\n");
}
