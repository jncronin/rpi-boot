#include <stdint.h>
#include "gpio.h"
#include "bcm2836.h"

#define WAIT150CYCLE for(int i=0;i<15;i++){ __asm__("nop"); __asm__("nop"); __asm__("nop"); __asm__("nop"); __asm__("nop"); __asm__("nop"); __asm__("nop"); __asm__("nop"); __asm__("nop"); __asm__("nop");}

void pin_pull(PinName pin, PinMode mode) {
    // vuint32_t *res = GPPUDCLK(pin / 33);
    vuint32_t * res = (vuint32_t *)GPPUDCLK((pin & 0xffffffdf) ? 1:0);

    switch(mode){
        case PullUp:
            *(vuint32_t *)GPPUD = 0x02;
            break;
        case PullDown:
            *(vuint32_t *)GPPUD = 0x01;
            break;
        default:
            // PullNone
            *(vuint32_t *)GPPUD = 0x00;
    }
    // 150 cycle wait
    WAIT150CYCLE;
    // *res = (0x01 << (pin % 32));
    *res = (0x01 << (pin & 0x1F));
    // 150 cycle wait
    WAIT150CYCLE;
    // remove the control signal
    *(vuint32_t *)GPPUD = 0x00;
    // remove the clock
    *res = 0x00;
}

void pin_mode(PinName pin, PinMode mode) {
    if (pin == (PinName)NC) { return; }
    // set pullup, pulldown,...

    // select resister number
    int group = pin / 10;
    vuint32_t *res = (vuint32_t *)GPFSEL(group);
    vuint32_t shift = (pin % 10) * 3;

    switch(mode){
        case Output:
        case Alt0:
        case Alt1:
        case Alt2:
        case Alt3:
        case Alt4:
        case Alt5:
            *res = (mode << shift) | (*res & ~(0x07 << shift));
            break;
        case PullUp:
        case PullDown:
        case PullNone:
            pin_pull(pin, mode);
            break;
        case Input:
            // register clear(3bit)
            *res &= ~(0x07 << shift);
        default:
            return;
    }
}

void gpio_init(gpio_t *obj, PinName pin){
    int group;
    if(pin == NC || 53 < pin) return;
    group = pin / 32;
    //    group = (pin & 0xffffffe0) ? 1:0;
    if (group > 2) return;

    obj->pin = pin;
    //    int mask_point = pin - (32 * group);
    //    obj->mask = 0x01 << mask_point;

    obj->reg_set = (vuint32_t *)GPSET(group);
    obj->reg_clr = (vuint32_t *)GPCLR(group);
    obj->reg_lev = (vuint32_t *)GPLEV(group);
    obj->mask = 0x01 << (pin % 32);
}

void gpio_mode (gpio_t *obj, PinMode mode){
    pin_mode(obj->pin, mode);
}

void gpio_write(gpio_t *obj, int value) {
    if (value)
      *obj->reg_set = obj->mask;
    else
      *obj->reg_clr = obj->mask;
}

int gpio_read(gpio_t *obj) {
    if(obj->mode == Output){
        return ((*obj->reg_lev & obj->mask) ? 1 : 0);
    }
    else{
        return ((*obj->reg_lev & obj->mask) ? 1 : 0);
    }
}


// the following set of functions are generic and are implemented in the common gpio.c file
void gpio_init_in(gpio_t* gpio, PinName pin){
    gpio_init(gpio, pin);
    gpio_mode(gpio, Input);
}


void gpio_init_out(gpio_t* gpio, PinName pin){
    gpio_init(gpio, pin);
    gpio_mode(gpio, Output);
}
