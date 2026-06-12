#include <msp430.h> 

// - Esse código transmite 0x00 / 0xFF para o LCD
// - Endereço do LCD: 0x3F
// - UCB0 é configurado como MASTER TRANSMITTER
//Hardware setup:
// - P3.0 - SDA
// - P3.1 - SCL
// - Alimentar o LCD e conectar SDA / SCL
// - O código NÃO liga os resistores de pull-up internos, então se o LCD não tiver esses resistores eles tem que ser colocados na protoboard.


#define LED_RED_ON      (P1OUT |= BIT0)
#define LED_RED_OFF     (P1OUT &= ~BIT0)

#define LED_GREEN_ON      (P4OUT |= BIT7)
#define LED_GREEN_OFF     (P4OUT &= ~BIT7)



void initialize_I2C_UCB0_MasterTransmitter();
void master_TransmitOneByte(unsigned char address, unsigned char data);

void delay_us(unsigned int time_us);


/**
 * main.c
 */
int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer

    initialize_I2C_UCB0_MasterTransmitter();

    //LED VERMELHO
    P1SEL &= ~BIT0;
    P1DIR |= BIT0;
    P1OUT &= ~BIT0;

    //LED VERDE
    P4SEL &= ~BIT7;
    P4DIR |= BIT7;
    P4OUT &= ~BIT7;

    LED_RED_OFF;
    LED_GREEN_OFF;

    while(1)
    {

        master_TransmitOneByte(0x3F, 0xFF);

        volatile int n;
        for (n = 0; n < 10; n++)
        {
            delay_us(50000);
        }

        master_TransmitOneByte(0x3F, 0x00);

        for (n = 0; n < 10; n++)
        {
            delay_us(50000);
         }



        }

        return 0;
}

/*
 *  P3.0 - SDA
 *  P3.1 - SCL
 */
void initialize_I2C_UCB0_MasterTransmitter()
{
    //Desliga o módulo
    UCB0CTL1 |= UCSWRST;

    //Configura os pinos
    P3SEL |= BIT0;     //Configuro os pinos para "from module"
    P3SEL |= BIT1;
    P3REN &= ~BIT0; //Resistores externos.
    P3REN &= ~BIT1;


    UCB0CTL0 = UCMST |           //Master Mode
                          UCMODE_3 |    //I2C Mode
                          UCSYNC;         //Synchronous Mode

    UCB0CTL1 = UCSSEL__ACLK |    //Clock Source: ACLK
                          UCTR |                      //Transmitter
                          UCSWRST ;             //Mantém o módulo desligado

    //Divisor de clock para o BAUDRate
    UCB0BR0 = 2;
    UCB0BR1 = 0;

    //Liga o módulo.
    UCB0CTL1 &= ~UCSWRST;

    //Se eu quisesse ligar interrupções eu iria fazer isso aqui, depois de re-ligar o módulo..
}

void master_TransmitOneByte(unsigned char address, unsigned char data)
{
    //Desligo todas as interrupções
    UCB0IE = 0;

    //Coloco o slave address
    UCB0I2CSA = address;

    //Espero a linha estar desocupada.
    if (UCB0STAT & UCBBUSY) return;

    //Peço um START
    UCB0CTL1 |= UCTXSTT;

    //Espero até o buffer de transmissão estar disponível
    while ((UCB0IFG & UCTXIFG) == 0);

    //Escrevo o dado
    UCB0TXBUF = data;

    //Aguardo o acknowledge
    while (UCB0CTL1 & UCTXSTT);

    //Verifico se é um ACK ou um NACK
    if ((UCB0IFG & UCNACKIFG) != 0)
    {
        //Peço uma condição de parada
        LED_RED_ON;
        UCB0CTL1 |= UCTXSTP;
    } else
    {
        //Peço uma condição de parada
        LED_GREEN_ON;
        UCB0CTL1 |= UCTXSTP;
    }

    return;
}

/*
 * Delay microsseconds.
 */
void delay_us(unsigned int time_us)
{
    //Configure timer A0 and starts it.
    TA0CCR0 = time_us;
    TA0CTL = TASSEL__SMCLK | ID__1 | MC_1 | TACLR;

    //Locks, waiting for the timer.
    while((TA0CTL & TAIFG) == 0);

    //Stops the timer
    TA0CTL = MC_0 | TACLR;
}


