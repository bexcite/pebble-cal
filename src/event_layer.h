#ifndef EVENT_LAYER_H
#define EVENT_LAYER_H

#include <pebble.h>


typedef enum { 
  EVENT_TYPE_EMPTY = 0,
  EVENT_TYPE_DEFAULT = 1,
  EVENT_TYPE_ALL_DAY = 2

} EventType;

struct EventLayer;
typedef struct EventLayer EventLayer;


EventLayer* event_layer_init(uint8_t num, GSize parent_size);

void event_layer_set_summary(EventLayer *layer, const char *summary);
void event_layer_set_time(EventLayer *layer, time_t start_time, time_t end_time);

void event_layer_mark_dirty(EventLayer *layer);

Layer * event_layer_get_layer(EventLayer *event_layer);
void event_layer_set_type(EventLayer *event_layer, EventType type);

void event_layer_update_font(EventLayer *event_layer, GFont font);

void event_layer_destroy(EventLayer *layer);


#endif
