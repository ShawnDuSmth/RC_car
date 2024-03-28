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

volatile int ISR_PB0=150, ISR_PD7=150, ISR_PD6=150, ISR_PD5=150, ISR_cnt=0;

// 'Timer 0 output compare A' Interrupt Service Routine
// This ISR happens at a rate of 100kHz.  It is used
// to generate two standard hobby servo 50Hz signal with
// a pulse width of 0.6ms to 2.4ms.
// ISR(TIMER0_COMPA_vect)
// {
// 	OCR0A = OCR0A + OCR0_RELOAD;
// 	ISR_cnt++;
// 	if(ISR_cnt==ISR_PB0)
// 	{
// 		PORTB |= (1<<0); // PB0=1
// 	}
// 	if(ISR_cnt==ISR_PD7)
// 	{
// 		PORTD |= (1<<7); // PD7=1
// 	}
// 	if(ISR_cnt==ISR_PD6)
// 	{
// 		PORTD |= (1<<6); // PD6=1
// 	}
// 	if(ISR_cnt==ISR_PD5)
// 	{
// 		PORTD |= (1<<5); // PD5=1
// 	}

// 	if(ISR_cnt>=2000)
// 	{
// 		ISR_cnt=0; // 2000 * 10us=20ms
// 		PORTB &= ~(1<<0); // PB0=0
// 		PORTD &= ~(1<<7); // PD7=0
// 		PORTD &= ~(1<<6); // PD6=0
// 		PORTD &= ~(1<<5); // PD5=0
// 	}
// }
//************************************************************************************************
//timer initializations 
// void timer_init0 (void)
// {
//     cli();// disable global interupt
//     TCCR0A = 0;// set entire TCCR1A register to 0
//     TCCR0B = 0;// same for TCCR1B
//     TCNT0  = 0;//initialize counter value to 0
//     // set compare match register for 100khz increments
//     OCR0A = OCR0_RELOAD;// = (16*10^6) / (1*100000) - 1 (must be <255)   
//     TCCR0B |= (1 << WGM12); // turn on CTC mode   
//     TCCR0B |= (1 << CS10); // Set CS10 bits for 1 prescaler  
//     TIMSK0 |= (1 << OCIE0A); // enable timer compare interrupt    
//     sei(); // enable global interupt
// }

// void timer_init1 (void)
// {
// 	// Turn on timer with no prescaler on the clock.  We use it for delays and to measure period.
// 	TCCR1B |= _BV(CS10); // Check page 110 of ATmega328P datasheet
// }
//************************************************************************************************
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

// void PrintNumber(long int N, int Base, int digits)
// { 
// 	char HexDigit[]="0123456789ABCDEF";
// 	int j;
// 	#define NBITS 32
// 	char buff[NBITS+1];
// 	buff[NBITS]=0;

