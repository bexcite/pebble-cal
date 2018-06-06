#include <pebble.h>
#include "event_layer.h"

#define PT_PER_SPACE 0.35

#define UPCOMING_EVENT_TIME_DIFFERENCE 10 * 60
#define WEEK_TIME_DIFFERENCE 60 * 60 * 24 * 7



#if defined(PBL_COLOR)
  #define COLOR_EVENT_TIME GColorDukeBlue
#elif defined(PBL_BW)
  #define COLOR_EVENT_TIME GColorBlack
#endif

struct EventLayer {
	uint8_t type;
	Layer *base_layer;

  TextLayer *time_layer;
  char time_text[30];

  char summary_text[100];
  TextLayer *summary_layer1;

  GBitmap *excl_mark_image;
  BitmapLayer *excl_mark_image_layer;

};

// ------------------------ PRIVATE DECLARATIONS ------------
void text_layer_default_update_callback(Layer *me, GContext *ctx);
void text_layer_empty_update_callback(Layer *me, GContext *ctx);
void format_time_str(char* line, size_t line_size, uint8_t type, time_t start_time, time_t end_time);
time_t get_UTC_offset(struct tm *t);

// ----------------------------------------------------------
// ------------------------ PUBLIC --------------------------
// ----------------------------------------------------------
/**
@base_layer - pointer to text layer
@param num - number of text layer. Starts from 0
@param parent_size - size of parent layer. Usefull for adjusting to different device sizes.
*/
EventLayer* event_layer_init(uint8_t num, GSize parent_size) {
  Layer *base_layer;

  int8_t event_height = parent_size.h / 3;

  GRect fr_mt = (GRect) {
    .origin = {0, num * event_height}, // top layer  shift 
    .size = {parent_size.w, event_height}
  };
  base_layer = layer_create(fr_mt);


  // Draw line here
  layer_set_update_proc(base_layer, text_layer_default_update_callback);

  int8_t xpad = 2;
  int8_t incell_pad = -3; //verticall shift to fit text into tight cell. For RECT PBL

//time layer creation
  GRect text_frame = (GRect){
    .origin = {xpad, PBL_IF_RECT_ELSE(incell_pad, 0)},
    .size = {PBL_IF_RECT_ELSE(144, parent_size.w), event_height / 2}
  };
  TextLayer *time_layer = text_layer_create(text_frame);
  // text_layer_set_background_color(time_layer, PBL_IF_COLOR_ELSE(GColorRed, GColorWhite));
  text_layer_set_text_color(time_layer, COLOR_EVENT_TIME);
  text_layer_set_font(time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(time_layer, PBL_IF_RECT_ELSE(GTextAlignmentLeft, GTextAlignmentCenter));
  text_layer_set_text(time_layer, "Loading ...");
  //layer_set_clips(text_layer_get_layer(time_layer), false);


  //summary text layers
  // here comes a trick. We display summary with 2 layers with shifted bounds (for rect screens)
  GRect sframe = (GRect){
    .origin = {xpad, PBL_IF_RECT_ELSE(incell_pad, text_frame.origin.y + text_frame.size.h - 3)},
    .size = {parent_size.w - xpad * 2, PBL_IF_RECT_ELSE(event_height + 2, event_height / 2 + 2)}
  };

  TextLayer *summary_layer1 = text_layer_create(sframe);

  //exclamation mark for upcoming events (in X min)

  GBitmap *excl_mark_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_EXCL_MARK);
  GRect image_bounds = gbitmap_get_bounds(excl_mark_image);
  GRect fr = (GRect) {
    .origin = {2, 2},
    .size = image_bounds.size
  };
  BitmapLayer *excl_mark_image_layer = bitmap_layer_create(fr);

  bitmap_layer_set_bitmap(excl_mark_image_layer, excl_mark_image);
  layer_set_hidden(bitmap_layer_get_layer(excl_mark_image_layer), true);

  //END excl mark

  // Test Cyrillic font [down button for other variants]
  // TODO: Check the right way to clean memory after working with fonts. (unload etc, )
  // Documentation says that system unload fonts automatically 
  // http://developer.getpebble.com/2/api-reference/group___fonts.html#gae25249ea28fddf938ef900efdcf1777c
  //text_layer_set_font(summary_layer2, fonts_get_system_font(FONT_KEY_GOTHIC_18));

  // Dec 13, 2015 -- Pavlo
  // Removed custom font due to bugs with ~color images. RIP russian and 
  // better to ask users to install Languages Packs from
  // https://github.com/MarSoft/pebble-firmware-utils/wiki/Language-Packs
  // GFont font35 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_35)); //RESOURCE_ID_FONT_35 - lighter version
  text_layer_set_font(summary_layer1, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  // text_layer_set_font(summary_layer1, font35);

  text_layer_set_text_color(summary_layer1, GColorBlack);
  text_layer_set_text_alignment(summary_layer1, PBL_IF_RECT_ELSE(GTextAlignmentLeft, GTextAlignmentCenter));
  
  layer_add_child(base_layer, text_layer_get_layer(summary_layer1));
  layer_add_child(base_layer, text_layer_get_layer(time_layer));
  layer_add_child(base_layer, bitmap_layer_get_layer(excl_mark_image_layer));


  EventLayer *layer = (EventLayer *)malloc(sizeof(EventLayer));
  layer->type = EVENT_TYPE_DEFAULT;
  layer->base_layer = base_layer;
  layer->time_layer = time_layer;
  layer->summary_layer1 = summary_layer1;
  layer->excl_mark_image_layer = excl_mark_image_layer;
  layer->excl_mark_image = excl_mark_image;
  return layer;
}

void event_layer_set_summary(EventLayer *layer, const char *summary) {
  GRect time_rect = layer_get_frame(text_layer_get_layer(layer->time_layer));
  // APP_LOG(APP_LOG_LEVEL_DEBUG, "Heights of event=%d", layer_get_frame(layer->base_layer).size.h); 

  strcpy(layer->summary_text, summary);

  bool is_mark_visible = !layer_get_hidden(bitmap_layer_get_layer(layer->excl_mark_image_layer));
  GSize mark_size = layer_get_frame(bitmap_layer_get_layer(layer->excl_mark_image_layer)).size;
//might require updating this thing on redraw, not needed so far
  // this is a hack way to align 2 text layers on square screen
  // as we have to lines in summary but would like to have simple text calculations 
  // just space the first line with spaces (simple)
#ifndef PBL_ROUND
  snprintf(layer->summary_text, sizeof(layer->summary_text),"%.*s%s ",
    (int)((time_rect.size.w + (is_mark_visible ? mark_size.w + 2 : 0)) * PT_PER_SPACE),
    "                                                                                                                                            ",
      summary);
#endif

  // APP_LOG(APP_LOG_LEVEL_DEBUG, "Number=%d", (int)((time_rect.size.w + (is_mark_visible ? mark_size.w + 2 : 0)) * PT_PER_SPACE));
  // APP_LOG(APP_LOG_LEVEL_DEBUG, "Drawing= %.*s%s ", (int)((time_rect.size.w + (is_mark_visible ? mark_size.w + 2 : 0)) * PT_PER_SPACE),
  //   "                                                                                                                                            ",
  //   summary
  // );

  text_layer_set_text(layer->summary_layer1, layer->summary_text);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Summary Text=%s", layer->summary_text);


#ifdef PBL_ROUND
  text_layer_enable_screen_text_flow_and_paging(layer->summary_layer1, 2);
#endif

  // APP_LOG(APP_LOG_LEVEL_DEBUG, "%d   \"%s\"", (int)(time_rect.size.w * PT_PER_SPACE), layer->summary_text);                                                   
}

void event_layer_set_time(EventLayer *layer, time_t start_time, time_t end_time) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "setting time");

  char line[22];

  // Format time
  format_time_str(line, sizeof(line), layer->type, start_time, end_time);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "format_time_str = %s", line);

  ////////////////////////////////////////////////
