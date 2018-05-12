/*
 * canary.h
 *
 * Created: 5/2/2018 1:59:56 PM
 *  Author: Gary
 */ 


#ifndef CANARY_H_
#define CANARY_H_

#define clrbit(reg,bit)	((reg) &= ~(1 << (bit)))
#define setbit(reg,bit)	((reg) |=  (1 << (bit)))
#define readbit(reg,bit) ((reg & (1 << (bit))) >> (bit))
#define bitval(bit) (1 << (bit))

// The solid-state relay controlling the Power/Reset switch is on PB3 and is active-low
#define mobo_reset_on()		(clrbit(PORTB,PB3))
#define mobo_reset_off()	(setbit(PORTB,PB3))

#define WDT_TIMEOUT_16MS	0
#define WDT_TIMEOUT_32MS	bitval(WDP0)
#define WDT_TIMEOUT_64MS	bitval(WDP1)
#define WDT_TIMEOUT_128MS	bitval(WDP1) | bitval(WDP0)
#define WDT_TIMEOUT_256MS	bitval(WDP2)
#define WDT_TIMEOUT_512MS	bitval(WDP0) | bitval(WDP0)
#define WDT_TIMEOUT_1S		bitval(WDP2) | bitval(WDP1)
#define WDT_TIMEOUT_2S		bitval(WDP2) | bitval(WDP1) | bitval(WDP0)
#define WDT_TIMEOUT_4S		bitval(WDP3)
#define WDT_TIMEOUT_8S		bitval(WDP3) | bitval(WDP0)

#define POWER_CYCLE_HOLD_MS		5000
#define POWER_CYCLE_RELEASE_MS	5000
#define POWER_CYCLE_ON_MS		1000

#define FLASH_DELAY_LONG_MS		500
#define FLASH_DELAY_SHORT_MS	200


#endif /* CANARY_H_ */