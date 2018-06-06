#include <pebble.h>
#include "agenda.h"
#include "event_details.h"
#include "event_layer.h"
#include "state_layer.h"
#include "clock.h"

/*
 * =============================================================
 *    Implementation Variables
 * =============================================================
 */

static Window *window;

static Layer *center_layer;
static StateLayer *state_layer;


static EventLayer *event1_text_layer;
static EventLayer *event2_text_layer;
static EventLayer *event3_text_layer;

typedef struct AgendaClickHandlers {
  ClickHandler up;
  ClickHandler select;
  ClickHandler down;
} AgendaClickHandlers;

static AgendaClickHandlers agendaClickHandlers;

static uint8_t events_error = 0; // 0 - no error, 1 (+unknown) - auth error, 2 - error with custom text, 3 - loading state
static char *events_error_text;

// ############# forward declaration ##################
static void show_events_error();
static void show_error(char *error_text);
static void show_error_and_time(char *error_text, time_t time_to_show);

/*
 * =============================================================
 *    Implementation Functions
 * =============================================================
 */

static void process_event(Event e, EventLayer *layer /*, char *line_container*/) {
  //APP_LOG(APP_LOG_LEVEL_DEBUG, "process_event");
  if (e.type == EVENT_TYPE_DEFAULT || e.type == EVENT_TYPE_ALL_DAY) {
    // event_line(e, line_container);
    event_layer_set_type(layer, e.type);
    event_layer_set_time(layer, e.start, e.end);
    event_layer_set_summary(layer, e.summary);
  } else {
    event_layer_set_type(layer, EVENT_TYPE_EMPTY);
    // event_layer_set_text(layer, "Empty");
  }

  event_layer_mark_dirty(layer);

}

//TODO: Refector all this events_error, codes and USE enums, and nice structure...
static void show_events_error() {
  if (events_error) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "show events_error for events_error=%hd", events_error);

    // If Error 2 than we have text
    if (events_error == 2) {
      show_error(events_error_text);
      return;
    } else if (events_error == 3) {
      // 3 - Show loading state (continue)
      layer_set_hidden(center_layer, true);
      state_layer_set_hidden(state_layer, false);
      state_layer_set_state(state_layer, STATE_LOADING);
      return;
    }

    // 1 - auth error (other codes also here)
    
    layer_set_hidden(center_layer, true);
    state_layer_set_hidden(state_layer, false);
    state_layer_set_state(state_layer, STATE_AUTH_ERROR);
  }
}

// Debug method
static void show_error(char *error_text) {
  events_error = 2;
  events_error_text = error_text;
  layer_set_hidden(center_layer, true);
  state_layer_set_hidden(state_layer, false);
  state_layer_set_text(state_layer, error_text);
}

static void show_error_with_time(char *error_text) {
  // Get Time
  time_t now = time(NULL);

  show_error_and_time(error_text, now);

}


static void show_error_and_time(char *error_text, time_t time_to_show) {
  // Get Time
  //time_t now = time(NULL);
  struct tm *current_time = localtime(&time_to_show);

  // Format time
  static char buffer_time[] = "             "; // 00:00:00
  strftime(buffer_time, sizeof(buffer_time), "%b %d, %H:%M", current_time); // %X

  // Compose a string
  static char buffer[80];
  strcpy(buffer, error_text);
  strcat(buffer, " ");
  strcat(buffer, buffer_time);

  // Show error
  show_error(buffer);
}


/*
 * DEBUG method
 */
static void select_click_handler(ClickRecognizerRef recognizer, void* context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Select click");
}

/*
 * DEBUG method
 */
static void up_click_handler(ClickRecognizerRef recognizer, void* context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Up click");
}

/*
 * DEBUG method - change fonts
 */
static void down_click_handler(ClickRecognizerRef recognizer, void* context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Down click");

  // Show Details Window
  //event_details_show();
  
}

static void back_click_handler(ClickRecognizerRef recognizer, void* context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Back click");
}


static void config_provider(void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Agenda Config Provider CALL");

  if (agendaClickHandlers.select != NULL) {
    window_single_click_subscribe(BUTTON_ID_SELECT, agendaClickHandlers.select);
  }

  if (agendaClickHandlers.up != NULL) {
    window_single_click_subscribe(BUTTON_ID_UP, agendaClickHandlers.up);
  }

  if (agendaClickHandlers.down != NULL) {
    window_single_click_subscribe(BUTTON_ID_DOWN, agendaClickHandlers.down);
  }
  
  //window_single_click_subscribe(BUTTON_ID_BACK, back_click_handler);
}

