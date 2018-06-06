#ifndef AGENDA_H
#define AGENDA_H

#include <pebble.h>
#include "event_layer.h"
#include "state_layer.h"

typedef struct {
  uint8_t type;
  uint32_t start;
  uint32_t end;
  char summary[150];
} Event;

// Sets click handler if needed
void agenda_set_up_click_handler(ClickHandler up);
void agenda_set_select_click_handler(ClickHandler select);
void agenda_set_down_click_handler(ClickHandler down);

// Update events if there is no errors
void agenda_refresh_calendar(Event *e);

// Clean all errors and show events
void agenda_update_calendar(Event *e);

// Shows errors
void agenda_show_auth_error();
void agenda_show_error_and_time(char *error_text, time_t time_to_show);
void agenda_show_error(char *error_text);

// Init/Deinit
void agenda_init();
void agenda_deinit();

#endif
