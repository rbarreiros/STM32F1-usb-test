
#include <stdio.h>
#include <string.h>

#include "ch.h"
#include "hal.h"
#include "usbcfg.h"

static THD_WORKING_AREA(waThread2, 8192);
static __attribute__((noreturn)) THD_FUNCTION(Thread2, arg)
{
  event_listener_t el1;
  eventflags_t flags;
  uint8_t buff[128];
  
  chRegSetThreadName("USB");

  (void)arg;
  
  sduObjectInit(&SDU1);
  sduStart(&SDU1, &serusbcfg);

  usbDisconnectBus(serusbcfg.usbp);
  chThdSleepMilliseconds(1500);
  usbStart(serusbcfg.usbp, &usbcfg);
  usbConnectBus(serusbcfg.usbp);

  chEvtRegisterMask(chnGetEventSource(&SDU1), &el1, ALL_EVENTS);

  while(USBD1.state != USB_READY) chThdSleepMilliseconds(10);
  while(SDU1.state != SDU_READY) chThdSleepMilliseconds(10);

  while(true)
  {
    chEvtWaitAny(ALL_EVENTS);
    chSysLock();
    flags = chEvtGetAndClearFlagsI(&el1);
    chSysUnlock();

    bool read = false;
    unsigned int len = 0;
    
    if (flags & CHN_INPUT_AVAILABLE)
    {
      read = true;

      while(true)
      {
        len = chnReadTimeout((BaseChannel *)&SDU1, buff, sizeof(buff), TIME_IMMEDIATE);
        if(len == 0 || len < sizeof(buff))
          break;
      }
    }

    if(read)
    {
      read = false;
      chnWriteTimeout((BaseChannel *)&SDU1, buff, len, TIME_IMMEDIATE);
      palTogglePad(GPIOC, 13);
    }
    
    //chThdSleepMicroseconds(10);
  }
}

int __attribute__((noreturn)) main(void) {
  halInit();
  chSysInit();

  chThdCreateStatic(waThread2, sizeof(waThread2), NORMALPRIO, Thread2, NULL);

  while (true) {
    chThdSleepMilliseconds(1000);
  }
}
