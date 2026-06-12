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

  P1DIR &= ~BIT1;
  P1OUT |= BIT1;
  P1REN |= BIT1;

  P4DIR |= BIT7;
  P4OUT &= ~BIT7;
  
  __enable_interrupt();

  // lcdPrint("Hello WOrld !\n"); 
  while (1){
    while(P1IN & BIT1);
    debounce(5000);
    lcdWriteByte( 0x01, INSTR);
    P4OUT ^= BIT7;
    while(!P1IN & BIT1);
    debounce(5000);
    P4OUT ^= BIT7;
  }
}

#pragma vector = PORT2_VECTOR;
__interrupt void isr_S1(){
  debounce(10000);
  switch (P2IV) {
    case 0x04:
      debounce(10000);
      lcdPrint("Alan seu lindao enche a garrafa");
      break;
  }
  
}

void debounce(int valor) {
  volatile int x;
  // volatile evita optimizador do compilador
  for (x = 0; x < valor; x++)
    ; // Apenas gasta tempo
}
