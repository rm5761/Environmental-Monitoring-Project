#include <msp430g2553.h>
#include "TI_USCI_I2C_master.h"
#include "ECS_header.h"

/**
 * main.c
 */

volatile unsigned char mode;
unsigned char VEML_setup[3] = {0x00, 0x00, 0x00};

unsigned char SHTdata[6];
unsigned char VEMLdata[2];
unsigned char command;

volatile unsigned int temp;
volatile unsigned int humidity;
volatile unsigned int lux;


volatile unsigned char comp;
volatile unsigned int threslow;
volatile unsigned int threshi;

volatile unsigned char SWdelay;
volatile unsigned char timecount = 0;

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer

    P2DIR |= 0Xff;
    P2OUT &= ~(0xff);

    P1DIR |= 0x38;
    P1OUT &= ~(0x38);
    setup();
    while(1){
        modeSel();
        reqData();
        calculate();
        display();
        SWdelay = 40;
        SWtimer();
    }



    return 0;
}

void setup(){
    UCB0STAT &= ~UCBBUSY;
    //I2C Pin config
    P1SEL |= 0xc0;
    P1SEL2 |= 0xc0;

    //mode setup
    //mode = 2;

    _EINT();
    //If light sensor needs initial conditions, put them here
    //100 ms at gain 1


    TI_USCI_I2C_transmitinit(0x10,0x04);
    while(TI_USCI_I2C_notready());

    TI_USCI_I2C_transmit(3, VEML_setup);
    while(TI_USCI_I2C_notready());


    mode = 0;
    SWdelay = 3;
    SWtimer();

}

void modeSel(){
    if(mode == 0){
        P2OUT = 0x01;
        _EINT();
    }
    else if(mode == 1){
        P2OUT = 0x02;
        _EINT();
    }
    else if(mode == 2){
        P2OUT = 0x04;
        _EINT();
    }
}

void reqData(){
    _EINT();

    if(mode == 0 || mode == 1){
        command = 0xFD;
        TI_USCI_I2C_transmitinit(0x44,0x04);
        while(TI_USCI_I2C_notready());

        TI_USCI_I2C_transmit(1,&command);
        while(TI_USCI_I2C_notready());

        SWdelay = 0x02;
        SWtimer();


        TI_USCI_I2C_receiveinit(0x44,0x04);
        TI_USCI_I2C_receive(6,SHTdata);
        while(TI_USCI_I2C_notready());
    }
    else if (mode == 2){

        command = 0x04;
        TI_USCI_VEML_SETUP(0x10, 0x04, &command, VEMLdata);

    }

}



void calculate(){
    if(mode == 0){
        temp = SHTdata[0]*256 + SHTdata[1];
        comp = ((temp/65535.0)*175)-45;
        threslow = 20;
        threshi = 24;
    }
    else if(mode == 1){
        humidity = SHTdata[3] * 256 + SHTdata[4];
        comp = ((humidity/65535.0)*125)-6;
        threslow = 35;
        threshi  = 45;
    }
    else if(mode == 2){
        lux = VEMLdata[1]*256 + VEMLdata[0];
        comp = (int)(lux * 0.0672);
        threslow = 50;
        threshi = 150;
    }
}

void display(){
    if(comp >= threshi){
        P1OUT &= ~(0x38);
        P1OUT |= 0x08;
    }
    else if(comp <= threslow){
        P1OUT &= ~(0x38);
        P1OUT |= 0x20;
    }
    else if( (comp > threslow) && (comp < threshi) ){
        P1OUT &= ~(0x38);
        P1OUT |= 0x10;
    }


    mode++;
    if(mode > 2){
        mode = 0;
    }
}

void SWtimer(void){

    timecount = 0;

    TACCR0 = 50000;
    TACTL = TASSEL_2 + ID_0 + TACLR;
    TACCTL0 = CCIE;

    TACTL |= MC_1;


    while(timecount < SWdelay) {

    }
}
#pragma vector = TIMER0_A0_VECTOR
__interrupt void Timer_A0_ISR(void) {
    timecount++;
    if (timecount >= SWdelay) {
        TACTL &= ~MC_1;
    }
}
