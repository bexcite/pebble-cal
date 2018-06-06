#include <pebble.h>
#include "clock.h"
#include "event_layer.h"
#include "state_layer.h"
#include "time_utils.h"
#include "event_details.h"
#include "agenda.h"


#define EVENT_FETCH_INTERVAL 15 * 60 - 1     // 15 min (15 * 60 - 1)
#define EVENT_FETCH_TIMEOUT 45               // 45 sec
#define EVENT_UPDATE_INTERVAL 50             // 60 sec
#define EVENTS_IS_OLD_INTERVAL 24 * 60 * 60  // 24 hours (24 * 60 * 60)

enum {
  APP_KEY_UPDATE = 0x0,    // cmdUpdate
  APP_KEY_ERROR = 0x1      // cmdError
};

#define PERSIST_VERSION_KEY 10000
#define PERSIST_VERSION_VALUE 1

#define PERSIST_EVENT_KEY_0 0
#define PERSIST_EVENT_KEY_1 1
#define PERSIST_EVENT_KEY_2 2

#define PERSIST_EVENT_SUCCESS_LAST_TIME_KEY 100

#define EVENT_STRUCT_START 20
#define EVENT_STRUCT_COUNT 4

static Event events[3];
#define EVENTS_NUM sizeof(events)/sizeof(events[0])

//timers
static time_t events_fetch_last_time = 0;
static time_t event_timer_last_time = 0; //in secs

// success
static time_t events_fetch_success_last_time = 0;

// ############# forward declaration ##################
static void events_fetch();
static void events_fetch_success();
static bool events_fetch_should_retry();
static void events_fetch_send();
static bool events_is_old();
static void events_redraw_timer_callback(struct tm *tick_time, TimeUnits units_changed);
static void app_message_init(void);


/*
 * =============================================================
 *    Events Fetch Functions
 * =============================================================
 */

 static void events_fetch() {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Constructing events_fetch command >>>>>>>>>>>>>>>>> ");

  if (events_is_old()) {
    // N/A message could be if we have really old data to this moment
    agenda_show_error_and_time("New Data is N/A\nLast update ", events_fetch_success_last_time);
  } else {
    // Debug message - fetching with time
    //show_error_with_time("Fetching events ...");
  }

  if(!bluetooth_connection_service_peek()) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Bluetooth is not enabled. Drop send cmdUpdate.");
    return;
  }

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Bluetooth enabled.");

  DictionaryIterator *iter;
  AppMessageResult amResult = app_message_outbox_begin(&iter);
  if (amResult != APP_MSG_OK) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "amResult = %i", amResult);
  }
  Tuplet value = TupletInteger(APP_KEY_UPDATE, 42); // Just 42. Why we need to use 1 here? :)
  dict_write_tuplet(iter, &value);
  dict_write_end(iter); // optional but litb for convenience
  app_message_outbox_send();

  events_fetch_send();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "app_message_outbox_send() called");
}


// On received and update events successfully
static void events_fetch_success() {

  // Get time
  time_t current_time;
  time(&current_time);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "SUCCESS CURRENT_TIME = %lu, LAST SUCCESS = %lu, diff = %lu",
    (uint32_t)current_time,
    (uint32_t)events_fetch_success_last_time,
    (uint32_t)(current_time - events_fetch_success_last_time));
  APP_LOG(APP_LOG_LEVEL_DEBUG, "FETCH_LAST_TIME_DIFF = %lu",
    (uint32_t)(current_time - events_fetch_last_time));
  events_fetch_success_last_time = current_time;

  // Success: received calendar events
  //vibes_short_pulse();

}

static void events_fetch_send() {

  // Log events_fetch time
  //   Get time
  time_t current_time;
  time(&current_time);
  events_fetch_last_time = current_time;

}

