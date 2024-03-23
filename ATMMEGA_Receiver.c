// #include <avr/io.h>
// #include <stdio.h>
// #include "usart.h"

// #include <avr/io.h>
// #include <util/delay.h>

// /* Pinout for DIP28 ATMega328P:

//                            -------
//      (PCINT14/RESET) PC6 -|1    28|- PC5 (ADC5/SCL/PCINT13)
//        (PCINT16/RXD) PD0 -|2    27|- PC4 (ADC4/SDA/PCINT12)
//        (PCINT17/TXD) PD1 -|3    26|- PC3 (ADC3/PCINT11)
//       (PCINT18/INT0) PD2 -|4    25|- PC2 (ADC2/PCINT10)
//  (PCINT19/OC2B/INT1) PD3 -|5    24|- PC1 (ADC1/PCINT9)
//     (PCINT20/XCK/T0) PD4 -|6    23|- PC0 (ADC0/PCINT8)
//                      VCC -|7    22|- GND
//                      GND -|8    21|- AREF
// (PCINT6/XTAL1/TOSC1) PB6 -|9    20|- AVCC
// (PCINT7/XTAL2/TOSC2) PB7 -|10   19|- PB5 (SCK/PCINT5)
//    (PCINT21/OC0B/T1) PD5 -|11   18|- PB4 (MISO/PCINT4)
//  (PCINT22/OC0A/AIN0) PD6 -|12   17|- PB3 (MOSI/OC2A/PCINT3)
//       (PCINT23/AIN1) PD7 -|13   16|- PB2 (SS/OC1B/PCINT2)
//   (PCINT0/CLKO/ICP1) PB0 -|14   15|- PB1 (OC1A/PCINT1)
//                            -------
// */


// int main( void )
// {
// 	unsigned char j=0;
	
// 	usart_init (); // configure the usart and baudrate

// 	_delay_ms(500); // Give putty some time to start.
// 	printf("Pwm output test.  Check PD6 (pin 12 of DIP28)\n");
	
//     DDRD |= (1 << DDD5)|(1 << DDD6);; // PD6 is now an output
//     OCR0A = 128; // set PWM for 50% duty cycle
//     TCCR0A |= (1 << COM0A1); // set none-inverting mode
//     TCCR0A |= (1 << WGM01) | (1 << WGM00); // set fast PWM Mode
//     TCCR0B |= (1 << CS00); // set prescaler to none and starts PWM

// 	while(1)
// 	{
// 		OCR0A=j;
// 		j++;
// 		_delay_ms(10);
// 	}
// }

#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdlib.h>
#include <util/delay.h>
#include "usart.h"
#include "Software_UART.h"

/* Pinout for DIP28 ATMega328P:

                           -------
     (PCINT14/RESET) PC6 -|1    28|- PC5 (ADC5/SCL/PCINT13)
       (PCINT16/RXD) PD0 -|2    27|- PC4 (ADC4/SDA/PCINT12)
       (PCINT17/TXD) PD1 -|3    26|- PC3 (ADC3/PCINT11)
      (PCINT18/INT0) PD2 -|4    25|- PC2 (ADC2/PCINT10)
 (PCINT19/OC2B/INT1) PD3 -|5    24|- PC1 (ADC1/PCINT9)
    (PCINT20/XCK/T0) PD4 -|6    23|- PC0 (ADC0/PCINT8)
                     VCC -|7    22|- GND
                     GND -|8    21|- AREF
(PCINT6/XTAL1/TOSC1) PB6 -|9    20|- AVCC
(PCINT7/XTAL2/TOSC2) PB7 -|10   19|- PB5 (SCK/PCINT5)
   (PCINT21/OC0B/T1) PD5 -|11   18|- PB4 (MISO/PCINT4)
 (PCINT22/OC0A/AIN0) PD6 -|12   17|- PB3 (MOSI/OC2A/PCINT3)
      (PCINT23/AIN1) PD7 -|13   16|- PB2 (SS/OC1B/PCINT2)
  (PCINT0/CLKO/ICP1) PB0 -|14   15|- PB1 (OC1A/PCINT1)
                           -------
*/

