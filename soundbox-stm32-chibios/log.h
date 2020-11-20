#ifndef LOG_H_
#define LOG_H_

#include "chprintf.h"

#ifdef __cplusplus
 extern "C" {
#endif

#define DEBUG_LOG 1

#if DEBUG_LOG
#define LOG_DEBUG(...) chprintf((BaseSequentialStream*)&SD2, __VA_ARGS__);
#else
#define LOG_DEBUG LOG_DUMMY
void LOG_DUMMY(const char *fmt, ...) {
}
#endif

#define LOG(...) chprintf((BaseSequentialStream*)&SD2, __VA_ARGS__);


#ifdef __cplusplus
}
#endif


#endif /* LOG_H_ */