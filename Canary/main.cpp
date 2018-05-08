//#define SIM 

#define F_CPU 1000000UL

#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>

#include "canary.h"

#ifdef SIM
	volatile bool hd_led_changed = true;			// LED changed flag
	volatile float prescaler_freq_ms;				// prescaler configuration for WDT frequency in milliseconds
	volatile float configured_delay;				// delay from switches scaled appropriately
	volatile unsigned long wdt_counter = 0;			// counts number of times WDT got hit
	volatile unsigned long main_loop_counter = 0;	// counts main loop iterations for testing
	volatile unsigned int idle_multiplier = 60000;  // minutes for board shorter for sim
#else
	bool hd_led_changed = true;
	float prescaler_freq_ms;
	float configured_delay;
	unsigned long wdt_counter = 0;
	unsigned long main_loop_counter = 0;
	unsigned int idle_multiplier = 60000;
#endif

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


bool is_led_changed() {
#ifdef SIM
	volatile bool is_changed = readbit(GIFR,PCIF);
#else
	bool is_changed = readbit(GIFR,PCIF);
#endif

	if (is_changed) { setbit(GIFR,PCIF); }
	
	return is_changed;
}


ISR(PCINT0_vect) {
	hd_led_changed = true;
}

ISR(WDT_vect) {
	wdt_counter++;
	setbit(WDTCR, WDIE);	// this keeps us from resetting the micro
	tester_flash(1);
}


// the idle time input is on DIP switches #1-3, is logically inverted, and rolled.
unsigned char idletime_input() {
#ifdef SIM
	volatile unsigned char out;
#else
	unsigned char out;
#endif

	out = ~PINB & 0b00000111;
	return (((out & 1) ? 4:0) + (out & 2) + ((out & 4) ? 1:0) + 1);
}


unsigned int setup_wdtcr() {
	//set watchdog timeout to 8 seconds (max on ATtiny)
	//datasheet 8.5.2
	//
	// WDIF - watchdog timeout interrupt flag
	// WDIE - watchdog timeout interrupt enable
	// WDCE - watchdog change enable
	// WDE  - watchdog enable
	// WDP3:WDP0 - timer prescaler oscillator cycles select
	
	//volatile unsigned char WDTCR_mask = bitval(WDIF) | bitval(WDCE);
	//volatile unsigned char WDTCR_new = (WDTCR & WDTCR_mask) | (~WDTCR_mask | bitval(WDE) | bitval(WDIE));
		
	#ifdef SIM
		volatile unsigned long prescaler_freq_ms;
		volatile unsigned char timeout;
		prescaler_freq_ms = 32;
		idle_multiplier = 5;
		timeout = WDT_TIMEOUT_32MS
	#else
		unsigned long prescaler_freq_ms;
		unsigned char timeout;
		prescaler_freq_ms = 8000;
		idle_multiplier = 60000;
		timeout = WDT_TIMEOUT_8S;
	#endif

	WDTCR |= bitval(WDE) | bitval(WDIE) | timeout;
	
	return prescaler_freq_ms;

}

void go_to_sleep() {
	MCUCR |= SLEEP_MODE_PWR_DOWN;	// set sleep mode
	setbit(MCUCR,SE);				// sleep enable bit
	sei();							// enable interrupts
	sleep_cpu();					// sleep
	clrbit(MCUCR,SE);				// sleep disable
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

	clrbit(ADCSRA,ADEN);	// disable ADC (default is enabled in all sleep modes)
	
	prescaler_freq_ms = setup_wdtcr();

	configured_delay = (idletime_input() * idle_multiplier) / prescaler_freq_ms;
	
	// we don't want to interrupt on a pin change, only check the PCIF when we
	// come out of sleep from a watchdog timeout
	//setbit(GIMSK, PCIE);	// enable pin change interrupts
	setbit(PCMSK, PCINT4);	// setup to interrupt on pin change of PB4
	
    while (1) 
    {
		go_to_sleep();
		main_loop_counter++;
		if (is_led_changed()) {
			wdt_counter = 0;
			tester_flash(1);
		} else if (configured_delay < wdt_counter) {
			wdt_counter = 0;
			tester_flash(2);
		}
	}
}