//#define SIM 
//#define TESTER

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
	volatile float idle_multiplier = 60000;			// 1 minute
	volatile float timeout_progress = 0;			// percent of timeout elapsed (count of time slices)
#else
	bool hd_led_changed = true;                              
	float prescaler_freq_ms;
	float configured_delay;
	unsigned long wdt_counter = 0;
	unsigned long main_loop_counter = 0;
	#ifdef TESTER
		float idle_multiplier = 60000;	// 1 min if testing
	#else
		float idle_multiplier = 300000; //5 min
	#endif
	float timeout_progress = 0;
#endif

void delay_ms(unsigned long ms)
{
	while(ms--)
	_delay_ms(1);
}


void tester_flash(int times, int howlong) {
	#ifdef TESTER
		for(int i=0; i<times; i++) {
			mobo_reset_on();
			delay_ms(howlong);
			mobo_reset_off();
			delay_ms(FLASH_DELAY_SHORT_MS);
		}
	#endif
}

void led_off() {
	led_green_off();
	led_red_off();	
}

void led_flash(int color, int times, int delay) {
	led_off();
	for (int i=0; i<times; i++) {
		switch (color) {
			case LED_GREEN :
				led_green_on();
				break;
			case LED_RED :
				led_red_on();
				break;
			case LED_ORANGE :
				led_green_on();
				led_red_on();
		}
		delay_ms(delay);
		led_off();
		delay_ms(delay);
	}
}


void led_startup() {
	led_flash(LED_RED,1,FLASH_DELAY_LONG_MS);
	led_flash(LED_ORANGE,1,FLASH_DELAY_LONG_MS);
	led_flash(LED_GREEN,1,FLASH_DELAY_LONG_MS);
	led_flash(LED_RED,1,FLASH_DELAY_LONG_MS);
	led_flash(LED_ORANGE,1,FLASH_DELAY_LONG_MS);
	led_flash(LED_GREEN,1,FLASH_DELAY_LONG_MS);}

ISR(PCINT0_vect) {
	cli();
	hd_led_changed = true;
	#ifdef TESTER
		led_flash(LED_RED,1,FLASH_DELAY_BLINK_MS);
		tester_flash(1,FLASH_DELAY_SHORT_MS);
	#endif
	sei();
}

ISR(WDT_vect) {
	cli();
	setbit(WDTCR, WDIE);	// this keeps us from resetting the micro

	wdt_counter++;

	if (hd_led_changed) {
		hd_led_changed = false;
		wdt_counter = 0;
		timeout_progress = 0;
		led_flash(LED_GREEN,1,FLASH_DELAY_LONG_MS);
	} else {
		led_flash(LED_ORANGE,(int)timeout_progress,FLASH_DELAY_SHORT_MS);
	}
	sei();
}


// the idle time input is on DIP switches #1-3, is logically inverted, and rolled.
unsigned char idletime_input() {
	#ifdef SIM
		volatile unsigned char out;
	#else
		unsigned char out;
	#endif

	//out = ~PINB & 0b00000100;
	out = (readbit(PINB,PB2)) >> PB2;
	return (!out + 1);
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
		timeout = WDT_TIMEOUT_32MS;
	#else
		unsigned long prescaler_freq_ms;
		unsigned char timeout;
		prescaler_freq_ms = 8000;
		timeout = WDT_TIMEOUT_8S;
	#endif

	WDTCR |= bitval(WDE) | bitval(WDIE) | timeout;
	
	return prescaler_freq_ms;

}

void wait_for_interrupt() {
	MCUCR |= SLEEP_MODE_PWR_DOWN;	// set sleep mode
	setbit(MCUCR,SE);				// sleep enable bit
	sei();							// enable interrupts
	sleep_cpu();					// sleep
	clrbit(MCUCR,SE);				// sleep disable
}

void reset_mobo() {
	cli();
	
	mobo_reset_on();
	delay_ms(POWER_CYCLE_HOLD_MS);
	mobo_reset_off();
	delay_ms(POWER_CYCLE_RELEASE_MS);
	mobo_reset_on();
	delay_ms(POWER_CYCLE_ON_MS);
	mobo_reset_off();
	
	led_flash(LED_RED,5,FLASH_DELAY_SHORT_MS);
	
	sei();
}


int main(void)
{
	// Rev 2 Pinout:
	//
	// PB5 input	DIP switch #4, Reset output mode, Power SW (0) / Reset SW (1)
	// PB4 input	HD LED from mobo
	// PB3 output	Solid-state relay Q1, shorts Power/Reset SW on outputting 0
	// PB2 input	DIP switch/jumper - select 5/10 minute
	// PB1 output	LED red
	// PB0 output	LED green
	
	DDRB =  0b00001011;		// set all ports as input except for PB3,PB1 and PB0
	PORTB = 0b00101111;		// turn ports on for all inputs except PB4 (enabling pull-ups)

	prescaler_freq_ms = setup_wdtcr();
	
	#ifdef SIM
		volatile unsigned char idle_input = idletime_input();
	#else 
		unsigned char idle_input = idletime_input();
	#endif
	
	led_off();
	led_startup();
	
	clrbit(ADCSRA,ADEN);	// disable ADC (default is enabled in all sleep modes)
	setbit(GIMSK, PCIE);	// enable pin change interrupts
	setbit(PCMSK, PCINT4);	// setup to interrupt on pin change of PB4
	
    while (1) 
    {
		cli();
		idle_input = idletime_input(); // can change switches while device running
		configured_delay = ((float)idle_input * idle_multiplier) / prescaler_freq_ms;
		timeout_progress = (wdt_counter / configured_delay) / 0.2; // counts 20% slices

		wait_for_interrupt();
				
		if (wdt_counter	> configured_delay) {
			timeout_progress = 0;
			wdt_counter = 0;
			reset_mobo();
		}
	}
}