/*
    struct tm * t_time;

  // >>> Debug block

  char dbg_line[] = "XXX, 01-01-2015 00:00";

  // Print Start Time
  t_time = gmtime(&start_time);
  strftime(dbg_line, sizeof(dbg_line), "%a, %F %H:%M", t_time);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "START_TIME = %s", dbg_line);

  // Print End Time
  t_time = gmtime(&end_time);
  strftime(dbg_line, sizeof(dbg_line), "%a, %F %H:%M", t_time);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "END_TIME = %s", dbg_line);

  // <<< end debug block

  time_t now1 = time(NULL) + get_UTC_offset(NULL);
  time_t diff = start_time - now1;

  // >>> Debug block

  t_time = gmtime(&now1);
  strftime(dbg_line, sizeof(dbg_line), "%a, %F %H:%M", t_time);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "NOW = %s, diff=%lu", dbg_line, diff);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "UPCOMING_EVENT_TIME_DIFFERENCE = %d", UPCOMING_EVENT_TIME_DIFFERENCE);

  // <<< end debug block
*/
  /////////////////////////////////////////////


  // Get Base Frame
  GRect base_frame = layer_get_frame(layer->base_layer);

  //visibility of excl mark

  time_t now = time(NULL);

  
  bool is_mark_visible = start_time > now && start_time - now <= UPCOMING_EVENT_TIME_DIFFERENCE && end_time >= now && layer->type == EVENT_TYPE_DEFAULT;
  // bool is_mark_visible = true;


  Layer *bitmap_layer = bitmap_layer_get_layer(layer->excl_mark_image_layer);
  
  layer_set_hidden(bitmap_layer, !is_mark_visible);


  // Get prev frame
  GRect old_time_frame = layer_get_frame(text_layer_get_layer(layer->time_layer));
  // APP_LOG(APP_LOG_LEVEL_DEBUG, "old_time_frame=(orig.x=%d, orig.y=%d), (size.w=%d, size.h=%d)",
  //   old_time_frame.origin.x, old_time_frame.origin.y, old_time_frame.size.w, old_time_frame.size.h);

  // Get mark frame
  GRect mark_frame = layer_get_frame(bitmap_layer);
  // APP_LOG(APP_LOG_LEVEL_DEBUG, "mark_frame: .x = %d, .y = %d, .w = %d, .h = %d",
  //   mark_frame.origin.x, mark_frame.origin.y, mark_frame.size.w, mark_frame.size.h);


  //Make time frame big
  int base_width = layer_get_bounds(layer->base_layer).size.w;
  GRect big_time_frame = (GRect){
      .origin = {(is_mark_visible ? mark_frame.origin.x + mark_frame.size.w : 0) + 2, old_time_frame.origin.y},
      .size = (GSize){base_width, old_time_frame.size.h}
  };
  layer_set_frame(text_layer_get_layer(layer->time_layer), big_time_frame);


  GSize size = text_layer_get_content_size(layer->time_layer);
  // APP_LOG(APP_LOG_LEVEL_DEBUG, "time_content_size (before): .w = %d, .h = %d", size.w, size.h);

  // Set new text and adjust size
  strcpy(layer->time_text, line);
  text_layer_set_text(layer->time_layer, layer->time_text);

  size = text_layer_get_content_size(layer->time_layer);
  // APP_LOG(APP_LOG_LEVEL_DEBUG, "time_content_size (after) : .w = %d, .h = %d", size.w, size.h);

  GSize new_size_for_rect = (GSize){size.w, size.h + 5};
  //text_layer_set_size(layer->time_layer, PBL_IF_RECT_ELSE(new_size_for_rect, old_time_frame.size)); //old_time_frame.size

  // Adjust frame taking in account mark visibility
  
  
  GSize frame_size = (GSize){size.w, size.h}; //new_size_for_frame, old_time_frame.size