#define ISR_FREQ 100000L // Interrupt service routine tick is 10 us
#define OCR0_RELOAD ((F_CPU/ISR_FREQ)-1)

volatile int ISR_pwm1=150, ISR_pwm2=150, ISR_cnt=0;

// 'Timer 0 output compare A' Interrupt Service Routine
// This ISR happens at a rate of 100kHz.  It is used
// to generate two standard hobby servo 50Hz signal with
// a pulse width of 0.6ms to 2.4ms.
ISR(TIMER0_COMPA_vect)
{
	OCR0A = OCR0A + OCR0_RELOAD;
	ISR_cnt++;
	if(ISR_cnt==ISR_pwm1)
	{
		PORTD &= ~(1<<7); // PD7=0
	}
	if(ISR_cnt==ISR_pwm2)
	{
		PORTB &= ~(1<<0); // PB0=0
	}
	if(ISR_cnt>=2000)
	{
		ISR_cnt=0; // 2000 * 10us=20ms
		PORTD |= (1<<7); // PD7=1
		PORTB |= (1<<0); // PB0=1
	}
}

void timer_init0 (void)
{
    cli();// disable global interupt
    TCCR0A = 0;// set entire TCCR1A register to 0
    TCCR0B = 0;// same for TCCR1B
    TCNT0  = 0;//initialize counter value to 0
    // set compare match register for 100khz increments
    OCR0A = OCR0_RELOAD;// = (16*10^6) / (1*100000) - 1 (must be <255)   
    TCCR0B |= (1 << WGM12); // turn on CTC mode   
    TCCR0B |= (1 << CS10); // Set CS10 bits for 1 prescaler  
    TIMSK0 |= (1 << OCIE0A); // enable timer compare interrupt    
    sei(); // enable global interupt
}

void timer_init1 (void)
{
	// Turn on timer with no prescaler on the clock.  We use it for delays and to measure period.
	TCCR1B |= _BV(CS10); // Check page 110 of ATmega328P datasheet
}

void wait_1ms(void)
{
	unsigned int saved_TCNT1;
	
	saved_TCNT1=TCNT1;
	
	while((TCNT1-saved_TCNT1)<(F_CPU/1000L)); // Wait for 1 ms to pass
}

void waitms(int ms)
{
	while(ms--) wait_1ms();
}

#define PIN_PERIOD (PINB & (1<<1)) // PB1

// GetPeriod() seems to work fine for frequencies between 30Hz and 300kHz.
long int GetPeriod (int n)
{
	int i, overflow;
	unsigned int saved_TCNT1a, saved_TCNT1b;
	
	overflow=0;
	TIFR1=1; // TOV1 can be cleared by writing a logic one to its bit location.  Check ATmega328P datasheet page 113.
	while (PIN_PERIOD!=0) // Wait for square wave to be 0
	{
		if(TIFR1&1)	{ TIFR1=1; overflow++; if(overflow>5) return 0;}
	}
	overflow=0;
	TIFR1=1;
	while (PIN_PERIOD==0) // Wait for square wave to be 1
	{
		if(TIFR1&1)	{ TIFR1=1; overflow++; if(overflow>5) return 0;}
	}
	
	overflow=0;
	TIFR1=1;
	saved_TCNT1a=TCNT1;
	for(i=0; i<n; i++) // Measure the time of 'n' periods
	{
		while (PIN_PERIOD!=0) // Wait for square wave to be 0
		{
			if(TIFR1&1)	{ TIFR1=1; overflow++; if(overflow>1024) return 0;}
		}
		while (PIN_PERIOD==0) // Wait for square wave to be 1
		{
			if(TIFR1&1)	{ TIFR1=1; overflow++; if(overflow>1024) return 0;}
		}
	}
	saved_TCNT1b=TCNT1;
	if(saved_TCNT1b<saved_TCNT1a) overflow--; // Added an extra overflow.  Get rid of it.

	return overflow*0x10000L+(saved_TCNT1b-saved_TCNT1a);
}

