#include <msp430.h>
#include "am2302.h"
#include "globals.h"
#include "usb_printf.h"

void am2302_dump(){
  DEBUG("am2302: ");
  if(!am2302_state.newData)
    DEBUG("(old): ");
  DEBUG("0x%lx\r\n", am2302_state.data);
  DEBUG("temp: %d\r\n", am2302_state.temperature);
  DEBUG("rh: %u\r\n", am2302_state.relativeHumidity);
  DEBUG("phase:%d errPhase:%d err:%d code:%d\r\n", 
	am2302_state.phase, am2302_state.errorPhase, am2302_state.error, 
	am2302_state.errorCode);
  DEBUG("TA1CTL: 0x%x\r\n", TA1CTL);
  DEBUG("TA1CCTL1: 0x%x\r\n", TA1CCTL1);
  DEBUG("P2REN: 0x%x\r\n", P2REN);
  DEBUG("P2SEL: 0x%x\r\n", P2SEL);
  DEBUG("P2DIR: 0x%x\r\n", P2DIR);

  int i;
  DEBUG("am2302 timing: ");
  for(i = 0; i < sizeof(am2302_state.timing)/sizeof(am2302_state.timing[0]); i++)
    DEBUG("%d:%u\t", i, am2302_state.timing[i]);
  DEBUG("\r\n");
}

void am2302Low(){
  P2OUT &= ~BIT0; // P2.0 = low
  P2DIR |=  BIT0; // output
  P2SEL &= ~BIT0; // IO function
}

void am2302InPullup(){
  //TA1CCTL1 &= ~CCIE;

  // dummy read capture
  //uint16_t ta1_capture = TA1CCR1;

  P2REN |=  BIT0;  // enable pullup
  P2DIR &= ~BIT0;  // input
  P2OUT |=  BIT0;  // P2.0 = high
  P2SEL |=  BIT0;  // peripheral function

  TA1CCTL1 |= CAP; // enable capture
  TA1CTL |=  TACLR;
  am2302_state.last_ta1 = 0;

  //TA1CCTL1 |= CCIE;
}

void am2302OutHigh(){
  //TA1CCTL1 &= ~CCIE;
  P2OUT |=  BIT0; // high
  P2DIR |=  BIT0; // output
  P2SEL &= ~BIT0; // IO function
  // no re-enable of CCIE
}

void am2302Start(){
  TA1CTL   &= ~TAIE;
  // compare mode
  // clear capture overflow
  TA1CCTL1 &= ~(CCIE | CAP | COV);

  am2302_state.phase     = 1;
  am2302_state.bit       = 0;
  am2302_state.data      = 0;
  am2302_state.error     = 0;
  am2302_state.newData   = 0;
  am2302_state.reset     = 0;
  am2302_state.last_ta1  = 0;
  am2302_state.errorCode = 0;

  int i;
  for(i = 0; i < sizeof(am2302_state.timing)/sizeof(am2302_state.timing[0]); i++)
    am2302_state.timing[i] = 0;

  am2302_state.ta1_overflows = 0;
  am2302_state.errorPhase = 0;

  // dummy read capture
  uint16_t ta1_capture = TA1CCR1;

  am2302_dump();

  am2302Low();
  
  // Reset timer before changing compare value

  TA1CTL &= ~(MC0 | MC1);
  TA1CCR1              = 20000; // 20ms
  TA1CTL &= ~(TAIFG);
  TA1CTL |=  TACLR;
  TA1CTL |=  MC__CONTINUOUS;
  
  //TA1CCTL1 |= CCIE;
  //TA1CTL   |= TAIE;
}

void am2302_clearFault(){
  //TA1CTL   &= ~(TAIE | TAIFG);
  //TA1CCTL1 &= ~(CCIE | CCIFG | COV);
  TA1CCTL1 |= CAP; // enable capture
  TA1CTL |=  TACLR;

  TA1CTL   &= ~(TAIFG);
  TA1CCTL1 &= ~(CCIFG | COV);
  am2302_state.phase = -1;
  am2302OutHigh();
  
  am2302_state.ta1_overflows = 0;
  am2302_state.error = 0;
  TA1CTL |= TAIE;
}

