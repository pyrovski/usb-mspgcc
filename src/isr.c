#include <stdint.h>
#include <msp430.h>
#include "am2302.h"
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
  //  ISR_union.ISR_bits.adc = 1;
}

// 0xFFEA, TA0CCR0 CCIFG0
#pragma vector=TIMER0_A0_VECTOR
__interrupt void TA0_ISR_0(void){

  // we don't have to reset any interrupt flags here
  //  ISR_union.ISR_bits.ta0_0 = 1;
}

#pragma vector=TIMER0_A1_VECTOR
// 0xFFEA, TA0CCR1 CCIFG1 to TA0CCR4 CCIFG4, TA0IFG (TA0IV)
__interrupt void TA0_ISR_1(void){
  uint16_t IV = TA0IV;
  if(IV == TA0IV_TA0CCR1){
    uint16_t cap = TA0CCR1;
    PPD42_state = (TA0CCTL1 & SCCI) >> 10;
    TA0R = 0;
    if(PPD42_state){
      PPD_new_up = 1;
      PPD_last_up = cap;
    } else {
      PPD_new_dn = 1;
      PPD_last_dn = cap;
    }
    
    P1OUT ^= BIT0;
  } else if(IV == TA0IV_TA0IFG){
    if(PPD42_state)
      PPD_ta0_overflow_up++;
    else
      PPD_ta0_overflow_dn++;
  }
}

/* // 0xFFE2 Timer1_A3 CC0 */
/* #pragma vector=TIMER1_A0_VECTOR */
/* __interrupt void TA1_ISR_0(void){ */

/*   // we don't have to reset any interrupt flags here */
/*   ISR_union.ISR_bits.ta1_0 = 1; */
/* } */

#pragma vector=TIMER1_A1_VECTOR
// 0xFFE0 Timer1_A3 CC1-2, TA1
__interrupt void TA1_ISR_1(void){
  uint16_t IV = TA1IV;
  if(IV == TA1IV_TA1CCR1){
    uint16_t ta1_capture = TA1CCR1;
    am2302_event(ta1_capture);
    //ISR_union.ISR_bits.ta1_ccr1 = 1;
  } else if(IV == TA1IV_TA1IFG){
    //ISR_union.ISR_bits.ta1_if = 1;
    am2302_overflowEvent();
  }
}

/* #pragma vector=PORT1_VECTOR */
/* __interrupt void PORT1_ISR(void){ */
/*   volatile int IV = P1IV; */
/*   ISR_union.ISR_bits.p1_2 = 1; */
/*   //P1IES ^= BIT2; // change edge interrupt direction */
/*   p1_2_count++; */
/*   //DEBUG("P1.2 event!\r\n"); */
/*   P1OUT ^= BIT0; */
/* } */
