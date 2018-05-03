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
#include <avr/sleep.h>
#include <util/delay.h>

#include "canary.h"

bool volatile hd_led_changed = true;
unsigned int prescaler_freq_ms;
unsigned int configured_delay;


void delay_ms(unsigned long ms)
{
	while(ms--)
	_delay_ms(1);
}


void tester_flash(int times) {
	
	for(int i=0; i<times; i++) {
		mobo_reset_on();
		delay_ms(FLASH_DELAY_MS);
		mobo_reset_off();
		delay_ms(FLASH_DELAY_MS);
	}
}


ISR(PCINT0_vect) {
	hd_led_changed = true;
}

ISR(WDT_vect) {
	tester_flash(3);
}


// the idle time input is on DIP switches #1-3, is logically inverted, and rolled.
unsigned char idletime_input() {
	unsigned char out;
	out = ~PINB & 0b00000111;
	return ((out & 1) ? 4:0) + (out & 2) + ((out & 4) ? 1:0) + 1;
}


void setup_wdt() {
	//set watchdog timeout to 8 seconds (max on ATtiny)
	//datasheet 8.5.2
	WDTCR =
		(1 << WDIF)			//watchdog timeout interrupt flag
		+ (1 << WDIE)		//watchdog timeout interrupt enable
		+ (0 << WDCE)		//watchdog change enable
		+ (1 << WDE)		//watchdog enable
		+ (0 << WDP3)		//
		+ (1 << WDP2)		//watchdog timer prescale - WDP3:WDP0
		+ (1 << WDP1)		// 0000=16ms, 0001=32ms, 0010=64ms, 0011=0.125s, 0100=0.25s, 
		+ (1 << WDP0);		// 0101=0.5s, 0110=1s, 0111=2s, 1000=4s, 1001=8s
		
	prescaler_freq_ms = 2000;
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
	
	configured_delay = idletime_input();
	
	setup_wdt();

	// we don't want to interrupt on a pin change, only check the PCIF when we
	// come out of sleep from a watchdog timeout
	//setbit(GIMSK, PCIE);	// enable pin change interrupts
	setbit(PCMSK, PCINT4);	// setup to interrupt on pin change of PB4
	sei();					// enable interrupts
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	sleep_enable();
	
    while (1) 
    {
		sleep_cpu();
	}
}