/* mbed Microcontroller Library
 * Copyright (c) 2006-2013 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef MBED_PINNAMES_H
#define MBED_PINNAMES_H

#include "cmsis.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GP_0 = 0,
           GP_1, GP_2, GP_3, GP_4, GP_5, GP_6, GP_7, GP_8, GP_9, GP_10, GP_11, GP_12, GP_13, GP_14, GP_15,
    GP_16, GP_17, GP_18, GP_19, GP_20, GP_21, GP_22, GP_23, GP_24, GP_25, GP_26, GP_27, GP_28, GP_29, GP_30,
    GP_31, GP_32, GP_33, GP_34, GP_35, GP_36, GP_37, GP_38, GP_39, GP_40, GP_41, GP_42, GP_43, GP_44, GP_45,
    GP_46, GP_47, GP_48, GP_49, GP_50, GP_51, GP_52, GP_53,

    // LEDs
    LED_ACT = GP_47,
    LED_PWR = GP_35,

    // UART
    UART_TXD0 = GP_14,
    UART_RXD0 = GP_15,

    // JTAG
    JTAG_TRST = GP_22,
    JTAG_TDI = GP_4,
    JTAG_TMS = GP_27,
    JTAG_TCK = GP_25,
    JTAG_TDO = GP_24,

    // Not connected
    NC = (int)0xFFFFFFFF
} PinName;

typedef enum {
    Input = 0,
    Output = 1,
    Alt5 = 2,
    Alt4 = 3,
    Alt0 = 4,
    Alt1 = 5,
    Alt2 = 6,
    Alt3 = 7,
    PullUp = 8,
    PullDown = 9,
    PullNone = 10,
    // OpenDrain = 11,
    PullDefault = PullUp
} PinMode;


#ifdef __cplusplus
}
#endif

#endif
