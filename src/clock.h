#ifndef CLOCK_H
#define CLOCK_H

#define CLOCK_LAYER_HEIGHT PBL_IF_RECT_ELSE(59, 26)
#define DATE_LAYER_HEIGHT  PBL_IF_RECT_ELSE(0, 27) //Since PBL_ROUND

#define SETTINGS_BLUETOOTH_VIBE 0

void clock_layer_init(Window* window);
void clock_layer_deinit();


#endif