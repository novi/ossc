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

#include "mouse.h"

static uint8_t mouseSpeed = 0;

static int8_t mouseAccTable1[] = {
    0, // 0
    1, // 1
    1, // 2
    1, // 3
    1, // 4
    2, // 5
    2, // 6
    2, // 7
    2, // 8
    3, // 9
    3, // 10
    3, // 11
    4, // 12
    4, // 13
    4, // 14
    5, // 15
    5, // 16
    6, // 17
    7, // 18
};

static int8_t mouseAccTable2[] = {
    0, // 0
    1, // 1
    1, // 2
    1, // 3
    2, // 4
    2, // 5
    2, // 6
    3, // 7
    3, // 8
    4, // 9
    4, // 10
    5, // 11
    5, // 12
    6, // 13
    6, // 14
    7, // 15
    8, // 16
    9, // 17
    10, // 18
};

static int8_t mouseAccTable3[] = {
    0, // 0
    1, // 1
    1, // 2
    2, // 3
    2, // 4
    3, // 5
    3, // 6
    4, // 7
    4, // 8
    5, // 9
    6, // 10
    7, // 11
    8, // 12
    9, // 13
    10, // 14
    11, // 15
    12, // 16
    13, // 17
    14, // 18
};

void MouseSetSpeed(uint8_t speed)
{
    mouseSpeed = speed;
}


int8_t MouseFixMove(int8_t mov)
{
    uint8_t absx = mov < 0 ? -mov : mov;
    int8_t valx;
    if (mouseSpeed >= 4) {
        absx = absx * 2;
        valx = mov < 0 ? absx: -absx;
        return valx;
    }
    if (mouseSpeed == 3) {
        valx = mov < 0 ? absx: -absx;
        return valx;
    }
    int8_t* mouseAccTable;
    if (mouseSpeed == 0) {
        mouseAccTable = mouseAccTable1;
    } else if (mouseSpeed == 1) {
        mouseAccTable = mouseAccTable2;
    } else {
        // 2
        mouseAccTable = mouseAccTable3;
    }
    
    if (sizeof(mouseAccTable1) > absx) {
        valx = mov < 0 ? mouseAccTable[absx] : -mouseAccTable[absx]; // swap sign
    } else {
        // valx = -(movX/MOUSE_MOVE_SCALE_FACTOR);
        valx = mov < 0 ? mouseAccTable[sizeof(mouseAccTable1)-1] : -mouseAccTable[sizeof(mouseAccTable1)-1];
    }
    return valx;
}