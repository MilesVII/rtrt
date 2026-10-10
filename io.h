#pragma once

#include <stdbool.h>
#include "utils.h"

#define IO_KEYSCOUNT 8
enum Key {
	IO_W,
	IO_A,
	IO_S,
	IO_D,
	IO_Q,
	IO_E,
	IO_Z,
	IO_NO
};
typedef enum Key Key;

extern FVector io_mouse;
extern FVector io_mouseDelta;
extern FVector io_touchDownStored;
extern bool io_touchDown;
extern bool io_touchUp;
extern bool io_touch;
extern bool io_shortTouch;
extern bool io_longTouch;
extern bool io_quit;
extern int io_scroll;
extern float io_longTouchProgress;
extern bool io_keyPressed[IO_KEYSCOUNT];
extern Rect io_screenRect;

void io_init();
void io_update(float dt, float screenHeight);
float io_deltaTime();
bool io_skipFrame();

void io_onResize(int width, int height);