static bool events_fetch_should_retry() {

  //APP_LOG(APP_LOG_LEVEL_DEBUG, "=== should retry");
  if (events_fetch_last_time) {

    time_t current_time;
    time(&current_time);

    //APP_LOG(APP_LOG_LEVEL_DEBUG, "=== should retry - present events_fetch_last_time");

    // if was not success after last fetch
    if (events_fetch_success_last_time < events_fetch_last_time) {

      // APP_LOG(APP_LOG_LEVEL_DEBUG, "=== should retry - there was not success after last fetch, back_diff = %lu",
      //   (uint32_t)(events_fetch_last_time - events_fetch_success_last_time));

      uint32_t diff_fetch_last_time = current_time - events_fetch_last_time;

      if (diff_fetch_last_time > EVENT_FETCH_TIMEOUT) {

        //APP_LOG(APP_LOG_LEVEL_DEBUG, "=== should retry - timeout exceed, diff = %lu", diff_fetch_last_time);

        return true;
      }

    }

  }

  //APP_LOG(APP_LOG_LEVEL_DEBUG, "=== should retry = false");
  return false;

}



static bool events_is_old() {

  if (events_fetch_success_last_time) {

    time_t current_time;
    time(&current_time);

    uint32_t diff = current_time - events_fetch_success_last_time;

    //APP_LOG(APP_LOG_LEVEL_DEBUG, "=== events_is_old() diff = %lu", diff);

    if (diff > EVENTS_IS_OLD_INTERVAL) {
      //APP_LOG(APP_LOG_LEVEL_DEBUG, "=== events_is_old() IS OLD!!!");
      return true;
    }

  }

  return false;
}


static void fetch_click_handler(ClickRecognizerRef recognizer, void* context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Select click");
  // TODO: Event fetch extract or smth
  events_fetch();  
}

/*
 * =============================================================
 *    Helper Functions
 * =============================================================
 */

static void log_events(Event *e) {
  for (uint16_t i = 0; i < EVENTS_NUM; i++) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "events[%d] = {.type = %hd, .start = %ld, .end = %ld, .summary = %s}",
      i, e[i].type, e[i].start, e[i].end, "summary ommited in code :(");//e[i].summary
  }
}

static void clean_events(Event *e) {
  for (uint16_t i = 0; i < EVENTS_NUM; i++) {
    e[i].type = 0;
    e[i].start = 0;
    e[i].end = 0;
    //e[i].summary = "";
    strcpy(e[i].summary, "");
  } 
}


/*
 * =============================================================
 *    Timer CallBacks Functions
 * =============================================================
 */

static void events_fetch_timer_callback(struct tm *tick_time, TimeUnits units_changed) {

  struct tm tick_time_struct;
  tick_time_struct = *tick_time;
  time_t tick_time_value = mktime(&tick_time_struct);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "events_fetch_timer_callback tickPtime %lu, %lu", (uint32_t)tick_time_value, (uint32_t)events_fetch_last_time);

  
  int diff = tick_time_value - events_fetch_last_time;

  APP_LOG(APP_LOG_LEVEL_DEBUG, "events_fetch_timer_callback diff %d", diff);

  if (diff < EVENT_FETCH_INTERVAL) {
    return;
  }
  APP_LOG(APP_LOG_LEVEL_DEBUG, "events_fetch_timer_callback set last time to %lu", (uint32_t)tick_time_value);
  events_fetch_last_time = tick_time_value;

  events_fetch();

}

static void events_redraw_timer_callback(struct tm *tick_time, TimeUnits units_changed) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "FIRE TIMER!!!");
  //hacky way to get time difference as mktime crashes all
  // yMMddHHmm

  // Check if we should retry events_fetch
  if (events_fetch_should_retry()) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, " FETCH SHOULD RETRY");
    events_fetch();
  }

  struct tm tick_time_struct;
  tick_time_struct = *tick_time;
  time_t tick_time_value = mktime(&tick_time_struct);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "events_redraw_timer_callback tick_time_value %lu", (uint32_t)tick_time_value);
  if (!event_timer_last_time) {
    event_timer_last_time = tick_time_value;
  }

  
  int diff = tick_time_value - event_timer_last_time;

  APP_LOG(APP_LOG_LEVEL_DEBUG, "events_redraw_timer_callback %d", diff);

  if (diff < EVENT_UPDATE_INTERVAL) {
    return;
  }
  event_timer_last_time = tick_time_value;

  agenda_refresh_calendar(events);

}



