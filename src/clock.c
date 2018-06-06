#include <pebble.h>
#include "clock.h"
#include "time_utils.h"

#define BATTERY_IMAGE_PADDING_RIGHT 5

#if defined(PBL_COLOR)
  #define COLOR_DATE PBL_IF_RECT_ELSE(GColorPastelYellow, GColorPastelYellow)
  #define COLOR_TIME PBL_IF_RECT_ELSE(GColorWhite, GColorWhite)
  #define COLOR_BACKGROUND_TIME PBL_IF_RECT_ELSE(GColorDukeBlue, GColorDukeBlue)
#elif defined(PBL_BW)
  #define COLOR_DATE GColorWhite
  #define COLOR_TIME GColorWhite
  #define COLOR_BACKGROUND_TIME GColorBlack
#endif

Layer *top_layer;
Layer *bottom_layer;

// battery
uint8_t battery_percent = 0;

GBitmap *battery_image;
BitmapLayer *battery_image_layer;
BitmapLayer *battery_layer;

GBitmap *charging_image;
BitmapLayer *charging_layer;

GBitmap *bluetooth_image;
BitmapLayer *bluetooth_layer;
bool init_finished = false;

//Time
TextLayer *hour_layer;
TextLayer *minute_layer;
TextLayer *ampm_layer;
TextLayer *date_layer;


//############### PRIVATE FUNCTIONS PREDECLARATION ########
static void battery_layer_update_proc(Layer *layer, GContext *ctx);
static void update_battery(BatteryChargeState charge_state);
static void update_bluetooth(bool connected);
static void handle_tick(struct tm *tick_time, TimeUnits units_changed);

void battery_layer_init();
void bluetooth_layer_init();
void hour_layer_init_pbl_rect();
void hour_layer_init_pbl_round(Layer *layer);
void date_layer_init_pbl_round(Layer *layer);


//############### IMPLEMENTATION ##########################

void clock_layer_init(Window* window) {

  Layer* window_layer = window_get_root_layer(window);
  window_set_background_color(window, COLOR_BACKGROUND_TIME);

  GRect bounds = layer_get_bounds(window_layer);

  top_layer = layer_create((GRect) { .origin = {0,0}, .size = {bounds.size.w, CLOCK_LAYER_HEIGHT}}); // Height of top panel
  bottom_layer = layer_create((GRect) { .origin = {0, bounds.size.h - DATE_LAYER_HEIGHT},
                                        .size = {bounds.size.w, DATE_LAYER_HEIGHT}});
  
  layer_add_child(window_layer, top_layer);
  layer_add_child(window_layer, bottom_layer);

  // clock_layer_init finished
#ifndef PBL_ROUND
  battery_layer_init();
  bluetooth_layer_init();
  hour_layer_init_pbl_rect(top_layer);
#else
  hour_layer_init_pbl_round(top_layer);
  date_layer_init_pbl_round(bottom_layer);
#endif

  register_tick_handler(handle_tick);
}

// -------------- Battery --------------------

void battery_layer_init() {

  APP_LOG(APP_LOG_LEVEL_DEBUG, "battery_layer_init started");
  GRect top_layer_bounds = layer_get_bounds(top_layer);

      // Battery icon
  battery_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY);
  GRect bi_bounds = gbitmap_get_bounds(battery_image);
  GRect fr = (GRect) {
    .origin = {top_layer_bounds.size.w - bi_bounds.size.w - BATTERY_IMAGE_PADDING_RIGHT, 
                (top_layer_bounds.size.h - bi_bounds.size.h)/2 },
    .size = bi_bounds.size
  };
  battery_image_layer = bitmap_layer_create(fr);
  battery_layer = bitmap_layer_create(fr);
  bitmap_layer_set_bitmap(battery_image_layer, battery_image);
  layer_set_update_proc(bitmap_layer_get_layer(battery_layer), battery_layer_update_proc);
  layer_add_child(top_layer, bitmap_layer_get_layer(battery_image_layer));
  layer_add_child(top_layer, bitmap_layer_get_layer(battery_layer));

  APP_LOG(APP_LOG_LEVEL_DEBUG, "battery_layer_init main finished. Updating...");

  // Charging icon
  
  charging_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CHARGE);
  GRect ci_bounds = gbitmap_get_bounds(charging_image);

  GRect fr_ch = (GRect) {
    .origin = {fr.origin.x + fr.size.w / 2 - ci_bounds.size.w / 2, (top_layer_bounds.size.h - ci_bounds.size.h)/2},
    .size = ci_bounds.size
  };
  charging_layer = bitmap_layer_create(fr_ch);
  bitmap_layer_set_bitmap(charging_layer, charging_image);
  layer_add_child(top_layer, bitmap_layer_get_layer(charging_layer));

  update_battery(battery_state_service_peek());

  battery_state_service_subscribe(update_battery);
}


