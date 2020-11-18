#define REPEAT_DELAY 220

#include "TrinketHidCombo.h"
#include <Arduino.h>
#include <avr/delay.h>

volatile uint8_t m = 0, tcnt = 0, startflag = 0;
uint32_t irdata = 0, keydata = 0;

bool pressed = false;
bool complete = false;

void Action(uint32_t keycode);
void ms_delay(uint16_t x);
void setup() {
  DDRB |= (1 << DDB1); // P1 (LED) OUT not used in sketch
  PORTB |= 1 << PB2;   // a PB2 lift will not hurt.
  GIMSK |= 1 << INT0;  // interrupt int0 enable
  MCUCR |= 1 << ISC00; // Any logical change on INT0
                       // generates an interrupt request
  GTCCR |= 1 << PSR0;
  TCCR0A = 0;
  TCCR0B = (1 << CS02) | (1 << CS00); // divider /1024
  TIMSK = 1 << TOIE0;      // interrupt Timer/Counter1 Overflow  enable
  TrinketHidCombo.begin(); // start the USB device engine and enumerate
}

void loop() {
  if (complete) {       // if a code has been received
    if (keydata != 0) { // if a code is new
      Action(keydata);
      pressed = true;
    }
    complete = false;
    ms_delay(REPEAT_DELAY); // to balance repeating/input delay of the remote
  } else if (pressed) {
    digitalWrite(1, LOW);
    TrinketHidCombo.pressKey(0, 0);
    pressed = false;
  } else {
    _delay_ms(1);           // restrain USB polling on empty cycles
    TrinketHidCombo.poll(); // check if USB needs anything done
  }
}

ISR(INT0_vect) {
  if (PINB & 1 << 2) { // If log1
    TCNT0 = 0;
  } else {
    tcnt = TCNT0; // If log0
    if (startflag) {
      if (30 > tcnt && tcnt > 2) {
        if (tcnt > 15 && m < 32) {
          irdata |= (2147483648 >> m);
        }
        m++;
      }
    } else {
      startflag = 1;
    }
  }
}
ISR(TIMER0_OVF_vect) {
  if (m) {
    complete = true;
  }
  m = 0;
  startflag = 0;
  keydata = irdata;
  irdata = 0; // if the index is not 0, then create an end flag
}

void ms_delay(uint16_t x) // USB polling delay function
{
  for (uint16_t m = 0; m < (x / 10); m++) {
    _delay_ms(10);
    TrinketHidCombo.poll();
  }
}

void Action(uint32_t keycode) {
  // сохраняем значение чтобы оно не потерялось на прерывании
  uint32_t code = keydata;
  uint32_t checkInt = 1;
  for (int i = 8; i > 0; i--) {
    checkInt = (uint32_t)(checkInt << 4); // умножение на 16 (2^4)
    if (code < checkInt) {
      TrinketHidCombo.print("0");
    }
  }
  TrinketHidCombo.print(code, HEX);
  digitalWrite(1, HIGH);
}
