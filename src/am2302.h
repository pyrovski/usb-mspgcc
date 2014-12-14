#ifndef AM2302_H
#define AM2302_H
#include <stdint.h>

typedef struct {
  /*
    phase 0: idle (host pull-up)
    phase 1: host initiate (1-10ms)
    phase 2: wait for response (20-40us)
    phase 3: sensor low (80us)
    phase 4: sensor high (80us)
    phase 5: data (40 bits, 78-120us per bit: 50us low, 26-28/70us high)
    phase 6: sensor low (?us)
    phase 7: 2s wait
   */
  
  volatile int8_t   phase;
  volatile uint8_t  errorPhase;
  volatile uint8_t  bit         :6 ;
  union {
    volatile struct {
      volatile int16_t  temperature;
      volatile uint16_t relativeHumidity;
      volatile uint8_t  checksum;
    };
    volatile uint64_t data       :40;
  };
  volatile uint8_t  level        :1 ;
  volatile uint8_t  ta1_overflows:6 ;
  volatile uint8_t  newData      :1 ;
  volatile uint8_t  reset        :1 ;
  volatile uint8_t  error        :1 ;
  volatile uint8_t  errorCode    :3 ;

  volatile uint16_t timing[100];
  volatile uint16_t last_ta1;
  volatile uint8_t dbit;

} am2302_state_s;

typedef enum {
  am2302_noError       = 0,
  am2302_expHigh       = 1,
  am2302_expLow        = 2,
  am2302_unexpOverflow = 3,
  am2302_defPhase      = 4,
  am2302_unexpInput    = 5,
  am2302_cov           = 6,
  am2302_tooManyTrans  = 7
} am2302_errorCode_t;

void am2302_dump();
void am2302Low();
void am2302InPullup();
void am2302OutHigh();
void am2302Start();
void am2302_clearFault();
void am2302_event(const uint16_t ta1_capture);
void am2302_overflowEvent();
void am2302_error(uint8_t code);
void am2302Finish();
#endif
