#include <msp430.h>
#include "am2302.h"
#include "globals.h"

void am2302Low(){
  P2SEL ^= ~BIT0; // IO function
  P2DIR |=  BIT0; // output
  P2OUT &= ~BIT0; // P2.0 = low
}

void am2302InPullup(){
  TA1CCTL1 &= ~CCIE;
  P2OUT |=  BIT0; // P2.0 = high
  P2REN |=  BIT0; // enable pullup
  P2DIR &= ~BIT0; // input
  P2SEL |=  BIT0; // peripheral function
  TA1CCTL1 |= CAP; // enable capture
  TA1CCTL1 |= CCIE;
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
  TA1CCTL1 &= ~CAP;
  TA1CCR1   = 5000; // 5ms
  
  am2302Low();
  TA1R      = 0;
  am2302_state.phase = 1;
  am2302_state.bit = 0;
  am2302_state.data = 0;
  
  TA1CCTL1 |= CCIE;
  TA1CTL   |= TAIE;
}