#if defined(PBL_ROUND)
  GPoint frame_point = (GPoint){
    (base_frame.size.w - size.w + (is_mark_visible ? mark_frame.size.w + 2 : 0)) / 2, // x
    old_time_frame.origin.y// y
  };
#else
  GPoint frame_point = (GPoint){
    (is_mark_visible ? mark_frame.origin.x + mark_frame.size.w : 0) + 2, // x
    old_time_frame.origin.y
  }; //y
#endif  

  GRect new_time_frame = (GRect){
      .origin = frame_point, //{(is_mark_visible ? mark_frame.origin.x + mark_frame.size.w : 0) + 2, old_time_frame.origin.y}
      .size = frame_size //PBL_IF_RECT_ELSE((GSize){size.w, size.h}, old_time_frame.size)
  };
  // APP_LOG(APP_LOG_LEVEL_DEBUG, "new_time_frame=(orig.x=%d, orig.y=%d), (size.w=%d, size.h=%d)",
  //   new_time_frame.origin.x, new_time_frame.origin.y, new_time_frame.size.w, new_time_frame.size.h);
  layer_set_frame(text_layer_get_layer(layer->time_layer), new_time_frame);


#if defined(PBL_ROUND)
  int16_t mark_pos = (base_frame.size.w - size.w - mark_frame.size.w) / 2 - 2;
  GRect new_mark_frame = (GRect){
    .origin = (GPoint){mark_pos, 4},
    .size = mark_frame.size
  };
  layer_set_frame(bitmap_layer, new_mark_frame);
