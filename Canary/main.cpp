/*
 * canary.cpp
 *
 * Created: 4/20/2018 12:22:39 AM
 * Author : CDM
 */ 


#define F_CPU 1000000UL


#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <util/delay.h>


#define clrbit(reg,bit)	((reg) &= ~(1 << (bit)))
#define setbit(reg,bit)	((reg) |=  (1 << (bit)))
#define isbit(reg,bit) ((reg) & (1 << (bit)))


// The solid-state relay controlling the Power/Reset switch is on PB3 and is active-low
#define mobo_reset_on()		(clrbit(PORTB,PB3))
#define mobo_reset_off()	(setbit(PORTB,PB3))


bool hd_led_changed = true;


ISR(PCINT0_vect) {
	hd_led_changed = true;
}

ISR(WDT_vect) {
	
}


// the idletime input is on DIP switches #1-3, is logically inverted, and rolled.
unsigned char idletime_input() {
	unsigned char out;
	out = ~PINB & 0b00000111;
	return ((out & 1) ? 4:0) + (out & 2) + ((out & 4) ? 1:0);
}


void delay_ms(unsigned long ms)
{
	while(ms--)
		_delay_ms(1);
}


int main(void)
{
	// Pinout:
	//
	// PB5 input	DIP switch #4, Reset output mode, Power SW (0) / Reset SW (1)
	// PB4 input	HD LED from mobo
	// PB3 output	Solid-state relay Q1, shorts Power/Reset SW on outputting 0
	// PB2 input	DIP switch #3, DIP switches 1,2,3 bitmapped to idle LED time
	//					1-8 minutes, time is (1 + DIP setting) in minutes
	//					switch #1 is MSB
	// PB1 input	DIP switch #2
	// PB0 input	DIP switch #1
	
	DDRB =  0b00001000;		// set all ports as input except for PB3
	PORTB = 0b00101111;		// turn ports on for all inputs except PB4 (enabling pull-ups)

	// we don't want to interrupt on a pin change, only check the PCIF when we
	// come out of sleep from a watchdog timeout
	setbit(GIMSK, PCIE);	// enable pin change interrupts
	setbit(PCMSK, PCINT4);	// setup to interrupt on pin change of PB4
	sei();					// enable interrupts
	

    while (1) 
    {
		delay_ms(200);
		
		//if(PINB & (1 << PB4)) {
		if (hd_led_changed) {
			hd_led_changed = false;
			mobo_reset_on();
			delay_ms(200);
			mobo_reset_off();
		}
    }
}
