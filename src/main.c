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
volatile uint16_t last_conv = 0;
volatile uint8_t new_adc = 0;
volatile uint16_t last_adc_iv = 0;

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

#pragma vector=ADC12_VECTOR
__interrupt void ADC_ISR(void){
  uint16_t iv = ADC12IV;
  last_adc_iv = iv;
  if(iv & ADC12IV_ADC12IFG0){
    // ADC12MEM0 result is ready
    last_conv = ADC12MEM0;
    new_adc = 1;
  }
}

/** Initializes modules and pins */
void msp_init(void) {
    // Stop the watchdog
    wdt_stop();

    // Initialize USB interface and clocks
    usb_printf_init();

    // configure ADC for temperature
    REFCTL0 &= ~REFMSTR;

    // 1.5V reference
    ADC12CTL0 = 
      ADC12ON      | // turn on ADC
      ADC12REFON   | // turn on ADC reference
      //ADC12REF2_5V | // 2.5V reference
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
      ADC12PDIV        |  // predivide ADC clock by 4
      ADC12RES1        ;  // 12-bit resolution
    ADC12MCTL0 = 
      ADC12INCH_10     | // select temperature diode
      //ADC12INCH_11     | // select (Vcc - Vss)/2
      ADC12SREF_1      ; // VREF+/AVSS
    ADC12IE = 
      ADC12IE0         ; // enable interrupt on ADC12 mem 0 result

    /* 
       ------------------------------------------------------------
       sensors with digital interfaces:
       P1.2: PPD42
       P2.0: AM2302

       PPD42: Duty cycle-based measurement. Average duty cycle over 30
       seconds corresponds to a measurement of particle concentration.

       Low %	concentration (pcs/283ml = .01cf)
       0	0
       1	500
       2	1000
       3	1600
       4	2100
       5	2600
       6	3200
       7	3800
       8	4500
       9	5200
       10	5900
       11	6600
       12	7500
       13	8500

       AM2302: digital encoding. Events range in duration from 20-80us
       to 1-10ms. Measurement should be queried at intervals >= 2s.
       
       ------------------------------------------------------------
       Protocol: 

       Init: Host sets pull-up.
       
       Communication: Host pulls low 1-10ms; after 20-40us, device
       pulls low for 80us, followed by high for 80us. 40 bits of data
       will follow, corresponding to 16 bits of humidity data, 16 bits
       of temperature data, and 8 bits of checksum. Each bit is
       transmitted as a low-high sequence, with 50us low and 26-28us
       or 70us high for 0 or 1, respectively.

       Encoding: raw binary, with msb sign for temperature in Celsius. RH
       is in 10ths of a percent. Checksum is the least significant 8
       bits of the sum of the first four 8-bit words (humidity and
       temperature data)
       
       ------------------------------------------------------------
       Implementation: 

       PPD42: set TA0.1 to capture and interrupt on 1->0, then
       0->1. Maintain a running average duty cycle. Timer A0 should
       have a period less than 100us. Input P1.2 can be assigned to
       timer functionality, as we never need to control the output.

       AM2302: set TA1.1 to capture and interrupt on 1->0, then
       0->1. Track protocol state. Start measurement transmission
       every 2s by accumulating TA1 overflow events. Timer A1 should
       have a period of ~1us. Pin P2.0 must transition between input
       and output, so careful transfer of control is required.
    */

    // ------------------------------------------------------------
    // SMCLK divider: 16 -> 1 MHz
    UCSCTL5 = (UCSCTL5 & ~DIVS_7) | DIVS__16;

    // ------------------------------------------------------------
    // PPD42

    P1REN  =  BIT2;
    P1SEL  =  BIT2; // peripheral function
    P1IFG  =  0;
    P1IES  =  BIT2; // interrupt on 1->0
    P1DIR  =  BIT2;
    //!@todo ISR
    //P1IE   =  BIT2;

    // SMCLK is 1 MHz, so SMCLK/8 = 125 kHz, or 8 us period
    TA0CTL  = TASSEL__SMCLK | ID__8 | MC__CONTINUOUS;
    TA0CCTL0 = CM_3 | CCIS_1 | SCS | SCCI | CAP;
    TA0EX0 = TAIDEX_3; // further divide by 4 -> 31.250 kHz, or 32 us period 
    
    //!@todo ISR
    //TA0CTL |= TAIE;
    //TA0CCTL0 |= CCIE;

    // ------------------------------------------------------------
    // AM2302

    P2REN  =  BIT0;
    P2SEL ^= ~BIT0; // IO function
    P2IFG  =  0;
    P2IES  =  BIT0; // interrupt on 1->0
    P2DIR  =  BIT0;
    //!@todo ISR
    //P2IE   =  BIT0;

    TA1CTL = TASSEL__SMCLK | ID__1 | MC__CONTINUOUS; // 1 MHz
    TA1CCTL0 = CM_3 | CCIS_0 | SCS | SCCI | CAP;

    //!@todo ISR
    //TA1CTL |= TAIE;
    //TA1CCTL0 |= CCIE;

    // ------------------------------------------------------------
    // Enable global interrupts
    __enable_interrupt();

    // ------------------------------------------------------------
    // start conversion
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
