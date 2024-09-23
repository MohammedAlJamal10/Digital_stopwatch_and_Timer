/*
 *                  main.c
 *      Created on: September 17, 2024
 *         Author: Mohammed_AlJamal
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

unsigned char pause=0;
unsigned char countdown_mode=0;
unsigned char secs=0;
unsigned char hrs=0;
unsigned char mins=0;

void Timer1_Init_CTC_Mode(void)
{
	TCNT1 =0;
	TCCR1A = (1<<FOC1A) | (1<<FOC1B);
	TCCR1B |= (1 << WGM12) | (1<<CS12) | (1<<CS10); // CTC mode and 1024 prescaler
	/*
	 * Fcpu = 16 MHz
	 * N = 1024
	 * Ttime = 64 micro second
	 * overflows per sec = 1 sec / 64 micro sec = 15625 overflows
	 * */
	OCR1A  = 15625;
	TIMSK |= (1<<OCIE1A);
	SREG |= (1<<7);
}

void output_bcd(unsigned char number , unsigned char segment)
{
	/* last four bits*/
	PORTC = (PORTC & 0xF0) | (number & 0x0F);
	/* Enable selected digit only*/
	PORTA = (PORTA & 0xC0) | (1 << segment);
}

void display()
{
	unsigned char counter;
	unsigned char digits[6] = {hrs / 10, hrs % 10, mins / 10, mins % 10, secs / 10, secs % 10};

	for(counter=0 ; counter<6 ; counter++)
	{
		output_bcd (digits[counter] , counter);
		_delay_ms(2);
		PORTA = 0x00;
	}
}

ISR(INT0_vect)
{
	TCNT1=0;
	hrs=0;
	mins=0;
	secs=0;
}


ISR(INT1_vect)
{
	TCCR1B &= 0xF8;
	pause=1;
}

ISR(INT2_vect)
{
	TCCR1B = (1 << WGM12) | (1 << CS12) | (1 << CS10);
	PORTD&=~(1<<PD0);
	pause=0;
}

ISR(TIMER1_COMPA_vect)
{
	if (countdown_mode)
	{
		if(secs==0)
		{
			if (mins ==0)
			{
				if(hrs==0)
				{
					PORTD |= (1<< PD0);
					return;
				}
				hrs--;
				mins=59;
				secs=59;
			}
			else
			{
				mins--;
				secs=59;
			}
		}
		else
		{
			secs--;
		}
	}
	//Count Up mode
	else
	{
		//Turn on Red led
		PORTD |= (1<<PD4);
		//Turn off buzzer
		PORTD &=~ (1<<PD0);
		secs++;
		if(secs>=60)
		{
			secs=0;
			mins++;
			if (mins>=60)
			{
				mins=0;
				hrs++;
				if(hrs>=24)
				{
					hrs=0;
				}

			}
		}
	}
}

int main (void)
{
	/* Initialization code */

	/*configuration of the Reset Button (INT0) */
	DDRD &=~ (1<<PD2);
	PORTD |= (1<<PD2); /* Enable Internal Pull up*/
	/* Enable Falling Edge */
	MCUCR |=  (1<<ISC01);
	MCUCR &=~ (1<<ISC00);
	GICR  |= (1<<INT0);
	SREG  |= (1<<7);

	/*configuration the Pause Button (INT1) */
	DDRD &=~ (1<<PD3);
	/* Enable Raising Edge*/
	MCUCR |= (1<<ISC11) | (1<<ISC10);
	GICR  |= (1<<INT1);
	SREG  |= (1<<7);
	TCNT1 =0;
	PORTD &=~ (1<<PD0);

	/*configuration Resume Button (INT2) */
	DDRB &=~ (1<<PB2);
	PORTB |= (1<<PB2); /* Enable Internal Pull up*/
	/* Enable Falling Edge*/
	MCUCSR &=~ (1<< ISC2);
	GICR  |= (1<<INT2);

	/*configuration of Mode Toggle Button */
	DDRB &=~ (1<<PB7);
	PORTB |= (1<<PB7); /* Enable Internal Pull up*/

	/*configuration  Hours Adjustment Button */
	DDRB &=~ (1<<PB1);
	PORTB |= (1<<PB1); /* Enable Internal Pull up*/
	DDRB &=~ (1<<PB0);
	PORTB |= (1<<PB0); /* Enable Internal Pull up*/

	/*configuration of Minutes Adjustment Button */
	DDRB &=~ (1<<PB4);
	PORTB |= (1<<PB4); /* Enable Internal Pull up*/
	DDRB &=~ (1<<PB3);
	PORTB |= (1<<PB3); /* Enable Internal Pull up*/

	/*configuration  Seconds Adjustment Button */
	DDRB &=~ (1<<PB6);
	PORTB |= (1<<PB6); /* Enable Internal Pull up*/
	DDRB &=~ (1<<PB5);
	PORTB |= (1<<PB5); /* Enable Internal Pull up*/

	/* configuration the LED as O/P */
	DDRD |= (1<<PD4); /* counting up LED (Red)*/
	DDRD |= (1<<PD5); /* counting down LED (yellow)*/

	/* configuration Buzzer as O/P */
	DDRD |= (1<<PD0);

	DDRC |= 0x0F; // PC0-PC3 as output
	PORTC &= 0xF0; // Initialize PORTC to zero
	DDRA |= 0x3F; // PORTA as output

	PORTD |= (1 << PD4); // Enable normal increment mode (red LED on)
	PORTD &= ~(1 << PD5);


	Timer1_Init_CTC_Mode ();

	/* Enable global interrupt*/
	sei();


	while (1)
	{
		display();

		while(pause)
		{
			display();

			if (!(PINB & (1 << PB7)))
			{
				countdown_mode ^= 1;
				//Countdown led on
				PORTD ^= (1 << PD5);
				//normal led off
				PORTD &= ~(1 << PD4);

				while(!(PINB & (1 << PB7)))
				{
					display();
				}
			}

			/* Hrs increment button*/
			if(!(PINB & (1<<PB1)))
			{
				hrs = (hrs+1) % 24;
				while(!(PINB & (1<<PB1)))
				{
					display();
				}
			}

			/* Hrs decrement button*/
			if(!(PINB & (1<<PB0)))
			{
				hrs = (hrs==0) ? 23 : hrs-1;
				while(!(PINB & (1<<PB0)))
				{
					display();
				}
			}

			/* mins increment button*/
			if(!(PINB & (1<<PB4)))
			{
				mins = (mins+1) % 60;
				while(!(PINB & (1<<PB4)))
				{
					display();
				}
			}

			/* mins decrement button*/
			if(!(PINB & (1<<PB3)))
			{
				mins = (mins==0) ? 59 : mins-1;
				while(!(PINB & (1<<PB3)))
				{
					display();
				}
			}

			/* secs increment button*/
			if(!(PINB & (1<<PB6)))
			{
				secs = (secs+1) % 60;
				while(!(PINB & (1<<PB6)))
				{
					display();
				}
			}

			/* secs decrement button*/
			if(!(PINB & (1<<PB5)))
			{
				secs = (secs==0) ? 59 : secs-1;
				while(!(PINB & (1<<PB5)))
				{
					display();
				}
			}
		}
	}
}