#endif


/*
  GRect old_time_frame = layer_get_frame(text_layer_get_layer(layer->time_layer));
  GRect mark_frame = layer_get_frame(bitmap_layer);
  GRect new_time_frame = (GRect){
      .origin = {(is_mark_visible ? mark_frame.origin.x + mark_frame.size.w : 0) + 2, old_time_frame.origin.y},
      .size = old_time_frame.size
  };
  layer_set_frame(text_layer_get_layer(layer->time_layer), new_time_frame);
*/
  // ----

/*  
  strcpy(layer->time_text, line);
  //APP_LOG(APP_LOG_LEVEL_DEBUG, "Time text: %s", line);

  // Next line was commented to fix bug with not resizing time label on changes to Now, 8 min, Ended states
  // text_layer_set_size(layer->time_layer, (GSize){old_time_frame.size.w, 20});


  text_layer_set_text(layer->time_layer, layer->time_text);

  GSize size = text_layer_get_content_size(layer->time_layer);
  GSize new_size_for_rect = (GSize){size.w, size.h + 5};
  text_layer_set_size(layer->time_layer, PBL_IF_RECT_ELSE(new_size_for_rect, new_size_for_rect)); //old_time_frame.size
*/
}

void event_layer_mark_dirty(EventLayer *layer) {
  layer_mark_dirty(text_layer_get_layer(layer->time_layer));
  layer_mark_dirty(layer->base_layer);
  layer_mark_dirty(text_layer_get_layer(layer->summary_layer1));
}

/*
 * Return time format string depending on user preferred time format 12h/24h.
 * 12h = I:M
 * 24h = H:M
 */
char* user_time_format_string_for_event() {
  if (clock_is_24h_style()) {
    return "%H:%M";
  } else {
    return "%I:%M %p";
  }
}

/* 
 *  Forms a string for the time.
 *  This day event in format: %H:%M
 *  Other day event in format: %a %H:%M
 *  < 10 min: In %u min. 
 *  currently happenning: Now
 *  All day event: %a
 * Params:
 *   line - is a destination
 */