static void window_load(Window *window) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Main Window Load");

  Layer* window_layer = window_get_root_layer(window);

  GRect bounds = layer_get_bounds(window_layer);

  // Top Layer  
  clock_layer_init(window);

  // Center Layer
  GRect center_frame = (GRect) { .origin = {0, CLOCK_LAYER_HEIGHT + 1}, 
                                 .size = {bounds.size.w, 
                                  bounds.size.h - CLOCK_LAYER_HEIGHT - DATE_LAYER_HEIGHT + PBL_IF_RECT_ELSE(1, 0)}};
                                  
  center_layer = layer_create(center_frame);
  GRect center_layer_bounds = layer_get_bounds(center_layer);

  GSize center_layer_size = center_layer_bounds.size;
  event1_text_layer = event_layer_init(0, center_layer_size);
  event2_text_layer = event_layer_init(1, center_layer_size);
  event3_text_layer = event_layer_init(2, center_layer_size);

  
  layer_add_child(center_layer, event_layer_get_layer(event3_text_layer));
  layer_add_child(center_layer, event_layer_get_layer(event2_text_layer));
  layer_add_child(center_layer, event_layer_get_layer(event1_text_layer));
  
  layer_add_child(window_layer, center_layer);

  layer_set_hidden(center_layer, true); //untill it is loaded

  // END: center layer


  // Loading layer
  //TODO: extract/accedd to events_error
  events_error = 3; // it is in loading state

  state_layer = state_layer_init(center_frame);
  layer_add_child(window_layer, state_layer_get_layer(state_layer));
  //END loading layer

}

static void window_unload(Window *window) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Main Window UnLoad");

  clock_layer_deinit();

  layer_destroy(center_layer);
  state_layer_destroy(state_layer);

  event_layer_destroy(event1_text_layer);
  event_layer_destroy(event2_text_layer);
  event_layer_destroy(event3_text_layer);

}

/*
 * =============================================================
 *    Interface Functions
 * =============================================================
 */

 void agenda_set_up_click_handler(ClickHandler up) {
  agendaClickHandlers.up = up;
  window_set_click_config_provider(window, &config_provider);
}

void agenda_set_select_click_handler(ClickHandler select) {
  agendaClickHandlers.select = select;
  window_set_click_config_provider(window, &config_provider);
}

void agenda_set_down_click_handler(ClickHandler down) {
  agendaClickHandlers.down = down;
  window_set_click_config_provider(window, &config_provider);
}


 void agenda_refresh_calendar(Event *e) {
  if (events_error) {
    show_events_error();
    return;
  }
  agenda_update_calendar(e);
}

void agenda_update_calendar(Event *e) {

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Update calendar view ........... events_error=%hd", events_error);

  //APP_LOG(APP_LOG_LEVEL_DEBUG, "events_error=%hd", events_error);

  // clean error
  events_error = 0;

  layer_set_hidden(center_layer, false);

  process_event(e[0], event1_text_layer /*, line1*/);
  process_event(e[1], event2_text_layer /*, line2*/);
  process_event(e[2], event3_text_layer /*, line3*/);

  //hide state view and show events
  state_layer_set_hidden(state_layer, true);
  layer_set_hidden(center_layer, false);
  // state_layer_set_state(state_layer, STATE_AUTH_ERROR);

}


void agenda_show_auth_error() {
  events_error = 1; // auth error
  show_events_error();
}

void agenda_show_error_and_time(char *error_text, time_t time_to_show) {
  show_error_and_time(error_text, time_to_show);
}

void agenda_show_error(char *error_text) {
  show_error(error_text);
}


void agenda_init() {
  window = window_create();
  window_set_background_color(window, GColorBlack);
#ifdef PBL_PLATFORM_APLITE  
  //window_set_fullscreen(window, true);
#endif  

  // All Not Initialized Handlers should be NULL
  agendaClickHandlers = (AgendaClickHandlers){
    .up = &up_click_handler,
    .select = &select_click_handler,
    .down = &down_click_handler
  };

  window_set_click_config_provider(window, &config_provider);

  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload
  });

  window_stack_push(window, true);
}

void agenda_deinit() {
  window_destroy(window);
}