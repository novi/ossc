/*

   Copyright 2021-24 Yusuke Ito

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

*/

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
#define LOG_DEBUG(...) do { } while(0);
#endif

#define LOG(...) do { } while(0); // _usbh_dbgf(&USBHD1, __VA_ARGS__);, not work?
#define LOG_MAIN(...) chprintf((BaseSequentialStream*)&SD2, __VA_ARGS__); // use from main thread
#define LOG_BACKGROUND(...) _usbh_dbgf(&USBHD1, __VA_ARGS__);

#ifdef __cplusplus
}
#endif


#endif /* LOG_H_ */