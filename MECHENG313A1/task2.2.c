#include <avr/interrupt.h>
#include <avr/io.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <util/delay.h>

volatile bool conversion_start_ok = false;
volatile uint8_t ring_buffer[1000];
volatile uint16_t ring_buffer_index = 0;

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
  ADMUX |= (1 << ADLAR);                    // Left adjust ADC

  TCCR1B |=
      (1 << WGM12) | (1 << CS10) | (1 << CS11);  // Timer1 CTC mode prescaler 64
  TIMSK1 |= (1 << OCIE1A);  // Enable interrupt on match with OCR1A
  OCR1A = 1250;             // Every 5ms

  sei();

  while (1) {
    // Ensure we start only one singular ADC conversion every 5 ms
    if (!(ADCSRA & (1 << ADSC)) && conversion_start_ok) {
      ADCSRA |= (1 << ADSC);
      conversion_start_ok = false;
    }
  }
}

ISR(ADC_vect) {
  PORTB ^= (1 << PB5);  // Toggle light every 5ms (DEMO)
  // Add value read from ADC to ring buffer
  ring_buffer[ring_buffer_index] = ADCH;
  ring_buffer_index = (ring_buffer_index + 1) % 1000;
}

ISR(TIMER1_COMPA_vect) { conversion_start_ok = true; }
