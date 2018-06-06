#include <pebble.h>
#include "event_details.h"

/*
 * =============================================================
 *    Implementation Variables
 * =============================================================
 */

static Window *window;

/*
 * =============================================================
 *    Implementation Functions
 * =============================================================
 */

static void window_load(Window *window) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Event_details Window Load");
}

static void window_unload(Window *window) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Event_details Window UnLoad");
}

static void window_appear(Window *window) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Event Details Window Appear");
}

static void window_disappear(Window *window) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Event Details Window DisAppear");
}


static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Event Details Select Click");
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Event Details Up Click");
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Event Details Down Click");
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}


static Window* event_details_create() {
  if (window == NULL) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Event Details Window Create");
    window = window_create();
    window_set_window_handlers(window, (WindowHandlers) {
      .load = window_load,
      .unload = window_unload,
      .appear = window_appear,
      .disappear = window_disappear
    });
    window_set_click_config_provider(window, &click_config_provider);
  }
  return window;
}

/*
 * =============================================================
 *    Interface Functions
 * =============================================================
 */

void event_details_show() {
  Window* edWindow = event_details_create();
  window_stack_push(edWindow, true);
}

void event_details_init() {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Event_details INIT");
}

void event_details_deinit() {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Event_details DEINIT");
  if (window != NULL) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Event_details Destroy Window");
    window_destroy(window);
  }
}