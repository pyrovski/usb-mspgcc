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
  int i;
  DEBUG("am2302 timing: ");
  for(i = 0; i < sizeof(am2302_state.timing)/sizeof(am2302_state.timing[0]); i++)
    DEBUG("%d:%u\t", i, am2302_state.timing[i]);
  DEBUG("\r\n");
}

void am2302Low(){
  P2OUT &= ~BIT0; // P2.0 = low
  P2DIR |=  BIT0; // output
  P2SEL ^= ~BIT0; // IO function
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
  //TA1CCTL1 |= CCIE;
}

void am2302OutHigh(){
  TA1CCTL1 &= ~CCIE;
  P2OUT |=  BIT0; // high
  P2DIR |=  BIT0; // output
  P2SEL ^= ~BIT0; // IO function
  // no re-enable of CCIE
}

void am2302Start(){
  TA1CTL   &= ~TAIE;
  TA1CCTL1 &= ~CCIE;
  TA1CCTL1 &= ~CAP; // compare mode
  TA1CCTL1 &= ~COV; // clear capture overflow

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

  //TA1CTL |=  TACLR;
  TA1CTL &= ~(MC0 | MC1);
  TA1R                 = 0;    // reset timer A1
  TA1CCR1              = 10000; // 10ms
  TA1CTL |= MC__CONTINUOUS;
  //TA1CTL &= ~TACLR;
  
  TA1CCTL1 |= CCIE;
  TA1CTL   |= TAIE;
}

void am2302_clearFault(){
  TA1CTL   &= ~(TAIE | TAIFG);
  TA1CCTL1 &= ~(CCIE | CCIFG | COV);
  am2302_state.phase = -1;
  am2302OutHigh();
  
  am2302_state.ta1_overflows = 0;
  am2302_state.error = 0;
  TA1CTL   |= TAIE;
}