void format_time_str(char *line, size_t line_size, uint8_t type, time_t start_time, time_t end_time) {
  //char line[20];
  char start[] = "XXX 00:00 AM";
  char end[] = "00:00"; //carefull, this length is good enough to strip AM/PM letters

  APP_LOG(APP_LOG_LEVEL_DEBUG, "format_time_srt CALL");

  struct tm * t_time;

  // >>> Debug block
/*
  char dbg_line[] = "XXX, 01-01-2015 00:00";

  // Print Start Time
  t_time = gmtime(&start_time);
  strftime(dbg_line, sizeof(dbg_line), "%a, %F %H:%M", t_time);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "START_TIME = %s", dbg_line);

  // Print End Time
  t_time = gmtime(&end_time);
  strftime(dbg_line, sizeof(dbg_line), "%a, %F %H:%M", t_time);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "END_TIME = %s", dbg_line);
*/
  // <<< end debug block

  //time_t now = time(NULL); // need to have localtime here
  time_t now = time(NULL) + get_UTC_offset(NULL);
  time_t diff = start_time - now;

  // >>> Debug block
/*
  t_time = gmtime(&now);
  strftime(dbg_line, sizeof(dbg_line), "%a, %F %H:%M", t_time);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "NOW = %s, diff=%lu", dbg_line, diff);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "UPCOMING_EVENT_TIME_DIFFERENCE = %d", UPCOMING_EVENT_TIME_DIFFERENCE);
*/
  // <<< end debug block

  if ((diff > 0) &&  (diff < UPCOMING_EVENT_TIME_DIFFERENCE) && (type == EVENT_TYPE_DEFAULT)) {
    // Event will be in 0 - 10 min
    // APP_LOG(APP_LOG_LEVEL_DEBUG, "Event will be in 0 - 10 min");
    diff = (int)((diff + 59)/ 60); // Division is here because there was a bug when diff = 0 (but actually about 0.0-0.9). 
    //should show 1 when >0 and 0 when 0. 

    // So we need compare in seconds and then devide
    if (diff == 0) {
      snprintf(line, line_size, "In <1 min. ");
    } else {
      snprintf(line, line_size, "In %u min. ", (unsigned int)diff);
    }
    return;
  }

  if (start_time <= now && end_time >= now) {  
    // Event is happening Now - "Now-11:00"
    if (type == EVENT_TYPE_DEFAULT) {
      snprintf(line, line_size, "Now");
      // Add end part
      t_time = gmtime(&end_time); // JS returns GMTime
      strftime(end, sizeof(end), user_time_format_string_for_event(), t_time);
      strcat(line, "-");
      strcat(line, end);
      return;
    }
  } 


  if (end_time <= now) {
    // Event is finished "11:00-Ended"
    if (type == EVENT_TYPE_DEFAULT) {
      // Add start part
      t_time = gmtime(&start_time); // JS returns GMTime
      strftime(line, line_size, user_time_format_string_for_event(), t_time);
      strcat(line, "-Ended");
      return;
    }
  }

  if (diff > WEEK_TIME_DIFFERENCE) {
    // Event is more than in a week Format: "Oct 12, Mon 11:00 AM"
    t_time = gmtime(&start_time); // JS returns GMTime
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "format_time: event more than in a week diff = %lu (week = %lu)", (uint32_t)diff, (uint32_t)WEEK_TIME_DIFFERENCE);
    strftime(start, sizeof(start), "%b %d, ", t_time);

    char start_part[] = "00:00 AM";
    strftime(start_part, sizeof(start_part), user_time_format_string_for_event(), t_time);

    strcpy(line, start);
    strcat(line, start_part);

    return;

  }

  // compare dates to figure out if this is this day or not
  
  char today_str[] = "yyMMdd";
  char start_date_str[] = "yyMMdd";
  strftime(today_str, sizeof(today_str), "%y%m%d", gmtime(&now)); // time returns GMTime
  strftime(start_date_str, sizeof(start_date_str), "%y%m%d", gmtime(&start_time)); // JS returns GMTime

  //APP_LOG(APP_LOG_LEVEL_DEBUG, "now = %s, start_date = %s", today_str, start_date_str);

  t_time = gmtime(&start_time); // JS returns GMTime
  if (strcmp(today_str, start_date_str)) {
    // other day

    // APP_LOG(APP_LOG_LEVEL_DEBUG, "format_time: other day diff = %lu", (uint32_t)diff);

    if (type == EVENT_TYPE_ALL_DAY) {
      strftime(line, line_size, "%a ", t_time); // Anna's design decision. Dec 12, 2015
      return;
    }

    strftime(start, sizeof(start), "%a ", t_time);

    char start_part[] = "00:00 AM";
    strftime(start_part, sizeof(start_part), user_time_format_string_for_event(), t_time);

    strcat(start, start_part);
    
  } else {
    //same day

    if (type == EVENT_TYPE_ALL_DAY) {
      snprintf(line, line_size, "Today "); // Anna's design decision. Dec 12, 2015
      return;
    }

    strftime(start, sizeof(start), user_time_format_string_for_event(), t_time);
  }
  
  strcpy(line, start);

  // Add end part
  t_time = gmtime(&end_time); // JS returns GMTime
  strftime(end, sizeof(end), user_time_format_string_for_event(), t_time);
  strcat(line, "-");
  strcat(line, end);

}

