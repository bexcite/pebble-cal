#include <pebble.h>
#include "time_utils.h"


#define YEAR0           1900                    /* the first year */
#define EPOCH_YR        1970            /* EPOCH = Jan 1 1970 00:00:00 */
#define SECS_DAY        (24L * 60L * 60L)
#define LEAPYEAR(year)  (!((year) % 4) && (((year) % 100) || !((year) % 400)))
#define YEARSIZE(year)  (LEAPYEAR(year) ? 366 : 365)
#define FIRSTSUNDAY(timp)       (((timp)->tm_yday - (timp)->tm_wday + 420) % 7)
#define FIRSTDAYOF(timp)        (((timp)->tm_wday - (timp)->tm_yday + 420) % 7)
#define TIME_MAX        ULONG_MAX
#define ABB_LEN         3

const uint16_t _ytab[2][12] = {
        { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
        { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
};


//tick timer
#define MAX_TICK_HANDLERS 5

TickHandler tick_handlers[MAX_TICK_HANDLERS]; //max 5 handlers
uint8_t tick_handlers_counter;


struct tm * gmtime1(register const time_t *timer)
{
        static struct tm br_time;
        register struct tm *timep = &br_time;
        time_t time = *timer;
        register unsigned long dayclock, dayno;
        int year = EPOCH_YR;

        dayclock = (unsigned long)time % SECS_DAY;
        dayno = (unsigned long)time / SECS_DAY;

        timep->tm_sec = dayclock % 60;
        timep->tm_min = (dayclock % 3600) / 60;
        timep->tm_hour = dayclock / 3600;
        timep->tm_wday = (dayno + 4) % 7;       /* day 0 was a thursday */
        while (dayno >= YEARSIZE(year)) {
                dayno -= YEARSIZE(year);
                year++;
        }
        timep->tm_year = year - YEAR0;
        timep->tm_yday = dayno;
        timep->tm_mon = 0;
        while (dayno >= _ytab[LEAPYEAR(year)][timep->tm_mon]) {
                dayno -= _ytab[LEAPYEAR(year)][timep->tm_mon];
                timep->tm_mon++;
        }
        timep->tm_mday = dayno + 1;
        timep->tm_isdst = 0;

        return timep;
}


// ===================== Tick timer services =====================

void handle_global_time_tick(struct tm *tick_time, TimeUnits units_changed) {
  for (int i = 0; i < tick_handlers_counter; ++i) {
    tick_handlers[i](tick_time, units_changed);
  }
}

void register_tick_handler(TickHandler handler) {
  if (tick_handlers_counter < MAX_TICK_HANDLERS) {
    tick_handlers[tick_handlers_counter] = handler;
    tick_handlers_counter++;
  }
}

void unregister_tick_handler(TickHandler handler) {
  bool do_shift = false; //shift array to fill the gap after existing handler is found in array
  for (int i = 0; i < tick_handlers_counter; ++i) {
    if (tick_handlers[i] == handler) {
      do_shift = true;
      continue;
    } else if (do_shift) {
      tick_handlers[i-1] = tick_handlers[i];
    }
  }
  tick_handlers_counter--;
}