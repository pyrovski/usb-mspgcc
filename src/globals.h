#ifndef GLOBALS_H
#define GLOBALS_H

extern volatile uint8_t bCDCDataReceived_event;
extern volatile uint16_t last_conv;
extern volatile uint8_t new_adc;
extern volatile uint16_t last_adc_iv;

typedef union {
  volatile struct {
    int ta0_0   :1;
    int adc     :1;
    int ta0_ccr1:1;
    int ta0_if  :1;
    int p1_2    :1;
  } ISR_bits;
  int ISR_int;
} ISR_u;

extern ISR_u ISR_union;

extern volatile int p1_2_count;
extern volatile uint16_t last_ta0_ccr1;
extern volatile uint8_t PPD42_state;
extern volatile uint8_t ta0_overflow_counter;
#endif
