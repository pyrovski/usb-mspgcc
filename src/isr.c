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
    TA0R = 0;
    PPD42_state = !!(TA0CCTL1 & SCCI);
    
    PPD_tot_ticks += ((uint32_t)PPD_ta0_overflow_counter << 16) + TA0CCR1;
    PPD_ta0_overflow_counter = 0;

    if(!PPD42_state){
    } else { // high
      uint16_t up = TA0CCR1;
      
      PPD_tot_down_ticks += 
	((uint32_t)PPD_ta0_overflow_counter << 16) + up;

      if(PPD_count >= 10){
	PPD_last_10_duty = ((float) PPD_tot_down_ticks)/PPD_tot_ticks;
	PPD_tot_ticks = 0;
	PPD_tot_down_ticks = 0;
	PPD_count = 0;
	PPD_new = 1;
      } else {
	PPD_count++;
      }
    }

    P1OUT ^= BIT0;
    ISR_union.ISR_bits.ta0_ccr1 = 1;
  } else if(IV == TA0IV_TA0IFG){
    ISR_union.ISR_bits.ta0_if = 1;
    PPD_ta0_overflow_counter++;
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
  volatile uint16_t IV = TA1IV;
  uint16_t prevTA1R = TA1R;
  if(IV == TA1IV_TA1CCR1){
    TA1R = 0; // reset timer A1
    am2302_state.level = !!(TA1CCTL1 & SCCI);
    switch(am2302_state.phase){
    case 0: // idle
      //!@todo error
      break;
    case 1: // host init low done; pull up
      if(!am2302_state.level){
	am2302_state.phase = 2;
	am2302InPullup();
	//!@todo prevTA1R should be ~5000
      }
      break;
    case 2: // host wait high 20-40us done; pulled down
      if(!am2302_state.level){
	// device triggered low
	//!@todo prevTA1R should be 20-40+
	am2302_state.phase = 3;
      } //!@todo else error
      break;
    case 3: // sensor low 80us done
      if(am2302_state.level){
	// device triggered high
	//!@todo prevTA1R should be 80+
	am2302_state.phase = 4;
      } //!@todo else error
      break;
    case 4: // sensor high 80us done
      if(!am2302_state.level){
	// device triggered low
	//!@todo prevTA1R should be 80+
	am2302_state.phase = 5;
      } //!@todo else error
      break;
    case 5: // data
      if(!am2302_state.level){
	// device triggered low
	//!@todo prevTA1R should be 80+
	if(am2302_state.bit < 40){
	  am2302_state.phase = 5;
	} else {
	  am2302_state.phase = 6;
	}
      } else { // time for 1 (70us) or 0 (26-28us)
	//!@todo if < 20, error
	if(TA1CCR1 < 30){
	  // 0 bit
	} else if(TA1CCR1 > 60){
	  // 1 bit
	  am2302_state.data |= (1 << am2302_state.bit);
	} //!@todo if >= 80, error
	am2302_state.bit++;
      }
      break;
    case 6: // sensor low
      // bring high again; start 2s wait
      //!@todo do we need a wait phase here?
      am2302OutHigh();
      am2302_state.phase = 7;
      //!@todo set a TA0 CCR to compare TA0R-1; ~2s delay.
      break;
    case 7: // 2s wait; error
      break;
    default:
      // error
      break;
    }
    ISR_union.ISR_bits.ta1_ccr1 = 1;
  } else if(IV == TA1IV_TA1IFG){
    ISR_union.ISR_bits.ta1_if = 1;
    if(am2302_state.phase != 0 && am2302_state.phase != 7){
      //!@todo error
    }
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