static void battery_layer_update_proc(Layer *layer, GContext *ctx) {
  //APP_LOG(APP_LOG_LEVEL_DEBUG, "battery_layer_update %d", battery_percent);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, (GRect) { .origin = {2,2}, .size = {11*battery_percent/100.0, 4}}, 0, GCornerNone);
}


static void update_battery(BatteryChargeState charge_state) {

  battery_percent = charge_state.charge_percent;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "update_battery %d", battery_percent);
  layer_mark_dirty(bitmap_layer_get_layer(battery_layer));
  layer_set_hidden(bitmap_layer_get_layer(charging_layer), !charge_state.is_charging);
}



// -------------- Bluetooth --------------------

void bluetooth_layer_init() {

    // // Bluetooth icon
  bluetooth_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BLUETOOTH);
  GRect bi_bounds = gbitmap_get_bounds(bluetooth_image);

  GRect battery_layer_frame = layer_get_frame(bitmap_layer_get_layer(battery_layer));
  APP_LOG(APP_LOG_LEVEL_INFO, "battery_layer_frame.origin.x %d", battery_layer_frame.origin.x);

  GRect fr_bluetooth = (GRect) {
    .origin = {battery_layer_frame.origin.x + battery_layer_frame.size.w / 2 - bi_bounds.size.w / 2,
               (battery_layer_frame.origin.y + battery_layer_frame.size.h + 4)},
    .size = bi_bounds.size
  };
  bluetooth_layer = bitmap_layer_create(fr_bluetooth);
  bitmap_layer_set_bitmap(bluetooth_layer, bluetooth_image);
  layer_add_child(top_layer, bitmap_layer_get_layer(bluetooth_layer));

  update_bluetooth(bluetooth_connection_service_peek());

  bluetooth_connection_service_subscribe(update_bluetooth);

  init_finished = true;
}


static void update_bluetooth(bool connected) {
  //APP_LOG(APP_LOG_LEVEL_DEBUG, "bluetooth update");
  if (init_finished && SETTINGS_BLUETOOTH_VIBE) {
    vibes_short_pulse();
  }
  layer_set_hidden(bitmap_layer_get_layer(bluetooth_layer), !connected);
}

//------------------- Time layers ----------------------

void hour_layer_init_pbl_rect(Layer *parent_layer) {
  GRect hour_frame = (GRect){
    .origin = {PBL_IF_COLOR_ELSE(-2, -1), PBL_IF_COLOR_ELSE(8, -12)},
    .size = {64, 66}
  };
  
  //GFont font54 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_64_BOLD));

#ifdef PBL_COLOR
  GFont font54 = fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS);
#else
  GFont font54 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_64_BOLD));
