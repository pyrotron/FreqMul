/*
 PWM frequency multiplier x128

             100n
      ┌───────┤├───────┐
      │ ┌────────────┐ │
      │ │ 1        8 ├─┴─ VCC
      │ │ 2        7 ├─ PB2 (INT0)  INPUT     
      │ │ 3        6 ├─ PB1 (OC1A)  OUTPUT
 GND ─┴─┤ 4        5 │
        └────────────┘
           ATTiny45

fuses: lfuse=0xe2 hfuse=0xdf
*/
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <stdlib.h>
#include <limits.h>

//#define F_CPU 8000000UL

register uint8_t time_h asm("r4"); // High part of time counter
register uint16_t time_cycle asm("r12"); // Period 
register uint16_t time_on asm("r14"); // H level duration



__attribute__((naked)) int main(void) {

	time_h = 0;
	time_cycle = 0;
	time_on = 0;

	ACSR |= 1<<ACD; // Comparator disable

	// Timer0
	TCCR0A = 0;
	// CK/1
	TCCR0B = 1<<CS00;

	// Timer1
	DDRB |= 1<<PB1; // PWM output
	// CK/2, Clear the OC1A output line
	TCCR1 = 1<<CTC1|1<<PWM1A|2<<COM1A0|2<<CS10;

	TIMSK |= 1<<TOIE0; // Timer0 overflow

	// Ext int 0
	MCUCR |= 1<<ISC00; // Any logical change on INT0 generates an interrupt request
	GIMSK |= 1<<INT0; // External Interrupt Request 0 Enable
	PORTB |= 1<<PB2; // Input

	wdt_enable(WDTO_120MS); // Watchdog on
	sei(); // Interrupts enable

	while (1) { // Do not use flags or registers
		wdt_reset(); // Watchdog reset
	}
}


/* External Interrupt 0 */
ISR(INT0_vect, ISR_NAKED) {

	// F_CPU / Timer1 prescaler / F_PWM_IN / grades / 4
	#define THRESHOLD (F_CPU / 1 / 180 / 100 / 4)

	uint16_t time;
	uint8_t time_l = TCNT0;

	if ((TIFR & 1<<TOV0) && (time_l <= UCHAR_MAX/2)) { // Overflow occured right now
		time_l = UCHAR_MAX; // 0xff
	}
	time = (time_h << 8) + time_l;

	if (PINB & 1<<PB2) { // Risen
		if (abs(time - time_cycle) > THRESHOLD) {
			time_cycle = time;
			OCR1C = time_h;
		}
		TCNT0 = 0;
		time_h = 0;
		if (TIFR & 1<<TOV0) {
			TIFR = 1<<TOV0; // Clear Timer0 overflow flag
		}
	}
	else { // Falled
		if (abs(time - time_on) > THRESHOLD) {
			time_on = time;
			OCR1A = time_h;
		}
	}
	reti(); // Because ISR_NAKED

}

/* Timer/Counter0 Overflow */
ISR(TIM0_OVF_vect, ISR_NAKED) {

	#define TIME_H_LIM (UCHAR_MAX-1)

	if (time_h < TIME_H_LIM) { // Normal way
		time_h += 1;
	}
	else { // High part overflowed
		if (PINB & 1<<PB2) {
			OCR1A = TIME_H_LIM; // Always on
		}
		else {
			OCR1A = 0; // Always off
		}
		OCR1C = TIME_H_LIM;
		time_h = 0;
		time_cycle = 0;
		time_on = 0;
	}
	reti(); // Because ISR_NAKED
}



