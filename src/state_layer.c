#include "state_layer.h"
#include <pebble.h>

#define STATE_TEXT_LOADING "Loading events ..."
#define STATE_TEXT_AUTH_ERROR "Please authenticate \nGoogle Calendar \nin App Settings"

#if defined(PBL_COLOR)
  #define COLOR_BACKGROUND_MSG PBL_IF_RECT_ELSE(GColorDukeBlue, GColorDukeBlue)
  #define COLOR_MSG PBL_IF_RECT_ELSE(GColorWhite, GColorWhite)
#elif defined(PBL_BW)
  #define COLOR_BACKGROUND_MSG GColorBlack
  #define COLOR_MSG GColorWhite
#endif



struct StateLayer {
  Layer *base_layer;

  TextLayer *text_layer;


};


StateLayer* state_layer_init(GRect frame) {
  Layer *base_layer = layer_create(frame);

  int8_t text_font_size = 14;

  GRect text_frame = (GRect){
    .origin = {0, frame.size.h / 2 - text_font_size},
    .size = {frame.size.w, text_font_size * 2 + 8} //8 is just a magic number
  };
  TextLayer *text_layer = text_layer_create(text_frame);
  text_layer_set_text(text_layer, STATE_TEXT_LOADING);
  text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
  text_layer_set_font(text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_color(text_layer, COLOR_MSG);
  text_layer_set_background_color(text_layer, COLOR_BACKGROUND_MSG);

  layer_add_child(base_layer, text_layer_get_layer(text_layer));

  StateLayer *layer = (StateLayer *)malloc(sizeof(StateLayer));
  layer->base_layer = base_layer;
  layer->text_layer = text_layer;
  return layer;
}


Layer* state_layer_get_layer(StateLayer *layer) {
  return layer->base_layer;
}


void state_layer_set_hidden(StateLayer *layer, bool hidden) {
  layer_set_hidden(layer->base_layer, hidden);
}


void state_layer_set_state(StateLayer *layer, State state) {
  switch (state) {
    case STATE_LOADING:
      state_layer_set_text(layer, STATE_TEXT_LOADING);
      break;
    case STATE_AUTH_ERROR:
      state_layer_set_text(layer, STATE_TEXT_AUTH_ERROR);
      break;
    default:
      break;
      //do nothing
  }
  
}

void state_layer_set_text(StateLayer *layer, char *text) {

  text_layer_set_text(layer->text_layer, text);

  //adjust text layer position and size
  GRect frame = layer_get_frame(state_layer_get_layer(layer));
  // GRect text_frame = layer_get_frame(text_layer_get_layer(layer->text_layer));

  Window *window = window_stack_get_top_window();
  GRect win_frame = layer_get_bounds(window_get_root_layer(window));

  GRect constraint_rect = (GRect){
    .origin = {0, 0},
    .size = {win_frame.size.w, win_frame.size.h / 2} //divider is empiricaly selected number
  };
  int8_t bottom_padding = 5; //empiric number

  GSize size = graphics_text_layout_get_content_size(text_layer_get_text(layer->text_layer),
                  fonts_get_system_font(FONT_KEY_GOTHIC_18), constraint_rect, GTextOverflowModeWordWrap, GTextAlignmentCenter); 
  layer_set_frame(text_layer_get_layer(layer->text_layer), 
                  (GRect){.origin = {0, frame.size.h / 2 - size.h / 2}, .size = (GSize){win_frame.size.w, size.h + bottom_padding}});
}


void state_layer_destroy(StateLayer *layer) {
  layer_destroy(layer->base_layer);
  text_layer_destroy(layer->text_layer);
  free(layer);
}
