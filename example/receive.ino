// vim: ts=4:sw=4:et:tw=80

/*
  receive.ino

  Receives codes from a Sonoff telecommand and display it.
  Also, wait for the telecommand to end its transmission before display.
  This shows the feature wait_free_433(), useful if you plan to retransmit code
  over 433Mhz using another protocol.

  Schema:
    'data' of RF433 receiver needs be plugged on PIN 'D2' of Arduino.
*/

/*
  Copyright 2021 Sébastien Millet

  receive.ino is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  receive.ino is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses>.
*/

#include "sonoff.h"

// NOTE
//   As it is done Today, the code will show absolutely nothing if DEBUG is off.
//   You may comment it out (turn off debugging) if you update the code to do
//   something else...
#define DEBUG

#ifdef DEBUG

static char serial_printf_buffer[80];

static void serial_printf(const char* fmt, ...)
     __attribute__((format(printf, 1, 2)));

static void serial_printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsnprintf(serial_printf_buffer, sizeof(serial_printf_buffer), fmt, args);
    va_end(args);

    serial_printf_buffer[sizeof(serial_printf_buffer) - 1] = '\0';
    Serial.print(serial_printf_buffer);
}

static void serial_begin(long speed) {
    Serial.begin(speed);
}

#else // DEBUG

#define serial_printf(...)
#define serial_begin(speed)

#endif // DEBUG

Sonoff rx;

void setup() {
    serial_begin(115200);
    serial_printf("Start\n");

    pinMode(PIN_RFINPUT, INPUT);
}

void loop() {
    serial_printf("Waiting for signal\n");

    uint32_t val = rx.get_val();
    (void)val;  /* in case debug is off, don't warn of unused variable */

    rx.wait_free_433();

    serial_printf("Received 0x%08lx\n", val);
}