/*
 * =============================================================
 *    AppMessage Handlers
 * =============================================================
 */

static void in_received_handler(DictionaryIterator *received, void *context) {
  // incoming message received
  APP_LOG(APP_LOG_LEVEL_DEBUG, "in_received_handler");

/*
  Tuple *ft = dict_find(received, APP_KEY_ERROR);
  if (ft) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "APP_KEY_ERROR found!");
  }
  ft = dict_find(received, EVENT_STRUCT_START);
  if (ft) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "EVENT_STRUCT_START found!");
  }
*/

  Tuple *tuple = dict_read_first(received);

  // APP_LOG(APP_LOG_LEVEL_DEBUG, "Iterate over all received tuples");

  uint32_t current_key;
  uint16_t struct_index;
  uint16_t struct_offset;

  clean_events(events);

  // Clean error
  //events_error = 0;
  
  while (tuple) {

    current_key = tuple->key;

    APP_LOG(APP_LOG_LEVEL_DEBUG, ">>>> Received tuple->key = %ld", current_key);


    if (current_key == APP_KEY_ERROR) {
      // Error received
      //events_error = tuple->value->uint8;
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Received AUTH_ERROR, show agenda auth error");
      events_fetch_success_last_time = 0;
      agenda_show_auth_error();
      //show_events_error();
      /*
      //layer_set_hidden(center_layer, true);
      //state_layer_set_hidden(state_layer, false);
      //state_layer_set_state(state_layer, STATE_AUTH_ERROR);
      */
      return;
    }


    if (current_key >= EVENT_STRUCT_START) {
      // Key in events array

      struct_index = (current_key - EVENT_STRUCT_START) / EVENT_STRUCT_COUNT;
      struct_offset = (current_key - EVENT_STRUCT_START) % EVENT_STRUCT_COUNT;
      // APP_LOG(APP_LOG_LEVEL_DEBUG, "struct_index = %d, struct_offset = %d", struct_index, struct_offset);

      switch (struct_offset) {
        case 0: // Type
          // APP_LOG(APP_LOG_LEVEL_DEBUG, "type = %hd", tuple->value->uint8);
          events[struct_index].type = tuple->value->uint8;
          break;
        case 1: // Start
          // APP_LOG(APP_LOG_LEVEL_DEBUG, "start = %ld, %lx", tuple->value->uint32, tuple->value->uint32);
          events[struct_index].start = tuple->value->uint32;
          break;
        case 2: // End
          // APP_LOG(APP_LOG_LEVEL_DEBUG, "end = %ld, %lx", tuple->value->uint32, tuple->value->uint32);
          events[struct_index].end = tuple->value->uint32;
          break;
        case 3: // Summary
          // APP_LOG(APP_LOG_LEVEL_DEBUG, "summary = %s", tuple->value->cstring);
          //events[struct_index].summary = tuple->value->cstring;
          strcpy(events[struct_index].summary, tuple->value->cstring);
          break;

      }

    }

    tuple = dict_read_next(received);
  }

  log_events(events);

  events_fetch_success();
  agenda_update_calendar(events);

}

static void in_dropped_handler(AppMessageResult reason, void *context) {
  // incoming message dropped
  APP_LOG(APP_LOG_LEVEL_DEBUG, "in_dropped_handler %i", reason);
}

static void out_sent_handler(DictionaryIterator *sent, void *context) {
   // outgoing message was delivered
  APP_LOG(APP_LOG_LEVEL_DEBUG, "out_sent_handler");
}

static void pc_out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
  // outgoing message failed
  APP_LOG(APP_LOG_LEVEL_DEBUG, "pc_out_failed_handler");
}


