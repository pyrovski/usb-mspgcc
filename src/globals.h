#ifndef GLOBALS_H
#define GLOBALS_H
#include <stdint.h>

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
    int ta1_ccr1:1;
    int ta1_if  :1;
    int p1_2    :1;
  } ISR_bits;
  volatile int ISR_int;
} ISR_u;

extern volatile ISR_u ISR_union;

extern volatile uint8_t PPD_count;
#define PPD_avg_period_p2 3
extern volatile uint32_t PPD_tot_ticks, PPD_tot_down_ticks;
extern volatile uint8_t PPD42_state;
extern volatile uint8_t PPD_ta0_overflow_counter;
extern volatile float PPD_last_10_duty;
extern volatile uint8_t PPD_new;

typedef struct {
  /*
    phase 0: idle (host pull-up)
    phase 1: host initiate (1-10ms)
    phase 2: wait for response (20-40us)
    phase 3: sensor low (80us)
    phase 4: sensor high (80us)
    phase 5: data (40 bits, 78-120us per bit: 50us low, 26-28/70us high)
    phase 6: sensor low (?us)
    phase 7: 2s wait
   */
  
  volatile uint8_t  phase;
  volatile uint8_t  bit;
  volatile uint64_t data;
  volatile uint8_t  level;
} am2302_state_s;
extern volatile am2302_state_s am2302_state;

#endif
