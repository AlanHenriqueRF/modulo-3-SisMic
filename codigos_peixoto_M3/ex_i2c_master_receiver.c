#include <msp430.h> 

// - Esse código transmite um byte para o PCF usando o UCB0
//   e depois lê o byte da PCF usando o UCB1.
// - Endereço do LCD: 0x3F
// - UCB0 é configurado como MASTER TRANSMITTER
// - UCB1 é configurado como MASTER RECEIVER
//Hardware setup:
// - P3.0 - P4.1 - SDA
// - P3.1 - P4.2 - SCL
// - Alimentar o LCD e conectar SDA / SCL
// - O código NÃO liga os resistores de pull-up internos, então se o LCD não tiver esses resistores eles tem que ser colocados na protoboard.


#define LED1_ON             (P1OUT |= BIT0)
#define LED1_OFF           (P1OUT &= ~BIT0)
#define LED1_TOGGLE   (P1OUT ^= BIT0)

#define LED2_ON              (P4OUT |= BIT7)
#define LED2_OFF            (P4OUT &= ~BIT7)
#define LED2_TOGGLE    (P4OUT ^= BIT7)




void initialize_I2C_UCB0_MasterTransmitter();
void master_I2C_UCB0_TransmitOneByte(unsigned char address, unsigned char data);
unsigned char byteTransmitted;

void initialize_I2C_UCB1_MasterReceiver();
void master_I2C_UCB1_ReceiveOneByte(unsigned char address);
unsigned char byteReceived;
unsigned int rxFlag;

void delay_us(unsigned int time_us);

void configLED1()
{
    P1SEL &= ~BIT0;
    P1DIR |= BIT0;
    P1OUT &= ~BIT0;
}

void configLED2()
{
    P4SEL &= ~BIT7;
    P4DIR |= BIT7;
    LED2_OFF;
}

/**
 * main.c
 */
int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer

    configLED1();
    configLED2();

    initialize_I2C_UCB0_MasterTransmitter();
    initialize_I2C_UCB1_MasterReceiver();

    byteTransmitted = 0x08;

    while(1)
    {
        //Acendo
        LED1_ON;

        master_I2C_UCB0_TransmitOneByte(0x3F, byteTransmitted);

        volatile int n;
        for (n = 0; n < 10; n++)
        {
            delay_us(50000);
        }

        LED1_OFF;

        //Apago
        master_I2C_UCB1_ReceiveOneByte(0x3F);

       //Aguarda a flag de RX.
       while (rxFlag == 0);

       //Comparo o byte enviado com o byte recebido.
       //Pelo circuito, o bit P3 é sempre lido como zero, então eu comparo os demais bits.
       //if ((rxFlag == 1) && (byteReceived == byteTransmitted))
       if ((rxFlag == 1) && ((byteReceived & 0xF7) == (byteTransmitted & 0xF7)))
       {
           LED2_ON;
       } else
       {
           LED2_OFF;
       }

       //Peço uma condição de parada
       UCB1CTL1 |= UCTXSTP;

       //Aguardo a condição
       while ((UCB1CTL1 & UCTXSTP) != 0);


        for (n = 0; n < 10; n++)
        {
            delay_us(50000);
         }

        byteTransmitted += 0x08;

    }

    return 0;
}

/*
 *  P3.0 - SDA
 *  P3.1 - SCL
 */
void initialize_I2C_UCB0_MasterTransmitter()
{
    __disable_interrupt();

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

    UCB0IE = 0;

}

void master_I2C_UCB0_TransmitOneByte(unsigned char address, unsigned char data)
{
    //Coloco o slave address
    UCB0I2CSA = address;

    //Espero a linha estar desocupada.
    while (UCB0STAT & UCBBUSY);

    //Peço um START
    UCB0CTL1 |= UCTXSTT;

    //Espero até o buffer de transmissão estar disponível
    while ((UCB0IFG & UCTXIFG) == 0);

    //Escrevo o dado
    UCB0TXBUF = data;

    //Aguardo o acknowledge
    while (UCB0CTL1 & UCTXSTT);

    //Verifico se é um ACK ou um NACK
    if (UCB0IFG & UCTXNACK != 0)
    {
        //Peço uma condição de parada
        UCB0CTL1 |= UCTXSTP;
    } else
    {
        //Peço uma condição de parada
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

/*
 *  P4.1 - SDA
 *  P4.2 - SCL
 */
void initialize_I2C_UCB1_MasterReceiver()
{
    //Desliga o módulo
    UCB1CTL1 |= UCSWRST;

    //Configura os pinos
    P4SEL |= BIT1;     //Configuro os pinos para "from module"
    P4SEL |= BIT2;
    P4REN &= ~BIT1; //Resistores externos.
    P4REN &= ~BIT2;


    UCB1CTL0 = UCMST |           //Master Mode
                           UCMODE_3 |    //I2C Mode
                           UCSYNC;         //Synchronous Mode

    UCB1CTL1 = UCSSEL__ACLK |    //Clock Source: ACLK
                           UCSWRST ;             //Mantém o módulo desligado

    //Divisor de clock para o BAUDRate
    UCB1BR0 = 2;
    UCB1BR1 = 0;



    //Prepara minhas variáveis.
    byteReceived = 0;
    rxFlag = 0;

    //Liga o módulo.
    UCB1CTL1 &= ~UCSWRST;

    //Liga a interrupção de RX.
    UCB1IE = UCNACKIE | UCRXIE;

    _enable_interrupt();
}

void master_I2C_UCB1_ReceiveOneByte(unsigned char address)
{
    byteReceived = 0;
    rxFlag = 0;

    //Coloco o slave address
    UCB1I2CSA = address;

    //Espero a linha estar desocupada.
    while (UCB1STAT & UCBBUSY);

    //Peço um START
    UCB1CTL1 |= UCTXSTT;

    //A ação agora acontece na interrupção!

    return;
}

#pragma vector = USCI_B1_VECTOR;
__interrupt void i2c_isr()
{
    switch (__even_in_range(UCB1IV,12)) {
    case USCI_NONE:
        break;
    case USCI_I2C_UCALIFG:
        break;
    case USCI_I2C_UCNACKIFG:
        rxFlag = 2; //ERRO
        break;
    case USCI_I2C_UCSTTIFG:
        break;
    case USCI_I2C_UCSTPIFG:
        break;
    case USCI_I2C_UCRXIFG:
        byteReceived = (unsigned char) UCB1RXBUF;
        rxFlag = 1; // SUCESSO.
        break;
    case USCI_I2C_UCTXIFG:
        break;

    default:
        break;
    }
}