/*
 * =============================================================
 *    Persistent Storage Functions
 * =============================================================
 */

static void read_events_data() {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "=== Read Events Data ===");
  uint32_t version = persist_read_int(PERSIST_VERSION_KEY);
  if (version == 0) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "- storage is not present yet");
  } else if (version == PERSIST_VERSION_VALUE) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "- storage is present, and we can read events data from it, sizeof(events[0]) = %d", sizeof(events[0]));

    // read events data
    persist_read_data(PERSIST_EVENT_KEY_0, &events[0], sizeof(events[0]));
    persist_read_data(PERSIST_EVENT_KEY_1, &events[1], sizeof(events[1]));
    persist_read_data(PERSIST_EVENT_KEY_2, &events[2], sizeof(events[2]));
    log_events(events);

    events_fetch_success_last_time = persist_read_int(PERSIST_EVENT_SUCCESS_LAST_TIME_KEY);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "- set events_fetch_success_last_time = %lu", (uint32_t)events_fetch_success_last_time);

    if (events_is_old()) {
      APP_LOG(APP_LOG_LEVEL_DEBUG, "- restored events is old, so not use it");
      events_fetch_success_last_time = 0;
      return;
    }

    // Clean error and show restored events
    //events_error = 0;
    agenda_update_calendar(events);

  } else {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "- unknown version of storage, do nothing");
  }
}


static void write_events_data() {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "=== Write Events Data ===");
  if (events_fetch_success_last_time > 0) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "- events_fetch_success_last_time > 0, so let's store our events");
    persist_write_int(PERSIST_VERSION_KEY, PERSIST_VERSION_VALUE);

    persist_write_int(PERSIST_EVENT_SUCCESS_LAST_TIME_KEY, events_fetch_success_last_time);

    persist_write_data(PERSIST_EVENT_KEY_0, &events[0], sizeof(events[0]));
    persist_write_data(PERSIST_EVENT_KEY_1, &events[1], sizeof(events[1]));
    persist_write_data(PERSIST_EVENT_KEY_2, &events[2], sizeof(events[2]));
  }
}


/*
 * =============================================================
 *    Init/Deinit Functions
 * =============================================================
 */

static void init(void) {

  // Main Window Init
  agenda_init();
  agenda_set_select_click_handler(&fetch_click_handler);

  app_message_init();

  time_t seed_time = time(NULL);

  srand(seed_time); // This leaks 28B So dont worry

/*
  events[0] = (Event) {.type = 0, .start = 0, .end = 0, .summary = "Event One"};
  events[1] = (Event) {.type = 0, .start = 0, .end = 0, .summary = "Event Two"};
  events[2] = (Event) {.type = 0, .start = 0, .end = 0, .summary = "Event Three"};
*/


  // Register TickHandlers
  tick_timer_service_subscribe(MINUTE_UNIT, handle_global_time_tick);  
  register_tick_handler(events_fetch_timer_callback);
  register_tick_handler(events_redraw_timer_callback);

  // Restore from persist
  read_events_data();

  // Event Details Window
  event_details_init();

}

static void app_message_init(void) {
  // Register message handlers
  app_message_register_inbox_received(in_received_handler);
  app_message_register_inbox_dropped(in_dropped_handler);
  app_message_register_outbox_sent(out_sent_handler);
  app_message_register_outbox_failed(pc_out_failed_handler);


  // Init buffers inbound, outbound
  //app_message_open(256, 256);
  app_message_open(512, 256);

  // Max size was not working on aplite platform - Pavlo, Dec 16, 2015
  // app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());

}


static void deinit(void) {

  // Event Details Window
  event_details_deinit();

  // Write to persist
  write_events_data();

  // NOTE: This app leaks 28B at the end because of srand function
  // NOTE: And other leaks for custom font load - need to solve this
  tick_timer_service_unsubscribe();  

  // Main Window Deinit
  agenda_deinit();
}


int main(void) {
  init();
  app_event_loop();
  deinit();
}
