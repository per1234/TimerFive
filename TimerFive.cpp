/*
 *  Interrupt and PWM utilities for 16 bit Timer5 on ATmega2560
 *  Modified from TimerOne project http://code.google.com/p/arduino-timerone/
 *
 *  This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 3 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  See GitHub project https://github.com/FireDeveloper/TimerFive for latest
 */
#ifndef TIMERFIVE_cpp
#define TIMERFIVE_cpp

#include "TimerFive.h"

TimerFive Timer5;              // preinstatiate

ISR(TIMER5_OVF_vect){          // interrupt service routine that wraps a user defined function supplied by attachInterrupt
  Timer5.isrCallback();
}


void TimerFive::initialize(long microseconds){
  TCCR5A = 0;                 // clear control register A 
  TCCR5B = _BV(WGM53);        // set mode 8: phase and frequency correct pwm, stop the timer
  setPeriod(microseconds);
}


void TimerFive::setPeriod(long microseconds){

  long cycles = (F_CPU / 2000000) * microseconds;                                // the counter runs backwards after TOP, interrupt is at BOTTOM so divide microseconds by 2
  if(cycles < RESOLUTION)              clockSelectBits = _BV(CS50);              // no prescale, full xtal
  else if((cycles >>= 3) < RESOLUTION) clockSelectBits = _BV(CS51);              // prescale by /8
  else if((cycles >>= 3) < RESOLUTION) clockSelectBits = _BV(CS51) | _BV(CS50);  // prescale by /64
  else if((cycles >>= 2) < RESOLUTION) clockSelectBits = _BV(CS52);              // prescale by /256
  else if((cycles >>= 2) < RESOLUTION) clockSelectBits = _BV(CS52) | _BV(CS50);  // prescale by /1024
  else	cycles = RESOLUTION - 1, clockSelectBits = _BV(CS52) | _BV(CS50);		 // request was out of bounds, set as maximum
  
  oldSREG = SREG;				
  cli();						// Disable interrupts for 16 bit register access
  ICR5 = pwmPeriod = cycles;	// ICR5 is TOP in pfc mode
  SREG = oldSREG;
  
  TCCR5B &= ~(_BV(CS50) | _BV(CS51) | _BV(CS52)); //Reset clock select register
  TCCR5B |= clockSelectBits;		//Starts the clock with new clock select register
}

void TimerFive::setPwmDuty(char pin, int duty){	//Set PWM Duty Cycle to specified pin
  unsigned long dutyCycle = pwmPeriod;
  
  dutyCycle *= duty;
  dutyCycle >>= 10;
  
  oldSREG = SREG;
  cli();
  if(pin == 46){
	OCR5A = dutyCycle;
	DDRL |= _BV(PORTL3);
	TCCR5A |= _BV(COM5A1);
  }
  else if(pin == 45){
	OCR5B = dutyCycle;
	DDRL |= _BV(PORTL4);
	TCCR5A |= _BV(COM5B1);
  }
  else if(pin == 44){
	OCR5C = dutyCycle;
	DDRL |= _BV(PORTL5);
	TCCR5A |= _BV(COM5C1);
  }
  SREG = oldSREG;
}

void TimerFive::setPwmDuty(int duty){ //Set the same PWM Duty Cycle to all pins
  unsigned long dutyCycle = pwmPeriod;
  
  dutyCycle *= duty;
  dutyCycle >>= 10;
  
  oldSREG = SREG;
  cli();
  OCR5A = dutyCycle;
  OCR5B = dutyCycle;
  OCR5C = dutyCycle;
  DDRL |= _BV(PORTL3) | _BV(PORTL4) | _BV(PORTL5); //Make pins output
  TCCR5A |= _BV(COM5A1) | _BV(COM5B1) | _BV(COM5C1); //Enable PWM outputs
  SREG = oldSREG;
}

void TimerFive::pwm(char pin, int duty, long microseconds){	// expects duty cycle to be 10 bit (1024)
  if(microseconds > 0) setPeriod(microseconds);
  if(pin == 44) {
    DDRL |= _BV(PORTL3);									// sets data direction register for pwm output pin
    TCCR5A |= _BV(COM5A1);									// activates the output pin
  }
  else if(pin == 45) {
    DDRL |= _BV(PORTL4);
    TCCR5A |= _BV(COM5B1);
  }
  else if(pin == 46) {
    DDRL |= _BV(PORTL5);
    TCCR5A |= _BV(COM5C1);
  }
  setPwmDuty(pin, duty);
  resume();
}