#endif
  hour_layer = text_layer_create(hour_frame);

  
  text_layer_set_font(hour_layer, font54);
  text_layer_set_text_color(hour_layer, COLOR_TIME);
  text_layer_set_background_color(hour_layer, GColorClear);
  text_layer_set_text_alignment(hour_layer, GTextAlignmentRight);

  layer_add_child(parent_layer, text_layer_get_layer(hour_layer));


  //date layer
  GRect date_frame = (GRect){
    .origin = {hour_frame.origin.x + hour_frame.size.w, 2},
    .size = {72 + 10, 20 + 3} // +3 for 'g' symbol, + 10 for 'Mon'
  };
  date_layer = text_layer_create(date_frame);

  text_layer_set_font(date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_color(date_layer, COLOR_DATE);
  text_layer_set_background_color(date_layer, GColorClear);

  layer_add_child(parent_layer, text_layer_get_layer(date_layer));


  //minutes
  GRect minute_frame = (GRect){
    .origin = {date_frame.origin.x + 4, date_frame.origin.y + date_frame.size.h - 8},
    .size = {54, 36}
  };
  minute_layer = text_layer_create(minute_frame);

  //text_layer_set_font(minute_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_34_BOLD)));
#if defined(PBL_COLOR)
  text_layer_set_font(minute_layer, fonts_get_system_font(FONT_KEY_LECO_32_BOLD_NUMBERS));
#else
  text_layer_set_font(minute_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_34_BOLD)));
#endif

  text_layer_set_text_color(minute_layer, COLOR_TIME);
  text_layer_set_background_color(minute_layer, GColorClear);

  layer_add_child(parent_layer, text_layer_get_layer(minute_layer));



  // Init value of time_layer
  time_t now = time(NULL);
  struct tm *current_time = localtime(&now);
  handle_tick(current_time, SECOND_UNIT);
}

