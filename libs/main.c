#include <msp430.h>
#include <stdint.h>
#include "i2c.h"
#include "lcd.h"
#include "types.h"

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;

    return 1;
}