// Timezones strange (Aplite vs Basalt & Round)
// Borrowed from here https://forums.getpebble.com/discussion/comment/150513#Comment_150513
// Gets the UTC offset of the local time in seconds 
// (pass in an existing localtime struct tm to save creating another one, or else pass NULL)
time_t get_UTC_offset(struct tm *t) {

#ifdef PBL_SDK_3
  if (t == NULL) {
    time_t temp;
    temp = time(NULL);
    t = localtime(&temp);
  }
  
  return t->tm_gmtoff + ((t->tm_isdst > 0) ? 3600 : 0);
#else
  // SDK2 uses localtime instead of UTC for all time functions so always return 0
  return 0; 
#endif 
}


Layer * event_layer_get_layer(EventLayer *event_layer) {
  return event_layer->base_layer;
}

void event_layer_set_type(EventLayer *event_layer, EventType type) {
  event_layer->type = type;
  if (type == EVENT_TYPE_DEFAULT || type == EVENT_TYPE_ALL_DAY) {
    layer_set_hidden(text_layer_get_layer(event_layer->time_layer), false);
    layer_set_hidden(text_layer_get_layer(event_layer->summary_layer1), false);
    layer_set_update_proc(event_layer->base_layer, text_layer_default_update_callback);
  } else {
    layer_set_hidden(text_layer_get_layer(event_layer->time_layer), true);
    layer_set_hidden(text_layer_get_layer(event_layer->summary_layer1), true);
    layer_set_hidden(bitmap_layer_get_layer(event_layer->excl_mark_image_layer), true);
    layer_set_update_proc(event_layer->base_layer, text_layer_empty_update_callback);
  }
}

void event_layer_update_font(EventLayer *event_layer, GFont font) {
  text_layer_set_font(event_layer->summary_layer1, font);
}


void event_layer_destroy(EventLayer *layer) {
  layer_destroy(layer->base_layer);
  text_layer_destroy(layer->time_layer);
  text_layer_destroy(layer->summary_layer1);

  gbitmap_destroy(layer->excl_mark_image);
  bitmap_layer_destroy(layer->excl_mark_image_layer);

  // layer->excl_mark_image_layer = excl_mark_image_layer;
  // layer->excl_mark_image = excl_mark_image;

  free(layer);
}

// ----------------------------------------------------------
// ------------------------ PRIVATE -------------------------
// ----------------------------------------------------------
#define CELL_PADDING 4


void text_layer_draw_base(Layer *me, GContext *ctx) {
  GRect layer_frame = layer_get_frame(me);

  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, (GRect){.origin = {0, 0}, .size = {layer_frame.size.w, layer_frame.size.h}}, 0, GCornerNone);

  graphics_context_set_stroke_color(ctx, GColorBlack);
  // graphics_draw_line(ctx, GPoint(0, layer_frame.size.h - 1), GPoint(144, layer_frame.size.h - 1));
  
}

void text_layer_default_update_callback(Layer *me, GContext *ctx) {
  text_layer_draw_base(me, ctx);
  //APP_LOG(APP_LOG_LEVEL_DEBUG, "--- > Default Update Callbacl *base_layer");

  // --- draw pixel line
  /*
  GRect layer_frame = layer_get_frame(me);
  int lpad = 5;
  int pad = CELL_PADDING;

  for (int i = lpad + 1; i < layer_frame.size.w - pad; i += 2) {
    graphics_draw_pixel(ctx, GPoint(i, layer_frame.size.h - 1));
  }
  */
}

void text_layer_empty_update_callback(Layer *me, GContext *ctx) {
  text_layer_draw_base(me, ctx);

  GRect layer_frame = layer_get_frame(me);

  for (int i = 0; i < layer_frame.size.h; ++i) {
    for (int j = (i % 2 ? 2 : 0); j < layer_frame.size.w; j += 3) {
      graphics_draw_pixel(ctx, GPoint(j, i));
    }
  }

}

