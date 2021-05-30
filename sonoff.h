// vim: ts=4:sw=4:et:tw=80
/*
  sonoff.h

  Header of sonoff.cpp
*/

/*
  Copyright 2021 Sébastien Millet

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


#ifndef SONOFF_H
#define SONOFF_H

#define PIN_RFINPUT  2
#define INT_RFINPUT  0

#define TIMINGS_LEN 16

class Sonoff {
    private:

        // DC = Duration Category
        enum { DC_UNKNOWN, DC_SHORT, DC_LONG, DC_SEP };

        volatile static bool is_in_receive_mode;
        static void enter_mode_receive();
        static void leave_mode_receive();

        volatile static bool is_recording;
        volatile static bool is_available;
        volatile static uint32_t received_val;
        volatile static uint32_t copy_received_val;

        volatile static unsigned int timings[TIMINGS_LEN];

        volatile static int count_int;
        volatile static bool count_int_has_cycled;

        static bool is_in_range(unsigned int n,
          unsigned int vmin, unsigned int vmax);

        static const byte duration_cat(unsigned int n);

        static void handle_int_receive();
        static void handle_int_wait_free();

    public:

        Sonoff();

        static bool has_received_val();
        static uint32_t consume_received_val();

        bool is_busy();
        uint32_t get_val(bool wait = false);
        bool get_val_non_blocking(uint32_t* val, bool wait = false);

        static void wait_free_433();
};

#endif // SONOFF_H

