/*

#define F_CPU 1000000UL

#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <util/delay.h>


#define clrbit(reg,bit)	((reg) &= ~(1 << (bit)))
#define setbit(reg,bit)	((reg) |=  (1 << (bit)))
#define isbit(reg,bit) ((reg) & (1 << (bit)))

#define bitval(bit) (1 << (bit))



volatile unsigned char counter_8s = 0;

ISR(WDT_vect) {
	counter_8s++;
	setbit(WDTCR, WDIE);	// this keeps us from resetting the micro
}


volatile unsigned int loop_count = 0;

int main(void)
{
	WDTCR |= bitval(WDE) | bitval(WDIE) | bitval(WDP0);
	sei();					// enable interrupts


	while(1) {
		loop_count++;
		
	}
}


*/
