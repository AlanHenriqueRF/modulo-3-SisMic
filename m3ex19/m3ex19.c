#include <msp430.h>
#include <stdint.h>

#include "../libs/uart.h"

int main(void) {
  WDTCTL = WDTPW | WDTHOLD;

  uartOpen(1); 
  uartPrint("Alan Henrique");

  while (1) {
    
  }
}
