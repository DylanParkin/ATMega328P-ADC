#include <avr/interrupt.h>
#include <avr/io.h>
#include <inttypes.h>
#include <stdlib.h>
#include <util/delay.h>

volatile uint8_t ring_buffer[1000];
volatile uint16_t ring_buffer_index = 0;
volatile int state = 1;  // initial state
volatile int button_1_pressed = false;
volatile int button_2_pressed = false;
volatile uint16_t last_debounce_time = 0;
volatile uint16_t time = 0;  // 1 ms counter

int main() {
  cli();

  // Enable external interrupt 1 and 0
  EIMSK |= (1 << INT0) | (1 << INT1);
  EICRA |= (1 << ISC00) | (1 << ISC01);
  EICRA |= (1 << ISC11) | (1 << ISC10);

  ADCSRA |=
      (1 << ADEN) | (1 << ADIE) | (1 << ADPS0) | (1 << ADPS1) |
      (1 << ADPS2);  // ADC enable, interrupt enable and set prescaler to 128
  ADMUX |= (1 << MUX0);  // Select pin A1 (MUX0) to be ADC input
  ADMUX &= ~((1 << REFS0) | (1 << REFS1));  // Use external reference voltage
  ADMUX |= (1 << ADLAR);                    // Left adjust ADC

  /* Timer1 used for ADC */
  TCCR1B |=
      (1 << WGM12) | (1 << CS10) | (1 << CS11);  // Timer1 CTC mode prescaler 64
  TIMSK1 |= (1 << OCIE1A);  // Enable interrupt on match with OCR1A
  OCR1A = 250;              // 1ms

  Serial.begin(9600);

  sei();

  while (1) {
    switch (state) {
      case 1:  // State 1
        // LED blinks with 10% duty cycle, 1Hz:
        if ((time % 1000) <= 100) {
          PORTB |= (1 << PB5);
        } else {
          PORTB &= ~(1 << PB5);
        }

        if (button_1_pressed) {  // Change state
          button_1_pressed = false;
          new_state(2);
        }
        if (button_2_pressed) {  // Do nothing
          button_2_pressed = false;
        }
        break;
      case 2:  // State 2
        // LED blinks with 20% duty cycle, 2Hz:
        if ((time % 500) <= 100) {
          PORTB |= (1 << PB5);
        } else {
          PORTB &= ~(1 << PB5);
        }

        if (button_1_pressed) {  // Change state
          button_1_pressed = false;
          new_state(1);
        }
        if (button_2_pressed) {  // Transfer data
          button_2_pressed = false;
          transfer_data();
        }
        break;
    }
  }
}

void new_state(int ns) {  // State change function
  state = ns;
  if (ns == 1) {  // Clear ring buffer when transitioning to State 1
    ring_buffer_index = 0;
    for (int i = 0; i < 1000; i++) {
      ring_buffer[i] = 0;
    }
  }
}

bool debounced() {  // Performs debouncing, 200ms refractory period
  if (time - last_debounce_time < 200) {
    return false;
  } else {
    last_debounce_time = time;
    return true;
  }
}

void transfer_data() {  // Output ADC ring buffer contents to serial monitor
  Serial.println("ADC Ring buffer contents:");
  // Conditions allow print to restart if button is pressed again
  uint16_t i = 0;
  while (i < 1000 && !button_1_pressed && !button_2_pressed) {
    Serial.println(ring_buffer[i]);
    i++;
  }
}

ISR(TIMER1_COMPA_vect) {
  time++;
  if ((time % 5 == 0) && state == 1) {
    ADCSRA |= (1 << ADSC);  // ADC conversion every 5ms while in State 1
  }
}

ISR(ADC_vect) {
  // Add value read from ADC to ring buffer
  ring_buffer[ring_buffer_index] = ADCH;
  ring_buffer_index = (ring_buffer_index + 1) % 1000;
}

ISR(INT1_vect) {      // Button 1
  if (debounced()) {  // Ignores button noise, performs debouncing
    button_1_pressed = true;
  }
}

ISR(INT0_vect) {      // Button 2
  if (debounced()) {  // Ignores button noise, performs debouncing
    button_2_pressed = true;
  }
}
