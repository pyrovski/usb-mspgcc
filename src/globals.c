#include <stdint.h>
#include "globals.h"

//Indicates data has been received without an open rcv operation
volatile uint8_t bCDCDataReceived_event = 0;
volatile uint16_t last_conv = 0;
volatile uint8_t new_adc = 0;
volatile uint16_t last_adc_iv = 0;

volatile ISR_u ISR_union;

volatile int p1_2_count = 0;
volatile uint16_t ta0_PPD_down = 0, ta0_PPD_up = 0;
volatile uint8_t PPD42_state = 1;
volatile uint8_t ta0_overflow_counter = 0;
volatile uint32_t avg_PPD_period_ticks = 0, avg_PPD_down_ticks = 0;

volatile am2302_state_s am2302_state;