void hour_layer_init_pbl_round(Layer *parent_layer) {
  GRect bounds = layer_get_bounds(parent_layer);

  int8_t height = 20;
  GFont font = fonts_get_system_font(PBL_IF_COLOR_ELSE(FONT_KEY_LECO_20_BOLD_NUMBERS, FONT_KEY_GOTHIC_18_BOLD));

  GRect hour_frame = (GRect){
    .origin = {0, bounds.size.h / 2 - height / 2},
    .size = {bounds.size.w, height}
  };
  

  hour_layer = text_layer_create(hour_frame);
  text_layer_set_font(hour_layer, font);
  text_layer_set_text_color(hour_layer, COLOR_TIME);
  text_layer_set_text_alignment(hour_layer, GTextAlignmentCenter);
  text_layer_set_background_color(hour_layer, GColorClear);

  GRect ampm_frame = (GRect){
    .origin = {0, bounds.size.h / 2 - height / 2 + 2}, //X does not matter here
    .size = {10, height}
  };

  ampm_layer = text_layer_create(ampm_frame);
  text_layer_set_font(ampm_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_color(ampm_layer, COLOR_TIME);
  text_layer_set_text_alignment(ampm_layer, GTextAlignmentRight);
  text_layer_set_background_color(ampm_layer, COLOR_BACKGROUND_TIME);
  text_layer_set_overflow_mode(ampm_layer, GTextOverflowModeWordWrap);

  layer_add_child(parent_layer, text_layer_get_layer(hour_layer));
  // layer_add_child(parent_layer, text_layer_get_layer(ampm_layer));
}

void date_layer_init_pbl_round(Layer *parent_layer) {
  GRect bounds = layer_get_bounds(parent_layer);

  int8_t height = 18;

  GRect date_frame = (GRect){
    .origin = {0, bounds.size.h / 2 - height / 2 - 3},
    .size = {bounds.size.w, height}
  };

  date_layer = text_layer_create(date_frame);
  text_layer_set_font(date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_color(date_layer, GColorWhite);
  text_layer_set_text_alignment(date_layer, GTextAlignmentCenter);
  text_layer_set_background_color(date_layer, GColorClear);

  layer_add_child(parent_layer, text_layer_get_layer(date_layer));
}

/** Extract hour value from tm structure based on clock format. This eliminates leading zero. */
int8_t hours_from_tm(struct tm *tick_time) {
  int8_t hours = -1;
  if (clock_is_24h_style()) {
    hours = tick_time->tm_hour;
  } else {
    hours = tick_time->tm_hour % 12;
    if (hours == 0) {
      hours = 12;
    }
  }
  return hours;
}

void handle_tick_pbl_rect(struct tm *tick_time) {

  static char hour_text[] = "00";
  static char min_text[] = ": 00";
  static char date_text[] = "           "; // 11 chars for long labels "Aug 11, Mon"
  //APP_LOG(APP_LOG_LEVEL_DEBUG, time_text);

  int8_t hours = hours_from_tm(tick_time);
  
  snprintf(hour_text, sizeof(hour_text), "%d", hours);

  strftime(min_text, sizeof(min_text), ":%M", tick_time); // %H:%M:%S
  strftime(date_text, sizeof(date_text), "%b %d, %a", tick_time); // %H:%M:%S
  
  text_layer_set_text(hour_layer, hour_text);
  text_layer_set_text(minute_layer, min_text);
  text_layer_set_text(date_layer, date_text);

  APP_LOG(APP_LOG_LEVEL_DEBUG, "handle_tick: hour = %s, min = %s, date = %s", hour_text, min_text, date_text);
}

void handle_tick_pbl_round(struct tm *tick_time) {
  static char time_text[] = "00:00 a";
  static char date_text[] = "           "; // 11 chars for long labels "Aug 11, Mon"
  static char ampm_text[] = "1 ";

  if (clock_is_24h_style()) {
    strftime(time_text, sizeof(time_text), "%H:%M", tick_time); // %H:%M:%S
    layer_set_hidden(text_layer_get_layer(ampm_layer), true);
  } else {
    strftime(time_text, sizeof(time_text), "%I:%M", tick_time); // 1 is just a placholder text
    
    strftime(ampm_text, sizeof(ampm_text), "%p", tick_time);
    ampm_text[1] = '\0';
    layer_set_hidden(text_layer_get_layer(ampm_layer), false);
    text_layer_set_text(ampm_layer, ampm_text);
  }
  strftime(date_text, sizeof(date_text), "%b %d, %a", tick_time); // %H:%M:%S

  text_layer_set_text(hour_layer, (time_text[0] == '0') ? &time_text[1] : time_text); //trim leading zero in time
  text_layer_set_text(date_layer, date_text);


//update framme for 12h format
  if (!clock_is_24h_style()) {
    GRect hour_frame = layer_get_bounds(text_layer_get_layer(hour_layer));
    GSize hour_csize = text_layer_get_content_size(hour_layer);
    GRect ampm_bounds = layer_get_frame(text_layer_get_layer(ampm_layer));
    GRect ampm_frame = (GRect){
      .origin = {hour_frame.size.w / 2 + hour_csize.w / 2 - ampm_bounds.size.w, ampm_bounds.origin.y},
      .size = {ampm_bounds.size.w, ampm_bounds.size.h}
    };

    layer_set_frame(text_layer_get_layer(ampm_layer), ampm_frame);
  }

}

// TODO: Needs to be changed to minute_tick
static void handle_tick(struct tm *tick_time, TimeUnits units_changed) {

  #ifdef PBL_ROUND
    handle_tick_pbl_round(tick_time);
  #else
    handle_tick_pbl_rect(tick_time);
  #endif


  // Get Size of occupied text
  /*
  GSize text_size = text_layer_get_content_size(time_layer);
  text_layer_set_size(time_layer, text_size);
  layer_set_frame(text_layer_get_layer(time_layer), GRect((144-text_size.w)/2, -4, text_size.w, text_size.h));
  */
  //APP_LOG(APP_LOG_LEVEL_DEBUG, "text_size = {%d, %d}", text_size.w, text_size.h);
}


void clock_layer_deinit() {

  unregister_tick_handler(handle_tick);
  battery_state_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();

  layer_destroy(top_layer);
  layer_destroy(bottom_layer);

#ifndef PBL_ROUND
  bitmap_layer_destroy(battery_image_layer);
  bitmap_layer_destroy(battery_layer);
  gbitmap_destroy(battery_image);

  bitmap_layer_destroy(charging_layer);
  gbitmap_destroy(charging_image);

  bitmap_layer_destroy(bluetooth_layer);
  gbitmap_destroy(bluetooth_image);

  text_layer_destroy(minute_layer);
#else
  text_layer_destroy(ampm_layer);
#endif

  text_layer_destroy(hour_layer);
  text_layer_destroy(date_layer);
}