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

// The solid-state relay controlling the Power/Reset switch is on PB3 and is active-low
#define mobo_reset_on()		(clrbit(PORTB,PB3))
#define mobo_reset_off()	(setbit(PORTB,PB3))

#define FLASH_DELAY_MS 200


#endif /* CANARY_H_ */