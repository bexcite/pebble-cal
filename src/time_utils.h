#ifndef TIME_UTILS_H
#define TIME_UTILS_H

struct tm * gmtime(register const time_t *timer);

//this method must be passed to tick_timer_service_subscribe before using other utility methods
void handle_global_time_tick(struct tm *tick_time, TimeUnits units_changed);
//add and remove tick handlers that will be called from one central point
void register_tick_handler(TickHandler handler);
void unregister_tick_handler(TickHandler handler);

#endif