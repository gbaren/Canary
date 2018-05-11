
#define F_CPU 1000000UL

#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>

#include "canary.h"


// hd_led_changed will be true when we have detected a change in the
// HD LED signal from the mobo since last time we have used that
// information and reset it to false. The mobo is healthy when the
// HD LED is changing. 
bool hd_led_changed = false;


void delay_ms(unsigned long ms)
{
	while(ms--)
	_delay_ms(1);
}


void tester_flash(int times, int howlong) {
	
	for(int i=0; i<times; i++) {
		mobo_reset_on();
		delay_ms(howlong);
		mobo_reset_off();
		delay_ms(FLASH_DELAY_SHORT_MS);
	}
}


ISR(PCINT0_vect) {
	clrbit(PCMSK, PCINT4);
	hd_led_changed = true;
    #ifdef TESTER
        tester_flash(1,FLASH_DELAY_SHORT_MS);
    #endif
}


ISR(WDT_vect) {
	setbit(WDTCR, WDIE);	// this keeps us from resetting the micro
    #ifdef TESTER
        tester_flash(1,FLASH_DELAY_LONG_MS);
    #endif
	if (hd_led_changed) {
        #ifdef TESTER
            tester_flash(1,FLASH_DELAY_SHORT_MS);
            tester_flash(1,FLASH_DELAY_SHORT_MS);
        #endif
		hd_led_changed = false;
		setbit(PCMSK, PCINT4);
	}
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
	
	#ifdef SIM
		volatile unsigned long prescaler_freq_ms;
		volatile unsigned char timeout;
		prescaler_freq_ms = 32;
		idle_multiplier = 5;
		timeout = WDT_TIMEOUT_32MS;
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

    setup_wdtcr();          // setup watchdog mechanism

	setbit(GIMSK, PCIE);	// enable pin change interrupts
	setbit(PCMSK, PCINT4);	// setup to interrupt on pin change of PB4 (HD LED from mobo)

    #ifdef TESTER
        tester_flash(1,FLASH_DELAY_SHORT_MS);
    #endif
	
    while (1) 
    {
		go_to_sleep();
		main_loop_counter++;
	}
}