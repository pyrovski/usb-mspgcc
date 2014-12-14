#include "usb_printf.h"
#include "globals.h"
#include "am2302.h"
#include "ppd42.h"

void init_ports(void);
void init_clock(void);

volatile uint8_t usb_printf_state = USB_DISABLED;
__attribute__((critical)) int16_t usb_printf(const char * fmt, ...) {
    if(usb_printf_state == USB_DISABLED) return -1;
    if(usb_printf_state == USB_LOCKED) return -1;
    usb_printf_state = USB_LOCKED;
    int16_t len;
    va_list args;
    va_start(args, fmt);
    char msg[64];
    len = vsnprintf(msg, 64, fmt, args);
    cdcSendDataWaitTilDone((BYTE*) msg,
                            len, CDC0_INTFNUM, 1);
    va_end(args);
    usb_printf_state = USB_ENABLED;
    return len;
}

void usb_printf_init(void) {
    SetVCore(3);
    init_clock();
    //Init USB
    USB_init();                 
    //Enable various USB event handling routines
    USB_setEnabledEvents(kUSB_VbusOnEvent + 
                         kUSB_VbusOffEvent + 
                         kUSB_receiveCompletedEvent + 
                         kUSB_dataReceivedEvent + 
                         kUSB_UsbSuspendEvent + 
                         kUSB_UsbResumeEvent +
                         kUSB_UsbResetEvent);
    // See if we're already attached physically to USB, and if so, 
    // connect to it. Normally applications don't invoke the event 
    // handlers, but this is an exception.
    if(USB_connectionInfo() & kUSB_vbusPresent) {
        usb_printf_state = USB_ENABLED;
        USB_handleVbusOnEvent();
    }
}

void init_clock(void) {
    // enable XT2 pins for F5529
    P5SEL |= 0x0C;

    UCSCTL3 = (UCSCTL3 & ~(SELREF_7)) | (SELREF__REFOCLK);
    UCSCTL4 = (UCSCTL4 & ~(SELA_7)) | (SELA__REFOCLK);

    //Start the USB crystal (XT2)
    XT2_Start(XT2DRIVE_0);

    // use XT2 for FLL source
    UCSCTL3 = SELREF__XT2CLK;

    // use XT1 for ACLK, XT2 for SMCLK and MCLK
    UCSCTL4 = SELA__XT1CLK | SELS__XT2CLK | SELM__XT2CLK;

    //!@todo stuck here
    //Init_FLL_Settle(8000, 4);

    P1DIR |= BIT0;
    P1OUT |= BIT0;
}

void usb_receive_string(void) {
    uint8_t msg_len = 0;
    char msg[MAX_STR_LENGTH];
    //Get the next piece of the string
    msg_len = cdcReceiveDataInBuffer((BYTE*)msg,
        MAX_STR_LENGTH - 1,
        CDC0_INTFNUM);                                                         
    msg[msg_len] = 0;
    bCDCDataReceived_event = FALSE;
    //DEBUG("USB l=%d: %s\r\n", msg_len, msg);
    if(new_adc){
      DEBUG("t=0x%04x, iv=0x%04x\r\n", last_conv, last_adc_iv);
      new_adc = 0;
    }
    if(msg[0] == 'p'){
      DEBUG("Transition count: %d\r\n", PPD_count);
    } else if(msg[0] == 'l'){
      PPD_print_last();
    } else if(msg[0] == 'a'){
      if(am2302_state.phase == 0){
	DEBUG("start am2302 transfer\r\n");
	am2302Start();
	//uint8_t lastState = 0;
	__disable_interrupt();
	uint16_t ta1_capture = 0;
	while(am2302_state.phase > 0){
	  uint8_t newPhase = am2302_state.phase;
	  switch(am2302_state.phase){
	  case -1: // wait
	    break;
	  case 0:
	    break;
	  case 1:
	    // wait for compare
	    while(!(TA1CCTL1 & CCIFG) && !(TA1CTL & TAIFG));
	    if(TA1CCTL1 & CCIFG){
	      ta1_capture = TA1CCR1;

	      TA1CTL &= ~(MC0 | MC1);
	      TA1CCTL1 |= CAP; // enable capture
	      TA1CTL |=  TACLR;

	      P2REN |=  BIT0;  // enable pullup
	      P2DIR &= ~BIT0;  // input
	      P2OUT |=  BIT0;  // P2.0 = high
	      P2SEL |=  BIT0; // Peripheral function
	      TA1CTL |=  MC__CONTINUOUS;
	      TA1CCTL1 &= ~CCIFG;
 	      //uint16_t dummy_capture = TA1CCR1;
	      am2302_state.last_ta1 = 0;
	      newPhase++;
	    } else if(TA1CTL & TAIFG){ // this should never happen
	      TA1CTL &= ~TAIFG;
	      newPhase = -1;
	      am2302_error(am2302_unexpOverflow);
	    }
	    break;
	  default:
	    while(!(TA1CCTL1 & CCIFG) && !(TA1CTL & TAIFG));
	    if(TA1CCTL1 & CCIFG){
 	      ta1_capture = TA1CCR1;
	      TA1CCTL1 &= ~CCIFG;
	      //lastState = !lastState;
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
	  if(newPhase >= 85)
	    newPhase = -1;
	  am2302_state.phase = newPhase;
	}
	__enable_interrupt();
	if(am2302_state.error){
	  am2302_clearFault();
	  while(!am2302_state.reset){
	    while(!(TA1CTL & TAIFG));
	    am2302_overflowEvent();
	  }
	  am2302_state.reset = 0;
	  DEBUG("am2302 error\r\n");
	}
	am2302_dump(&am2302_state);
	if(!am2302_state.error && am2302_state.phase == -1){
	  // normal read
	  while(!am2302_state.reset){
	    while(!(TA1CTL & TAIFG));
	    am2302_overflowEvent();
	  }
	  am2302_state.reset = 0;
	  DEBUG("am2302 ready\r\n");
	}
      } else {
	DEBUG("am2302 busy: %d\r\n", am2302_state.phase);
      }
    } else if(msg[0] == 'm'){
      am2302_dump(&am2302_state);
    }
}