void TimerFive::pwm(int duty, long microseconds){	// expects duty cycle to be 10 bit (1024)
  if(microseconds > 0) setPeriod(microseconds);
 
  DDRL |= _BV(PORTL3) | _BV(PORTL4) | _BV(PORTL5);	// sets data direction register for all pwm output pins
  TCCR5A |= _BV(COM5A1) | _BV(COM5B1) | _BV(COM5C1);// activates the output pins

  setPwmDuty(duty); //Set the same PWM Duty Cycle to all pins
  resume();
}

void TimerFive::disablePwm(char pin){
  if(pin == 44)			TCCR5A &= ~_BV(COM5A1);   // clear the bit that enables pwm on PL3
  else if(pin == 45)	TCCR5A &= ~_BV(COM5B1);   // clear the bit that enables pwm on PL4
  else if(pin == 46)	TCCR5A &= ~_BV(COM5C1);   // clear the bit that enables pwm on PL5
}

void TimerFive::attachInterrupt(void (*isr)(), long microseconds){
  if(microseconds > 0) setPeriod(microseconds);
  isrCallback = isr;                                       // register the user's callback with the real ISR
  TIMSK5 = _BV(TOIE5);                                     // sets the timer overflow interrupt enable bit
// might be running with interrupts disabled (eg inside an ISR), so don't touch the global state
// sei();
  resume();												
}

void TimerFive::detachInterrupt(){
  TIMSK5 &= ~_BV(TOIE5);    // clears the timer overflow interrupt enable bit 
							// timer continues to count without calling the isr
}

void TimerFive::resume(){
  TCCR5B |= clockSelectBits;
}

void TimerFive::restart(){
	start();				
}

void TimerFive::start(){
  unsigned int tcnt5;
  
  TIMSK5 &= ~_BV(TOIE5);
  GTCCR |= _BV(PSRSYNC);   		// reset prescaler (NB: shared with all 16 bit timers);

  oldSREG = SREG;				// save status register
  cli();						// Disable interrupts
  TCNT5 = 0;                	
  SREG = oldSREG;          		// Restore status register
	resume();
  do {	// Nothing -- wait until timer moved on from zero - otherwise get a phantom interrupt
	oldSREG = SREG;
	cli();
	tcnt5 = TCNT5;
	SREG = oldSREG;
  } while (tcnt5==0); 
 
//  TIFR5 = 0xff;              		//Clear interrupt flags
//  TIMSK5 = _BV(TOIE5);              // sets the timer overflow interrupt enable bit
}

void TimerFive::stop(){
  TCCR5B &= ~(_BV(CS50) | _BV(CS51) | _BV(CS52));	// clears all clock selects bits
}

unsigned long TimerFive::read(){	//returns the value of the timer in microseconds
  	unsigned long tmp;				//remember! phase and freq correct mode counts up to then down again
  	unsigned int tcnt5;

	oldSREG= SREG;
  	cli();							
  	tmp=TCNT5;    					
	SREG = oldSREG;

	char scale=0;
	switch (clockSelectBits){
	case 1:// no prescalse
		scale=0;
		break;
	case 2:// x8 prescale
		scale=3;
		break;
	case 3:// x64
		scale=6;
		break;
	case 4:// x256
		scale=8;
		break;
	case 5:// x1024
		scale=10;
		break;
	}
	
	do {	// Nothing -- max delay here is ~1023 cycles.
		oldSREG = SREG;
		cli();
		tcnt5 = TCNT5;
		SREG = oldSREG;
	} while (tcnt5==tmp); //if the timer has not ticked yet

	//if we are counting down add the top value to how far we have counted down
	tmp = (  (tcnt5>tmp) ? (tmp) : (long)(ICR5-tcnt5)+(long)ICR5  );
	return ((tmp*1000L)/(F_CPU /1000L))<<scale;
}

#endif