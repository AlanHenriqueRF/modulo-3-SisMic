#include <msp430.h>

int main(void) {
  WDTCTL = WDTPW | WDTHOLD; // stop watchdog timer
}
