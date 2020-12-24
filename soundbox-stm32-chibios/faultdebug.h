// https://interrupt.memfault.com/blog/cortex-m-fault-debug
#ifndef FALUTDEBUG_H
#define FALUTDEBUG_H

#define USE_DEBUGGING (1)

// NOTE: If you are using CMSIS, the registers can also be
// accessed through CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk
#if USE_DEBUGGING

#define HALT_IF_DEBUGGING()                              \
  do {                                                   \
    if ((*(volatile uint32_t *)0xE000EDF0) & (1 << 0)) { \
      __asm("bkpt 1");                                   \
    }                                                    \
} while (0)

#else

#define HALT_IF_DEBUGGING()                              \
  do {                                                   \
} while (0)

#endif

#define HARDFAULT_HANDLING_ASM(_x)               \
  __asm volatile(                                \
      "tst lr, #4 \n"                            \
      "ite eq \n"                                \
      "mrseq r0, msp \n"                         \
      "mrsne r0, psp \n"                         \
      "b my_fault_handler_c \n"                  \
                                                 ) \


#endif // FALUTDEBUG_H