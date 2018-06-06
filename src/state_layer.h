#ifndef STATE_LAYER_H
#define STATE_LAYER_H

#include <pebble.h>

struct StateLayer;
typedef struct StateLayer StateLayer;

typedef enum { 
  STATE_DEFAULT = 0,
  STATE_LOADING = 1,
  STATE_AUTH_ERROR = 2

} State;


StateLayer* state_layer_init(GRect frame);
Layer* state_layer_get_layer(StateLayer *layer);
void state_layer_set_hidden(StateLayer *layer, bool hidden);
void state_layer_set_state(StateLayer *layer, State state);
void state_layer_set_text(StateLayer *layer, char *text);

void state_layer_destroy();

#endif