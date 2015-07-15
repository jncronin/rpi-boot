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
#ifndef MBED_GPIO_OBJECT_H
#define MBED_GPIO_OBJECT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "PinNames.h"
#include "cmsis.h"

typedef struct {
    PinName  pin;
    uint32_t mask;

    __IO uint32_t *reg_set;
    __IO uint32_t *reg_clr;
    __I  uint32_t *reg_lev;

    PinMode mode;
} gpio_t;


void pin_pull(PinName pin, PinMode mode);
void pin_mode(PinName pin, PinMode mode);
void gpio_mode (gpio_t *obj, PinMode mode);
void gpio_write(gpio_t *obj, int value);
int gpio_read(gpio_t *obj);
void gpio_init_in(gpio_t* gpio, PinName pin);
void gpio_init_out(gpio_t* gpio, PinName pin);

#ifdef __cplusplus
}
#endif

#endif
