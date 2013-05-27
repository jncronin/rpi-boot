/* Copyright (C) 2013 by John Cronin <jncronin@tysos.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "output.h"
#include "uart.h"
#include "console.h"

rpi_boot_output_state ostate;

rpi_boot_output_state output_get_state()
{
    return ostate;
}

void output_restore_state(rpi_boot_output_state state)
{
    ostate = state;
}

void output_disable_fb()
{
    ostate &= ~RPIBOOT_OUTPUT_FB;
}

void output_enable_fb()
{
#ifdef ENABLE_FRAMEBUFFER
    ostate |= RPIBOOT_OUTPUT_FB;
#endif
}

void output_disable_uart()
{
    ostate &= ~RPIBOOT_OUTPUT_UART;
}

void output_enable_uart()
{
#ifdef ENABLE_SERIAL
    ostate |= RPIBOOT_OUTPUT_UART;
#endif
}

void output_init()
{
    ostate = 0;
}

int split_putc(int c)
{
    int ret = 0;
#ifdef ENABLE_SERIAL
    if(ostate & RPIBOOT_OUTPUT_UART)
        ret = uart_putc(c);
#endif
#ifdef ENABLE_FRAMEBUFFER
    if(ostate & RPIBOOT_OUTPUT_FB)
        ret = console_putc(c);
#endif
    return ret;
}
