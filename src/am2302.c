#include <msp430.h>
#include "am2302.h"
#include "globals.h"
#include "usb_printf.h"

void am2302_dump(am2302_state_s *st){
  DEBUG("am2302: ");
  if(!st->newData)
    DEBUG("(old): ");
  DEBUG("0x%lx\r\n", st->data);
  DEBUG("temp: %d\r\n", st->temperature);
  DEBUG("rh: %u\r\n", st->relativeHumidity);
  DEBUG("phase:%d errPhase:%d err:%d code:%d\r\n", 
	st->phase, st->errorPhase, st->error, st->errorCode);
  int i;
  DEBUG("am2302 timing: ");
  for(i = 0; i < 86; i++)
    DEBUG("%d:%u\t", i, st->timing[i]);
  DEBUG("\r\n");
}

void am2302Low(){
  P2SEL ^= ~BIT0; // IO function
  P2OUT &= ~BIT0; // P2.0 = low
  P2DIR |=  BIT0; // output
}

void am2302InPullup(){
  uint16_t oldCCIE = TA1CCTL1 & CCIE;
  TA1CCTL1 &= ~CCIE;
  P2OUT |=  BIT0;  // P2.0 = high
  P2REN |=  BIT0;  // enable pullup
  P2DIR &= ~BIT0;  // input
  P2SEL |=  BIT0;  // peripheral function
  TA1CCTL1 |= CAP; // enable capture
  TA1CCTL1 |= oldCCIE;
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

  am2302_state.phase   = 1;
  am2302_state.bit     = 0;
  am2302_state.data    = 0;
  am2302_state.error   = 0;
  am2302_state.newData = 0;

  int i;
  for(i = 0; i < 8; i++)
    am2302_state.timing[i] = 0;

  am2302_state.ta1_overflows = 0;
  am2302_state.errorPhase = 0;

  am2302Low();

  // Reset timer before changing compare value
  TA1R                 = 0;    // reset timer A1
  TA1CCR1              = 20000; // 20ms
  
  TA1CCTL1 |= CCIE;
  TA1CTL   |= TAIE;
}

void am2302_clearFault(){
  TA1CTL   &= ~(TAIE | TAIFG);
  TA1CCTL1 &= ~(CCIE | CCIFG | COV);
  am2302_state.phase = 7;
  am2302OutHigh();
  
  am2302_state.ta1_overflows = 0;
  am2302_state.error = 0;
  TA1CTL   |= TAIE;
}
