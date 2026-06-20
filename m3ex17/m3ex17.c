#include <msp430.h>
#include <stdint.h>

int main(void) {
  WDTCTL = WDTPW | WDTHOLD;

  while (1) {
  }
}
