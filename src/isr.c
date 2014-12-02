#include <stdint.h>
#include <msp430.h>
#include "globals.h"

#pragma vector=ADC12_VECTOR
__interrupt void ADC_ISR(void){
  uint16_t iv = ADC12IV;
  last_adc_iv = iv;
  if(iv & ADC12IV_ADC12IFG0){
    // ADC12MEM0 result is ready
    last_conv = ADC12MEM0;
    new_adc = 1;
  }
  ISR_union.ISR_bits.adc = 1;
}

// 0xFFEA, TA0CCR0 CCIFG0
#pragma vector=TIMER0_A0_VECTOR
__interrupt void TA0_ISR_0(void){

  // we don't have to reset any interrupt flags here
  ISR_union.ISR_bits.ta0_0 = 1;
}

#pragma vector=TIMER0_A1_VECTOR
// 0xFFEA, TA0CCR1 CCIFG1 to TA0CCR4 CCIFG4, TA0IFG (TA0IV)
__interrupt void TA0_ISR_1(void){
  volatile uint16_t IV = TA0IV;
  if(IV == TA0IV_TA0CCR1){
    PPD42_state = !!(TA0CCTL1 & CCI);

    if(!PPD42_state){
      ta0_PPD_down = TA0CCR1;
    } else {
      uint16_t up = TA0CCR1;
      int32_t PPD_down_ticks = ((int32_t)(up - ta0_PPD_down) + 
				((int32_t)ta0_overflow_counter << (int32_t)16));
      avg_PPD_down_ticks = ((avg_PPD_down_ticks << PPD_avg_period_p2) - 
			    avg_PPD_down_ticks + PPD_down_ticks) >> PPD_avg_period_p2;
      int32_t PPD_period_ticks = ((int32_t)(up - ta0_PPD_up) + 
			      ((int32_t)ta0_overflow_counter << (int32_t)16)) ;
      avg_PPD_period_ticks = ((avg_PPD_period_ticks << PPD_avg_period_p2) - 
			      avg_PPD_period_ticks + PPD_period_ticks) >> PPD_avg_period_p2;
      ta0_PPD_up = up;
    }

    ta0_overflow_counter = 0;
    p1_2_count++;
    P1OUT ^= BIT0;
    ISR_union.ISR_bits.ta0_ccr1 = 1;
  } else if(IV == TA0IV_TA0IFG){
    ISR_union.ISR_bits.ta0_if = 1;
    ta0_overflow_counter++;
  }
}

#pragma vector=PORT1_VECTOR
__interrupt void PORT1_ISR(void){
  volatile int IV = P1IV;
  ISR_union.ISR_bits.p1_2 = 1;
  //P1IES ^= BIT2; // change edge interrupt direction
  p1_2_count++;
  //DEBUG("P1.2 event!\r\n");
  P1OUT ^= BIT0;
  
}