void am2302_event(const uint16_t ta1_capture){
  am2302_state.level = !!(TA1CCTL1 & SCCI);
  if(TA1CCTL1 & COV){ // missed a capture
    TA1CCTL1 &= ~COV;
    am2302_error(am2302_cov);
  }

  am2302_state.timing[am2302_state.phase] =
    ta1_capture - am2302_state.last_ta1;
    
  if(am2302_state.error){
    am2302_state.phase = -1;
  }

  switch(am2302_state.phase){
  case -1: // 2s delay
    break;
  case 0: // idle; should not have input transitions here
    am2302_error(am2302_unexpInput);
    break;
  case 1: // host init low done; pull up
    if(!am2302_state.level){
      am2302_state.phase = 2;
      am2302InPullup();
      //!@todo prevTA1R should be ~5000
    } else { // high
      am2302_error(am2302_expLow);
    }
    break;
  case 85:
    am2302_state.phase = -1;
    am2302OutHigh();
    break;
  default:
    am2302_state.phase++;
    break;
  }
  if(am2302_state.phase >= 99)
    am2302_error(am2302_noError);
    
  am2302_state.last_ta1 = ta1_capture;
  am2302_state.ta1_overflows = 0;
}

void am2302_overflowEvent(){
  am2302_state.ta1_overflows++;
  if(am2302_state.phase > 0 && am2302_state.ta1_overflows >= 3){
    am2302_error(am2302_unexpOverflow);
  } else if(am2302_state.phase == -1 && am2302_state.ta1_overflows >= 31){
    am2302_state.phase = 0;
    am2302_state.reset = 1;
  }
}

void am2302_error(uint8_t code){
  am2302_state.error = 1;
  am2302_state.errorCode = code;
  am2302_state.errorPhase = am2302_state.phase;
  am2302_state.ta1_overflows = 0;
  am2302OutHigh();
}

void am2302Finish(){
  uint16_t ta1_capture = 0;
  while(am2302_state.phase > 0){
    uint8_t newPhase = am2302_state.phase;
    switch(am2302_state.phase){
    case -1: // wait
      break;
    case 0:
      return;
    case 1:
      // wait for compare
      while(!(TA1CCTL1 & CCIFG) && !(TA1CTL & TAIFG));
      if(TA1CCTL1 & CCIFG){
	ta1_capture = TA1CCR1;

	TA1CTL   &= ~(MC0 | MC1);
	TA1CCTL1 |=  CAP; // enable capture
	TA1CTL   |=  TACLR;

	P2REN    |=  BIT0;  // enable pullup
	P2DIR    &= ~BIT0;  // input
	P2OUT    |=  BIT0;  // P2.0 = high
	P2SEL    |=  BIT0; // Peripheral function
	      
	am2302_state.ta1_overflows = 0;
	__disable_interrupt();
	TA1CTL   |=  MC__CONTINUOUS;
	TA1CCTL1 &= ~CCIFG;
	uint16_t dummy_capture = TA1CCR1;
	am2302_state.last_ta1 = 0;
	newPhase++;
      } else if(TA1CTL & TAIFG){ // this should never happen
	TA1CTL &= ~TAIFG;
	newPhase = -1;
	am2302_error(am2302_unexpOverflow);
      }
      break;
    /* case 5: */
    /*   break; */
    default:
      while(!(TA1CCTL1 & CCIFG) && !(TA1CTL & TAIFG));
      if(TA1CCTL1 & CCIFG){
	ta1_capture = TA1CCR1;
	TA1CCTL1 &= ~CCIFG;
	if(!ta1_capture) continue;
	newPhase++;
      } else if(TA1CTL & TAIFG){ // this should never happen
	TA1CTL &= ~TAIFG;
	newPhase = -1;
	am2302_error(am2302_unexpOverflow);
      }
      break;
    }
    am2302_state.timing[am2302_state.phase] = 
      ta1_capture - am2302_state.last_ta1;
    am2302_state.last_ta1 = ta1_capture;
    if(newPhase >= 86)
      newPhase = -1;
    am2302_state.phase = newPhase;
  }
  TA1CTL |= TAIE;
  __enable_interrupt();
  if(am2302_state.error){
    am2302_clearFault();
    while(!am2302_state.reset); // wait for am2302_overflowEvent to set reset
    am2302_state.reset = 0;
    DEBUG("am2302 error\r\n");
  }
  am2302_dump(&am2302_state);
  if(!am2302_state.error && am2302_state.phase == -1){
    // normal read
    am2302_state.ta1_overflows = 0;
    while(!am2302_state.reset); // wait for am2302_overflowEvent to set reset
    am2302_state.reset = 0;
    DEBUG("am2302 ready\r\n");
  } else {
    DEBUG("am2302 err:%d phase:%d\r\n", 
	  am2302_state.error, am2302_state.phase);
  }

}
