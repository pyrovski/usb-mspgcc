/** The following code uses TI's USB library
 *  for MSP430 to enable printf functionalities
 *  in the microcontroller.  It uses a USB CDC 
 *  interface to create a serial port, which
 *  can be accessed with standard software
 *  such as CoolTerm, HyperTerm, etc.
 *
 *  Please note that this library only works 
 *  in microcontrollers with USB capabilities, 
 *  such as the msp430f552x series.
 *
 *  To build it, just run:
 * 
 *  rake
 *
 *  To install it in the msp-exp430f5529 board:
 *  sudo mspdebug rf2500 "prog bin.elf"
 *
 *  This code was modified from the USB
 *  Developers Package available from the TI
 *  website. This code is provided as-is
 *  and comes with no warranty or support
 *  whatsoever. You are free to use it as long
 *  as you cite this work.
 *
 *  Enjoy!
 *
 *  Jose L. Honorato
 *  April, 2013
 */

#include <msp430.h>
#include <stdint.h>

#include "usb_printf.h"

//Indicates data has been received without an open rcv operation
volatile BYTE bCDCDataReceived_event = FALSE;
volatile uint16_t last_conv;

// Function declarations
void msp_init(void);
uint8_t events_available(void);
void process_events(void);

// To activate testing the rake command
// should be invoked like this:
// rake test=1
#ifdef TESTING
#include "test.h"
#endif

int main(void) {
    // If TESTING is defined at compile time then
    // we don't run the main application
    // This can be done by executing
    // rake test=1
#ifdef TESTING
    return test();
#endif
    // Initialize the different parts
    msp_init();

    DEBUG("Running\r\n");

    // Main loop
    while(1) {  
        // Check if there are events available. If there are
        // then we process them
        if(events_available()) {
            process_events();
        }
    }
}

/*
#ifndef interrupt
#define interrupt(x) void __attribute__((interrupt (x)))
#endif
*/
#pragma vector=ADC12_VECTOR
__interrupt void ADC_ISR(void){
  uint16_t iv = ADC12IV;
  if(iv & ADC12IV_ADC12IFG0){
    // ADC12MEM0 result is ready
    last_conv = ADC12MEM0;
  }
}

/** Initializes modules and pins */
void msp_init(void) {
    // Stop the watchdog
    wdt_stop();

    // Initialize USB interface
    usb_printf_init();

    // configure ADC for temperature
    // 1.5V reference
    ADC12CTL0 = 
      ADC12ON      | // turn on ADC
      ADC12REFON   | // turn on ADC reference
      ADC12MSC     | // automatic conversion restart
      ADC12SHT0_12 ; // 1024 ADC12CLK cycles sample-and-hold
    ADC12CTL1 = 
      ADC12CONSEQ1     | // repeat single channel
      ADC12DIV0        | 
      ADC12DIV1        |
      ADC12DIV2        | // divide ADC clock by 8
      ADC12SHP         | // trigger sample-and-hold from timer
      ADC12CSTARTADD_0 ; // store result in conversion address 0
    // temperature sensor on
    ADC12CTL2 = 
      ADC12PDIV |  // predivide ADC clock by 4
      ADC12RES1 ;  // 12-bit resolution
    ADC12MCTL0 = 
      ADC12INCH_10 | // select temperature diode
      ADC12SREF_1  ; // VREF+/AVSS
    ADC12IE = 
      ADC12IE0 ; // enable interrupt on ADC12 mem 0 result

    // Enable global interrupts
    __enable_interrupt();

    ADC12CTL0 |= ADC12ENC | ADC12SC;
}

/** Checks if there are events available to be processed
 * The currently supported events are:
 * - There are USB packets available
 *
 *   These events are checked quite frequently because the main
 *   loop is constantly checking them. Anyhow, whenever we need to process
 *   a lengthy task, the main loop will cease to check for an event. 
 *   This doesn't mean that the events will be dropped, but they will 
 *   not be managed instantly.
 */
uint8_t events_available(void) {
    return (
        //data received event
        bCDCDataReceived_event
        );
}

/** Process then events queried in the \ref events_available() function */
void process_events(void) {
    // Data received from USB
    if(bCDCDataReceived_event == TRUE) {
        usb_receive_string();
    }   
}
