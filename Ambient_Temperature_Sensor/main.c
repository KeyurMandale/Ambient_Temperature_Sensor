#include <avr/io.h>
#define F_CPU 1000000UL
#include <avr/interrupt.h>
#include <stdlib.h>
#include "LCDLibrary.h"

char tempresult[6];
uint16_t Vout=0;
uint16_t Vgnd=0;
int Vdiff=0;

int main(void)
{
	InitializeLCD();
	
	ADMUX  |= 1<<ADLAR; //Left Shift ADC conversion result i.e. ADCH will hold 8 most significant bits while ADCL will hold two least significant bits
	ADMUX  |= 1<<REFS0; // Using AVCC as Reference voltage for conversion
	ADCSRA |= 1<<ADPS2; // Setting Prescaler to 16. ADC runs at a much slower speed compared to the uC. The prescalar affects the resolution of the ADC as it determines the clock frequency of the ADC
	ADCSRA |= 1<<ADIE; // Enabling ADC Interrupt
	sei();			   // Enabling global interrupts
	ADCSRA |= 1<<ADEN; // Enabling ADC
	
	ADCSRA |= 1<<ADSC; // Start conversion

	while (1)
	{
	}
}

ISR(ADC_vect)
{
	uint8_t LowerBits = ADCL;							// Store the 2 least significant bits in LowerBits
	uint16_t TenBitResult = ADCH << 2 | LowerBits >> 6; // Left shifted ADCH by 2 bits ORed with Right shifted LowerBits(ADCL) by 6 bits

	/*
		To measure negative temperatures/volatges using the voltage shifting circuit given in LM35 datasheet two ADC channels are used 
		Channel 0 is the voltage at pin 2 of LM35
		Channel 1 is the voltage at pin 3 of LM35
		The output voltage is the difference between the voltage at output pin and ground pin of LM35  
	*/
	switch(ADMUX)
	{
		case 0x60:							    // Selecting channel 0 of ADC. Value depends on the setup of ADMUX
					Vout = TenBitResult;		// Output voltage at pin 2 of LM35
					ADMUX = 0x61;				// Select channel 1 of ADC for next conversion
					break;
		case 0x61:								// Selecting channel 1 of ADC. Value depends on the setup of ADMUX
					Vgnd = TenBitResult;		// Voltage at pin 3 of LM35
					ADMUX = 0x60;				// Select channel 0 of ADC for next conversion
					break;
		default:
					break;
	}
	
	Vdiff = Vout -Vgnd;			// Calculating the difference between pin 2 and 3 of LM35	
	
	float result;				
	result = Vdiff * 0.488; /* 
							  10bit resolution corresponds to 1024 different values for input voltage range
							  Since AVCC is being used as reference voltage the value of each division = 5V/1024
							  The sensitivity of LM35 is 10mV/C hence the actual voltage reading would = (5V/1024)/0.010 = 0.487
						   */

	SendCommand(0x80);                  // Go to first position on LCD
	Send_a_String("Temp :");
	dtostrf(result,6,2,tempresult);		// Convert the float to string value to display on LCD
	Send_a_String(tempresult);
	SendData(0xDF);						// Send degree symbol
	SendData(0x43);						// Send letter "C"
	
	ADCSRA |= 1 << ADSC;				// Start next conversion
}