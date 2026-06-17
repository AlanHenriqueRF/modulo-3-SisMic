#include <msp430.h>
#include "../libs/i2c.h"
#include "../libs/lcd.h"

int main(void) {
  WDTCTL = WDTPW | WDTHOLD; // stop watchdog timer

  i2cConfig();
  lcdInit();

  lcdPrint("Alan Henrique");
  while(1);
}
