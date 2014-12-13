#ifndef GLOBALS_H
#define GLOBALS_H
#include <stdint.h>
#include "am2302.h"

extern volatile uint8_t bCDCDataReceived_event;
extern volatile uint16_t last_conv;
extern volatile uint8_t new_adc;
extern volatile uint16_t last_adc_iv;

/*
typedef union {
  volatile struct {
    int ta0_0   :1;
    int adc     :1;
    int ta0_ccr1:1;
    int ta0_if  :1;
    int ta1_ccr1:1;
    int ta1_if  :1;
    int p1_2    :1;
  } ISR_bits;
  volatile int ISR_int;
} ISR_u;
*/

//extern volatile ISR_u ISR_union;

extern volatile uint8_t  PPD42_state;
extern volatile uint8_t  PPD_ta0_overflow_dn;
extern volatile uint8_t  PPD_ta0_overflow_up;
extern volatile uint8_t  PPD_new_up;
extern volatile uint8_t  PPD_new_dn;
extern volatile uint16_t PPD_last_up;
extern volatile uint16_t PPD_last_dn;

uint8_t PPD_count;
uint32_t PPD_tot_ticks, PPD_tot_down_ticks;
float PPD_last_10_duty;

extern volatile am2302_state_s am2302_state;

#endif