void adc_init(void)
{
    ADMUX = (1<<REFS0);
    ADCSRA = (1<<ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);
}

uint16_t adc_read(int channel)
{
    channel &= 0x7;
    ADMUX = (ADMUX & 0xf8)|channel;
     
    ADCSRA |= (1<<ADSC);
     
    while(ADCSRA & (1<<ADSC)); //as long as ADSC pin is 1 just wait.
     
    return (ADCW);
}

void PrintNumber(long int N, int Base, int digits)
{ 
	char HexDigit[]="0123456789ABCDEF";
	int j;
	#define NBITS 32
	char buff[NBITS+1];
	buff[NBITS]=0;

	j=NBITS-1;
	while ( (N>0) | (digits>0) )
	{
		buff[j--]=HexDigit[N%Base];
		N/=Base;
		if(digits!=0) digits--;
	}
	usart_pstr(&buff[j+1]);
}

void ConfigurePins (void)
{
	DDRB  &= 0b11111101; // Configure PB1 as input
	PORTB |= 0b00000010; // Activate pull-up in PB1
	
	DDRD  |= 0b11111100; // PD[7..2] configured as outputs
	PORTD &= 0b00000011; // PD[7..2] = 0
	
	DDRB  |= 0b00000001; // PB0 configured as output
	PORTB &= 0x11111110; // PB0 = 0
}

void SendATCommand (char * s)
{
	char buff[40];
	printf("Command: %s", s);
	PORTD &= ~(BIT4); // 'set' pin to 0 is 'AT' mode.
	_delay_ms(10);
	SendString1(s);
	GetString1(buff, 40);
	PORTD |= BIT4; // 'set' pin to 1 is normal operation mode.
	_delay_ms(10);
	printf("Response: %s\r\n", buff);
}

void main (void)
{
	char buff[80];
	int cnt=0;

	int i, j;
	unsigned char k=0;
	char sXAngle[5];
	char sYAngle[5];

	float iXAngle;
	float iYAngle;
	
	usart_init();   // configure the hardware usart and baudrate
	Init_Software_Uart(); // Configure the sorftware UART
	_delay_ms(500); // Give putty a chance to start before we send information...
	printf("\r\nJDY-40 test\r\n");
	
	// We should select an unique device ID.  The device ID can be a hex
	// number from 0x0000 to 0xFFFF.  In this case is set to 0xABBA
	SendATCommand("AT+DVIDBBDB\r\n");  

	// To check configuration
	SendATCommand("AT+VER\r\n");
	SendATCommand("AT+BAUD\r\n");
	SendATCommand("AT+RFID\r\n");
	SendATCommand("AT+DVID\r\n");
	SendATCommand("AT+RFC\r\n");
	SendATCommand("AT+POWE\r\n");
	SendATCommand("AT+CLSS\r\n");

	// Configure PD3 for input.  Information here:
	// http://www.elecrom.com/avr-tutorial-2-avr-input-output/
	DDRD  &= ~(BIT3); // PD3 configured as input
	PORTD |= BIT3; // Activate pull-up in PD3

	DDRD |= (1 << DDD5)|(1 << DDD6);; // PD6 is now an output
    OCR0A = 128; // set PWM for 50% duty cycle
    TCCR0A |= (1 << COM0A1); // set none-inverting mode
    TCCR0A |= (1 << WGM01) | (1 << WGM00); // set fast PWM Mode
    TCCR0B |= (1 << CS00); // set prescaler to none and starts PWM
	
	printf("\r\nPress and hold a push-button attached to PD3 (pin 5) to transmit.\r\n");
	
	cnt=0;
	while(1)
	{
		if(RXD_FLAG==1) // Something has arrived
		{
			GetString1(buff, 80);

			for(i = 0; i < 5; i++){
				sXAngle[i] = buff[i];
			}
			for(j = 7; j < 11; j++){
				sYAngle[j-7] = buff[j];
			}

			iXAngle = atof(sXAngle);
			iYAngle = atof(sYAngle);
		}

		OCR0A=128;
		//k++;
		_delay_ms(10);

	}
}