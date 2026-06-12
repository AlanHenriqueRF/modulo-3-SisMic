#include "./libs/i2c.h"
#include "./libs/lcd.h"
#include <msp430.h>
#include <stdint.h>

void debounce(int valor);

int main(void) {
  WDTCTL = WDTPW | WDTHOLD; // stop watchdog timer

  i2cConfig();
  lcdInit();
  
  P2DIR &= ~BIT1;
  P2OUT |= BIT1;
  P2REN |= BIT1;
  P2IE |= BIT1;

  __enable_interrupt();

  // lcdPrint("Hello WOrld !\n"); 
  while (1);
}

#pragma vector = PORT2_VECTOR;
__interrupt void isr_S1(){
  debounce(5000);
  switch (P2IV) {
    case 0x04:
      lcdWriteByte(0x47, 1);
      break;
  }
  
}

void debounce(int valor) {
  volatile int x;
  // volatile evita optimizador do compilador
  for (x = 0; x < valor; x++)
    ; // Apenas gasta tempo
}
