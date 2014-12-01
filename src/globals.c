#include <stdint.h>
#include "globals.h"

//Indicates data has been received without an open rcv operation
volatile uint8_t bCDCDataReceived_event = 0;
volatile uint16_t last_conv = 0;
volatile uint8_t new_adc = 0;
volatile uint16_t last_adc_iv = 0;

ISR_u ISR_union;

volatile int p1_2_count = 0;
volatile uint16_t last_ta0_ccr1 = 0;
volatile uint8_t PPD42_state = 1;
volatile uint8_t ta0_overflow_counter = 0;
