//#define ICE 
//#define TESTER

#define F_CPU 1000000UL

#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>

#include "canary.h"


unsigned char configured_delay;
unsigned int wdt_counter = 0;
unsigned char timeout_progress = 0;
bool hd_led_changed = true;
unsigned int prescaler_freq_ms;
unsigned char current_state = STATE_START;


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
				break;
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
}


ISR(PCINT0_vect) {
	cli();
	hd_led_changed = true;
	#ifdef TESTER
		led_flash(LED_RED,1,FLASH_DELAY_BLINK_MS);
		tester_flash(1,FLASH_DELAY_SHORT_MS);
	#endif
}


ISR(WDT_vect) {
	cli();
	setbit(WDTCR, WDIE);	// this keeps us from resetting the micro
	wdt_counter++;
}


// the idle time input is on DIP switches #1-3, is logically inverted, and rolled.
unsigned char idletime_input() {
	unsigned char out;
	out = (readbit(PINB,PB2));
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
		

	unsigned int prescaler_freq_ms;
	unsigned char timeout;
	prescaler_freq_ms = 8000;
	timeout = WDT_TIMEOUT_8S;

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

void shutdown_mobo() {
	cli();
	
	mobo_reset_on();
	delay_ms(POWER_CYCLE_HOLD_MS);
	mobo_reset_off();
	
	led_flash(LED_RED,3,FLASH_DELAY_SHORT_MS);
	
	sei();
}

void start_mobo() {
	cli();

	mobo_reset_on();
	delay_ms(POWER_CYCLE_ON_MS);
	mobo_reset_off();
	
	led_flash(LED_GREEN,3,FLASH_DELAY_SHORT_MS);
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
	
	clrbit(ADCSRA,ADEN);	// disable ADC (default is enabled in all sleep modes)
	setbit(GIMSK, PCIE);	// enable pin change interrupts
	setbit(PCMSK, PCINT4);	// setup to interrupt on pin change of PB4
	
    while (1) 
    {
		cli();
		volatile unsigned char idle_input = idletime_input(); // can change switches while device running
		configured_delay = ((float)idle_input * (float)IDLE_MULTIPLIER) / (float)prescaler_freq_ms;
		timeout_progress = ((float)wdt_counter / (float)configured_delay) / 0.2 + 1; // counts 20% slices

		wait_for_interrupt();
				
		switch (current_state) {
			case STATE_START :
				led_startup();
				current_state = STATE_WAIT;
				break;
			case STATE_WAIT :
				if (hd_led_changed) {
					hd_led_changed = false;
					wdt_counter = 0;
					timeout_progress = 0;
					led_flash(LED_GREEN,1,FLASH_DELAY_BLINK_MS);
					current_state = STATE_WAIT;
				} else if (wdt_counter	> configured_delay) {
					timeout_progress = 0;
					wdt_counter = 0;
					led_flash(LED_RED,3,FLASH_DELAY_LONG_MS);
					shutdown_mobo();
					current_state = STATE_SHUTDOWN;
				} else {
					led_flash(LED_ORANGE,timeout_progress,FLASH_DELAY_SHORT_MS);
					current_state = STATE_WAIT;
				}
				break;
			case STATE_BOOT :
				hd_led_changed = false;
				wdt_counter = 0;
				timeout_progress = 0;
				start_mobo();
				current_state = STATE_WAIT;
				break;
			case STATE_SHUTDOWN :
				if (hd_led_changed) {
					led_flash(LED_GREEN,3,FLASH_DELAY_LONG_MS);
					current_state = STATE_WAIT;
				} else if (wdt_counter > (30000.0 / prescaler_freq_ms)) {
					led_flash(LED_GREEN,3,FLASH_DELAY_SHORT_MS);
					current_state = STATE_BOOT;
				}
				break;
		}
		
	}
}
