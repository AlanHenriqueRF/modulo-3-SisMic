#include <msp430.h>
#include <stdint.h>
#include <stdio.h>

#include "../libs/adc.h"
#include "../libs/i2c.h"
#include "../libs/lcd.h"
#include "../libs/uart.h"

#define TERM_CLR "\033[2J"
#define TERM_HOME "\033[0;0H"

volatile uint8_t rxQntCanal = 1;
volatile uint8_t flagReconfigurar = 0;
volatile uint8_t flagNovaAmostra = 0;

volatile uint16_t adcValues[8];

void timerConfig(uint8_t canais);

int main(void) {
  WDTCTL = WDTPW | WDTHOLD;

  uartOpen(1);
  i2cConfig();
  lcdInit();

  uartPrint("TERM_CLR");
  uartPrint(TERM_CLR);
  uartPrint(TERM_HOME);
  rxQntCanal = 1;

  adcOscConfig(rxQntCanal);
  timerConfig(rxQntCanal);

  UCA1IE |= UCRXIE;
  __enable_interrupt();
  P1DIR |= BIT0;

  while (1) {

    if (flagReconfigurar) {
     flagReconfigurar = 0;

      TA0CTL = MC__STOP;     // para timer antes
      ADC12IE = 0;           // desliga interrupções ADC
      ADC12IFG = 0;          // limpa flags antigas

      adcOscConfig(rxQntCanal);
      timerConfig(rxQntCanal);

      uartPrint(TERM_CLR);
      uartPrint(TERM_HOME);
    }

    if (flagNovaAmostra) {
      flagNovaAmostra = 0;

      uartPrint(TERM_CLR);
      uartPrint(TERM_HOME);

      char str[20];
      uint8_t i;

      for (i = 0; i < rxQntCanal; i++) {
        sprintf(str, "%d: %d", i, adcValues[i]);
        uartPrint(str);
        uartPrint("\n");
      }

      char lcdStr[17];
      sprintf(lcdStr, "0: %d", adcValues[0] & 0x0FFF);

      lcdClear();
      lcdHome();
      lcdPrint((u8 *)lcdStr);
    }
  }
}

void timerConfig(uint8_t canais) {
  TA0CTL = MC__STOP;

  TA0CCR0 = (32768/2) - 1; // 1 segundo

  TA0CCTL0 = CCIE;
  TA0CTL = TASSEL__ACLK | MC__UP | TACLR;
}


// Dispara o SC do adc, o start conversion, e começa a converter
#pragma vector = TIMER0_A0_VECTOR
__interrupt void TIMER0_A0_ISR(void) { 
  if (!(ADC12CTL1 & ADC12BUSY)) {
    ADC12CTL0 &= ~ADC12SC;
    ADC12CTL0 |= ADC12SC;
  }
}

// interrupção gerada para quantificar quantos canais o usuario quer usar
#pragma vector = USCI_A1_VECTOR
__interrupt void USCI_A1_ISR(void) {
  switch (__even_in_range(UCA1IV, 4)) {
  case 0:
    break;
  case 2: {
    char c = UCA1RXBUF;

    if (c >= '1' && c <= '8') {
      rxQntCanal = c - '0';
      flagReconfigurar = 1;
    }
    break;
  }
  }
}

#pragma vector = ADC12_VECTOR
__interrupt void ADC12_interrupt(void){
    switch (_even_in_range(ADC12IV, 0x24)) {
    case ADC12IV_ADC12IFG0:       //MEM0 Ready
        //Pego o valor no MEM0.
        adcValues[0] = ADC12MEM0;
        flagNovaAmostra = 1;
        break;
    case ADC12IV_ADC12IFG1:      //MEM1 Ready
        adcValues[0] = ADC12MEM0;
        adcValues[1] = ADC12MEM1;
        flagNovaAmostra = 1;
        break;
    case ADC12IV_ADC12IFG2:      //MEM2 Ready
        adcValues[0] = ADC12MEM0;
        adcValues[1] = ADC12MEM1;
        adcValues[2] = ADC12MEM3;
        flagNovaAmostra = 1;
        break;
    case ADC12IV_ADC12IFG3:      //MEM3 Ready
        adcValues[0] = ADC12MEM0;
        adcValues[1] = ADC12MEM1;
        adcValues[2] = ADC12MEM3;
        adcValues[3] = ADC12MEM3;
        flagNovaAmostra = 1;
        break;
    case ADC12IV_ADC12IFG4:      //MEM4 Ready
        adcValues[0] = ADC12MEM0;
        adcValues[1] = ADC12MEM1;
        adcValues[2] = ADC12MEM3;
        adcValues[3] = ADC12MEM3;
        adcValues[4] = ADC12MEM3;
        flagNovaAmostra = 1;
        break;
    case ADC12IV_ADC12IFG5:      //MEM5 Ready
        adcValues[0] = ADC12MEM0;
        adcValues[1] = ADC12MEM1;
        adcValues[2] = ADC12MEM3;
        adcValues[3] = ADC12MEM3;
        adcValues[4] = ADC12MEM3;
        adcValues[5] = ADC12MEM3;
        flagNovaAmostra = 1;
        break;
    case ADC12IV_ADC12IFG6:      //MEM6 Ready
        adcValues[0] = ADC12MEM0;
        adcValues[1] = ADC12MEM1;
        adcValues[2] = ADC12MEM3;
        adcValues[3] = ADC12MEM3;
        adcValues[4] = ADC12MEM3;
        adcValues[5] = ADC12MEM3;
        adcValues[6] = ADC12MEM3;
        flagNovaAmostra = 1;
        break;
    case ADC12IV_ADC12IFG7:      //MEM7 Ready
        adcValues[0] = ADC12MEM0;
        adcValues[1] = ADC12MEM1;
        adcValues[2] = ADC12MEM3;
        adcValues[3] = ADC12MEM3;
        adcValues[4] = ADC12MEM3;
        adcValues[5] = ADC12MEM3;
        adcValues[6] = ADC12MEM3;
        adcValues[7] = ADC12MEM3;
        flagNovaAmostra = 1;
        break;
    }
}
