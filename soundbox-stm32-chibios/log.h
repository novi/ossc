#ifndef LOG_H_
#define LOG_H_

#include "hal.h"
#include "chprintf.h"
#include "usbh/debug.h"		/* for _usbh_dbg/_usbh_dbgf */

#ifdef __cplusplus
 extern "C" {
#endif

#define DEBUG_LOG 0

#if DEBUG_LOG
#define LOG_DEBUG(...) _usbh_dbgf(&USBHD1, __VA_ARGS__);
#else
#define LOG_DEBUG(...) LOG_DUMMY(__VA_ARGS__)
inline void LOG_DUMMY(const char *fmt, ...) {
}
#endif

#define LOG(...) _usbh_dbgf(&USBHD1, __VA_ARGS__);
#define LOG_MAIN(...) chprintf((BaseSequentialStream*)&SD2, __VA_ARGS__); // use from main thread


#ifdef __cplusplus
}
#endif


#endif /* LOG_H_ */