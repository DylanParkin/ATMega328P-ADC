#include <avr/interrupt.h>
#include <avr/io.h>
#include <inttypes.h>
#include <stdlib.h>
#include <util/delay.h>

volatile float result_conversion;
volatile int tick_count;

int main() {
  cli();

  // Set PB5 (on board LED) to output and initialise as off
  DDRB |= (1 << DDB5);
  PORTB &= ~(1 << PB5);

  ADCSRA |=
      (1 << ADEN) | (1 << ADIE) | (1 << ADPS0) | (1 << ADPS1) |
      (1 << ADPS2);  // ADC enable, interrupt enable and set prescaler to 128
  ADMUX |= (1 << MUX0);  // Select pin A1 (MUX0) to be ADC input
  ADMUX &= ~((1 << REFS0) | (1 << REFS1));  // Use external reference voltage

  TCCR1B |= (1 << WGM12) | (1 << CS12);  // Timer1 CTC mode prescaler 256
  TIMSK1 |= (1 << OCIE1A);               // Enable interrupt on match with OCR1A
  OCR1A = 6250;                          // 0.1 * (16M/256)

  tick_count = 0;
  sei();

  while (1) {
    if (result_conversion >= 2.0) {
      pulse_twice(true);
    } else {
      pulse_twice(false);
    }
  }
}

void pulse_twice(bool twice) {
  if (!twice) {  // LED 10% duty cycle 1 second period
    if (tick_count < 1) {
      PORTB |= (1 << PB5);
    } else {
      PORTB &= ~(1 << PB5);
    }

  } else {  // LED 20% duty cycle 1 period
    if (tick_count % 2 == 0 && tick_count < 4) {
      PORTB |= (1 << PB5);  // ON for even numbers below 4 (0, and 2)
    } else {
      PORTB &= ~(1 << PB5);
    }
  }
}

ISR(ADC_vect) { result_conversion = (5.0 * ADC) / 1024.0; }

ISR(TIMER1_COMPA_vect) {
  if (tick_count >= 10) {
    tick_count = 0;
    ADCSRA |= (1 << ADSC);  // Start new conversion exactly once per second
  } else {
    tick_count++;
  }
}
