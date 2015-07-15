#include "jtag.h"

void jtag_enable(void){
    pin_mode(JTAG_TRST, Alt4);
    pin_mode(JTAG_TDI, Alt5);
    pin_mode(JTAG_TMS, Alt4);
    pin_mode(JTAG_TCK, Alt4);
    pin_mode(JTAG_TDO, Alt4);
}
