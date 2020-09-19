// vim: ts=4:sw=4:et:tw=80
/*
  sonoff.cpp

  Library to receive codes from a Sonoff device (433Mhz).

  Schema:
    'data' of RF433 receiver needs be plugged on PIN 'D2' of Arduino.
*/

/*
  Copyright 2020 Sébastien Millet

  sonoff is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation, either version 3 of the
  License, or (at your option) any later version.

  sonoff is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this program. If not, see
  <https://www.gnu.org/licenses>.
*/

#include <Arduino.h>
#include <avr/sleep.h>

#include "sonoff.h"

volatile bool Sonoff::is_in_receive_mode = false;
volatile bool Sonoff::is_recording = false;
volatile uint32_t Sonoff::received_val = 0;
volatile uint32_t Sonoff::copy_received_val = 0;
volatile bool Sonoff::is_available = false;
volatile int Sonoff::count_int = 0;
volatile bool Sonoff::count_int_has_cycled = false;
volatile unsigned int Sonoff::timings[TIMINGS_LEN];

Sonoff::Sonoff() { }

bool Sonoff::is_in_range(unsigned int n,
                              unsigned int vmin, unsigned int vmax) {
    return n >= vmin && n <= vmax;
}

const byte Sonoff::duration_cat(unsigned int n) {

    if (is_in_range(n, 220, 550))
        return DC_SHORT;

    if (is_in_range(n, 800, 1200))
        return DC_LONG;

    if (is_in_range(n, 7000, 13000))
        return DC_SEP;

    return DC_UNKNOWN;
}

bool Sonoff::has_received_val() {
    return is_available;
}

uint32_t Sonoff::consume_received_val() {
    uint32_t retval = 0xFFFFFFFF;
    if (is_in_receive_mode && is_available) {
        retval = copy_received_val;
    }
    leave_mode_receive();
    return retval;
}

void Sonoff::leave_mode_receive() {
    detachInterrupt(INT_RFINPUT);
    is_in_receive_mode = false;

    is_available = false;
    is_recording = false;
}

void Sonoff::enter_mode_receive() {
    is_available = false;
    is_recording = false;

    is_in_receive_mode = true;
    attachInterrupt(INT_RFINPUT, &handle_int_receive, CHANGE);
}

void Sonoff::handle_int_receive() {
    static unsigned long last_t = 0;

    static byte last_cat;
    static int counter;

    const unsigned long t = micros();
    const unsigned long duration = t - last_t;
    last_t = t;
    const byte cat = duration_cat(duration);

        // Defensive programming:
        //   If we are not in receive mode, the interrupt should not occur...
        //   But who knows, better reject such a case.
    if (!is_in_receive_mode)
        return;

        // NOTE
        //   Not sure it is wise to read a PIN in an interrupt handler.
        //   But, this leaves room for a fully inverted signal...
        //   IMHO, not a big issue.
        //   So I use the code WITHOUT reading PIN, and comment out the one
        //   doing this check.
//    if (cat == DC_SEP && digitalRead(PIN_RFINPUT) == HIGH) {
    if (cat == DC_SEP) {
        counter = 0;
        received_val = 0;
        is_recording = true;
        return;
    }

    if (cat != DC_SHORT && cat != DC_LONG)
        is_recording = false;

    if (!is_recording)
        return;

    ++counter;
    if (is_recording && counter == 49) {
        copy_received_val = received_val;
        is_available = true;
        is_recording = false;
        return;
    }

    if (!(counter % 2)) {
        byte b;
        if (last_cat == DC_SHORT && cat == DC_LONG) {
            b = 0;
        } else if (last_cat == DC_LONG && cat == DC_SHORT) {
            b = 1;
        } else {
            is_recording = false;
            return;
        }
        received_val <<= 1;
        received_val |= b;
    } else {
        last_cat = cat;
    }
}

void Sonoff::handle_int_wait_free() {
    static unsigned long last_t = 0;

    unsigned long t = micros();
    timings[count_int] = t - last_t;
    last_t = t;

    ++count_int;
    if (count_int >= TIMINGS_LEN) {
        count_int = 0;
        count_int_has_cycled = true;
    }
}

void Sonoff::wait_free_433() {
    count_int = 0;
    count_int_has_cycled = false;

    attachInterrupt(INT_RFINPUT, &handle_int_wait_free, CHANGE);

    is_recording = false;

    unsigned long t0 = micros();
    unsigned long delta;

    do  {
        delta = micros() - t0;
    } while (!count_int_has_cycled && delta < 30000);

    if (!count_int_has_cycled)
        return;

    t0 = micros();
    long is_code;
    do {
        delay(1);
        is_code = 0;
        for (int i = 0; i < TIMINGS_LEN; ++i) {
            byte cat = duration_cat(timings[i]);
            if (cat == DC_SHORT || cat == DC_LONG || cat == DC_SEP)
                ++is_code;
        }
    } while ((100 * is_code) / TIMINGS_LEN >= 75);

    detachInterrupt(INT_RFINPUT);
}

bool Sonoff::is_busy() {
    return is_in_receive_mode && (is_recording || is_available);
}

uint32_t Sonoff::get_val(bool wait) {
    enter_mode_receive();
    sleep_enable();
    set_sleep_mode(SLEEP_MODE_IDLE);

    while (!has_received_val())
        sleep_mode();

    if (wait) {
        wait_free_433();
    }

    return consume_received_val();
}

//bool Sonoff::get_val_non_blocking(uint32_t* val, bool wait) {
//    if (!has_received_val())
//        return false;
//    return true;
//}