// 	j=NBITS-1;
// 	while ( (N>0) | (digits>0) )
// 	{
// 		buff[j--]=HexDigit[N%Base];
// 		N/=Base;
// 		if(digits!=0) digits--;
// 	}
// 	usart_pstr(&buff[j+1]);
// }
/*
void ConfigurePins (void)
{
	//PWM Configuration
	//D port configuration
	DDRD |= (1 << DDD5); // PD5 is now output
	DDRD |= (1 << DDD6); // PD6 is now output
	DDRD |= (1 << DDD7); // PD7 is now output
	PORTD &= 0b00110011; //set PD5,PD6,PD6 to low
	//B port configuration
	DDRB  |= 0b00000001; // PB0 configured as output
	PORTB &= 0x11111110; // PB0 = 0

	//other Configurations

	// CHANGE TO CONFIGURE WHICH PIN WE ARE USING AS INPUT FOR FREQUENCY MEASUREMENT
    DDRC &= ~(1 << PC5); // Configure PC5 as input
    PORTC |= (1 << PC5); // Activate pull-up in PC5

	//Transmitter input
	DDRD  &= ~(BIT3); // PD3 configured as input
	PORTD |= BIT3; // Activate pull-up in PD3

}
*/
void ConfigurePins (void)
{

//PWM Configuration - outputs
//D port 
	DDRD |= 0b11100000; //PD5, PD6, PD5 is now an output
	PORTD &= 0b00011111; //set PD5,PD6,PD6 to low
//B port 
	DDRB  |= 0b00000001; // PB0 configured as output
	PORTB &= 0x11111110; // PB0 = 0

//JDY-40 config - input
//D port 
	DDRD  &= 0b11110111; // PD3 configured as input
//PORTD &= 0b11110111; // this is wrong but frick it we ball
	PORTD |= 0b00001000; // Activate pull-up in PD3

//frequency detection 
//C port - inpute
	DDRC &= 0b11011111; // Configure PC5 as input
    PORTC |= 0b00100000; // Activate pull-up in PC5

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

// Code for Period measurement from the oscilator circuit:
//************************************************************************************************
unsigned int cnt = 0;
#define PIN_PERIOD (PINB & 0b00000010)

// GetPeriod() seems to work fine for frequencies between 30Hz and 300kHz.
// long int GetPeriod (int n)
// {
// 	int i, overflow;
// 	unsigned int saved_TCNT1a, saved_TCNT1b;
	
// 	overflow=0;
// 	TIFR1=1; // TOV1 can be cleared by writing a logic one to its bit location.  Check ATmega328P datasheet page 113.
// 	while (PIN_PERIOD!=0) // Wait for square wave to be 0
// 	{
// 		if(TIFR1&1)	{ TIFR1=1; overflow++; if(overflow>5) return 0;}
// 	}
// 	overflow=0;
// 	TIFR1=1;
// 	while (PIN_PERIOD==0) // Wait for square wave to be 1
// 	{
// 		if(TIFR1&1)	{ TIFR1=1; overflow++; if(overflow>5) return 0;}
// 	}
	
// 	overflow=0;
// 	TIFR1=1;
// 	saved_TCNT1a=TCNT1;
// 	for(i=0; i<n; i++) // Measure the time of 'n' periods
// 	{
// 		while (PIN_PERIOD!=0) // Wait for square wave to be 0
// 		{
// 			if(TIFR1&1)	{ TIFR1=1; overflow++; if(overflow>1024) return 0;}
// 		}
// 		while (PIN_PERIOD==0) // Wait for square wave to be 1
// 		{
// 			if(TIFR1&1)	{ TIFR1=1; overflow++; if(overflow>1024) return 0;}
// 		}
// 	}
// 	saved_TCNT1b=TCNT1;
// 	if(saved_TCNT1b<saved_TCNT1a) overflow--; // Added an extra overflow.  Get rid of it.

// 	return overflow*0x10000L+(saved_TCNT1b-saved_TCNT1a);
// }
//************************************************************************************	
// void adc_init(void)
// {
//     ADMUX = (1<<REFS0);
//     ADCSRA = (1<<ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);
// }

// uint16_t adc_read(int channel)
// {
//     channel &= 0x7;
//     ADMUX = (ADMUX & 0xf8)|channel;
     
//     ADCSRA |= (1<<ADSC);
     
//     while(ADCSRA & (1<<ADSC)); //as long as ADSC pin is 1 just wait.
     
//     return (ADCW);
// }


void main (void)
{
	char buff[80];
	char *ptr1;
	char *ptr2;
	long int count;
	float T, f;

	int i, j;
	int bufflen=0;
	unsigned char k=0;
	char sXAngle[6] = "";
	char sYAngle[6] = "";

	double iXAngle = 2.5;
	double iYAngle = 2.5;
	double PWM_Duty_Cycle;
	double L_PWM_Duty_Cycle;
	double R_PWM_Duty_Cycle;

	int Left_forward = 0;
	int Right_forward = 0;

	char original[] = "This is a sample string";
	char extracted[10]; // Assuming you want to extract 10 characters
	int start_index = 5; // Starting index of extraction
	int length = 6; // Number of characters to extract
	
	ConfigurePins();
	//timer_init0();
	//timer_init1();

	usart_init();   // configure the hardware usart and baudrate
	Init_Software_Uart(); // Configure the sorftware UART
	_delay_ms(500); // Give putty a chance to start before we send information...
	printf("\r\nJDY-40 test\r\n");
	
	// We should select an unique device ID.  The device ID can be a hex
	// number from 0x0000 to 0xFFFF.  In this case is set to 0xBBDB
	SendATCommand("AT+DVIDFBCD\r\n");  

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
	//DDRD  &= ~(BIT3); // PD3 configured as input
	//DDRB  |= (1 << DDB0); // PB0 configured as output
	//DDRD  |= (1 << DDD7); // PD[7..2] configured as outputs
	//DDRD |= (1 << DDD5)|(1 << DDD6);; // PD6 is now an output
    //OCR0A = 128; // set PWM for 50% duty cycle
    //TCCR0A |= (1 << COM0A1); // set noset fast PWM Mode
    //TCCR0B |= (1 << CS00); // set prescaler to nne-inverting mode
    //TCCR0A |= (1 << WGM01) | (1 << WGM00); // one and starts PWM


	// Turn on timer with no prescaler on the clock.  We use it for delays and to measure period.
	//TCCR1B |= _BV(CS10); // Check page 110 of ATmega328P datasheet


	// Now toggle the pins on/off to see if they are working.
	// First turn all off:
	//PORTD &= ~(1<<2); // PD2=0
	//PORTD &= ~(1<<3); // PD3=0
	//PORTD &= ~(1<<4); // PD4=0
	//PORTD &= ~(1<<5); // PD5=0
	//PORTD &= ~(1<<6); // PD6=0
	// Now turn on one of the outputs per loop cycle to check
	double juicyburger = 69.42;

	while(1)
	{
		if(RXD_FLAG==1) // Something has arrived
		{
			GetString1(buff, 80);
			
			bufflen = strlen(buff);
			if (bufflen == 13){
				//printf("%s\n",buff);
				//printf("%0.4lf\n",juicyburger);
				
				strncpy(sXAngle, buff + 0, 6);
				sXAngle[6] = '\0'; // Null-terminate the extracted string
				strncpy(sYAngle, buff + 7, 13);
				sYAngle[6] = '\0'; // Null-terminate the extracted string

				// for(i = 0; i < 6; i++){
				// 	sXAngle[i] = buff[i];
				// }
				// sXAngle[6] = '\0';
				// for(j = 7; j < 13; j++){
				// 	sYAngle[j-7] = buff[j];
				// }
				// sYAngle[6] = '\0';

				// printf("%s, ", sXAngle);
				// printf("%s\n", sYAngle);

				// iXAngle = strtod(sXAngle,&ptr1);
				// iYAngle = strtod(sYAngle,&ptr2);
				// printf("%.4f, %.4f\n", iXAngle, iYAngle);

				iXAngle = atof(sXAngle);
				iYAngle = atof(sYAngle);
				sprintf(buff, "test %0.4lf, %0.4lf\n", iXAngle, iYAngle);
				printf("%s\n",buff);
				// printf("%.4f, %.4f\n", iXAngle, iYAngle);
			}
			
		}
		
		// scaling the voltage to 5V
		// iXAngle *= 1.5151515;
		// iYAngle *= 1.5151515;
		

		

		// JOYSTICK VOLTAGES
		// 
		//				5 V
		//			0V	 o   5V
		//				0 V
	
		// this code maps the analog voltage from transmitter into PWM duty cycle
		if(iYAngle > 2.6){
			// mapping the forward PWM signal
			PWM_Duty_Cycle = 2000*((iXAngle-2.5)/2.5);
			Left_forward = 1;
			Right_forward = 1;
		}else if(iYAngle < 2.4){
			// mapping the backward PWM signal
			PWM_Duty_Cycle = 2000*(iXAngle/2.5);
			Left_forward = -1;
			Right_forward = -1;
		}else{
			PWM_Duty_Cycle = 0.0;
		}

		// this code maps the rpm difference between motors
		if(iXAngle > 2.6){
			if(iXAngle < 3.75){
				// scale the right motor down
				L_PWM_Duty_Cycle = PWM_Duty_Cycle;
				R_PWM_Duty_Cycle = PWM_Duty_Cycle*(1-(iXAngle-2.5)/1.25);
			}else{
				// scale the right motor up in opposite direction
				L_PWM_Duty_Cycle = PWM_Duty_Cycle;
				R_PWM_Duty_Cycle = PWM_Duty_Cycle*((iXAngle-3.75)/1.25);
				Right_forward *= -1;
			}
		}else if(iXAngle < 2.4){
			if(iXAngle > 1.25){
				// scale the left motor down
				R_PWM_Duty_Cycle = PWM_Duty_Cycle;
				L_PWM_Duty_Cycle = PWM_Duty_Cycle*((iXAngle-1.25)/1.25);
			}else{
				// scale the left motor up in opposite direction
				R_PWM_Duty_Cycle = PWM_Duty_Cycle;
				L_PWM_Duty_Cycle = PWM_Duty_Cycle*(1-(iXAngle/1.25));
				Right_forward *= -1;
			}
		}else{
			// if the joystick is sitting in the middle of X coord
			// copy PWM cycle to both left or right motor
			L_PWM_Duty_Cycle = PWM_Duty_Cycle;
			R_PWM_Duty_Cycle = PWM_Duty_Cycle;
		}

		//printf("%0.4f, %0.4f \r\n", L_PWM_Duty_Cycle, R_PWM_Duty_Cycle);

		// // changing the actual right motor PWM signals
		// if(Right_forward == 1){
		// 	ISR_PB0 = 20001-R_PWM_Duty_Cycle;
		// 	ISR_PD7 = 0.0;
		// }else if(Right_forward == -1){
		// 	ISR_PD7 = 2001-R_PWM_Duty_Cycle;
		// 	ISR_PB0 = 0.0;
		// }

		// // changing the actual left motor PWM signals
		// if(Left_forward == 1){
		// 	ISR_PD5 = 20001-L_PWM_Duty_Cycle;
		// 	ISR_PD6 = 0.0;
		// }else if(Left_forward == -1){
		// 	ISR_PD6 = 2001-L_PWM_Duty_Cycle;
		// 	ISR_PD5 = 0.0;
		// }
		// //printf("pwm duty: %0.4f\r\n",PWM_Duty_Cycle);
		// //printf("%0.4f %0.4f\r\n", L_PWM_Duty_Cycle, R_PWM_Duty_Cycle);

		// count=GetPeriod(100);
		// if(count>0) // Make sure a signal is detected - otherwise, divide my 0 error.
		// {
		// 	T=count/(F_CPU*100.0);
		// 	f=1/T;
		// }

		//_delay_ms(5);

	}
}

