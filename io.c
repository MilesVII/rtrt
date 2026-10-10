#include <SDL2/SDL.h>

#include "io.h"

#include "utils.h"

SDL_Event event;

const float IO_TOUCH_ALLOWED_SHAKE2 = 10 * 10;
const float IO_LONG_TOUCH_TIME = .3;
const int IO_SHORT_TOUCH_TIMEOUT_MS = 420;

FVector io_mouse = {-1, -1};
FVector io_touchDownStored = {-1, -1};
FVector io_mouseDelta = {0, 0};
static FVector io_mousePrevious = {0, 0};
bool io_quit = false;
bool io_touchDown = false;
bool io_touchUp = false;
bool io_touch = false;
bool io_longTouch = false;
bool io_shortTouch = false;
int io_scroll = 0;
float io_longTouchProgress = -1;
uint64_t io_deltaTimeStored = 0;
uint64_t io_timeStampStored = 0;
uint32_t io_touchDownTimeStamp;
bool io_keyPressed[IO_KEYSCOUNT];
Rect io_screenRect;

const float IO_DT_MIN_MS = 16.6667;

void io_init(){
	io_deltaTimeStored = SDL_GetPerformanceCounter();
	for (int i = 0; i < IO_KEYSCOUNT; ++i)
		io_keyPressed[i] = false;
}

static Key io_getKey(SDL_Keysym sym){
	switch (sym.scancode){
		case (SDL_SCANCODE_W):
			return IO_W;
		case (SDL_SCANCODE_A):
			return IO_A;
		case (SDL_SCANCODE_S):
			return IO_S;
		case (SDL_SCANCODE_D):
			return IO_D;
		case (SDL_SCANCODE_E):
			return IO_E;
		case (SDL_SCANCODE_Q):
			return IO_Q;
		case (SDL_SCANCODE_Z):
			return IO_Z;
		default:
			return IO_NO;
	}
}

void io_update(float dt, float screenHeight){
	io_mousePrevious = io_mouse;
	int iX, iY;
	SDL_GetMouseState(&iX, &iY);
	io_mouse = fvec_create(iX, iY);
	io_mouse.y = screenHeight - io_mouse.y;
	io_mouseDelta = fvec_sub(&io_mouse, &io_mousePrevious);
	io_touchDown = false;
	io_touchUp = false;
	io_shortTouch = false;
	io_longTouch = false;
	io_scroll = 0;

	if (io_touch && io_longTouchProgress >= 0){
		io_longTouchProgress += dt / IO_LONG_TOUCH_TIME;
		if (io_longTouchProgress >= 1){
			io_longTouch = true;
			io_longTouchProgress = -1;
		}
	}

	while(SDL_PollEvent(&event)){
		Key k;
		switch (event.type){
		case SDL_QUIT:
			io_quit = true;
			break;
		case SDL_KEYDOWN:
			k = io_getKey(event.key.keysym);
			io_keyPressed[k] = true;
			break;
		case SDL_KEYUP:
			k = io_getKey(event.key.keysym);
			io_keyPressed[k] = false;
			break;
		case SDL_MOUSEBUTTONDOWN:
			io_touchDown = true;
			io_touch = true;
			io_touchDownStored = io_mouse;
			io_longTouchProgress = 0;
			io_touchDownTimeStamp = SDL_GetTicks();
			break;
		case SDL_MOUSEBUTTONUP:
			io_touchUp = true;
			io_touch = false;
			io_longTouchProgress = -1;

			FVector delta = fvec_sub(&io_mouse, &io_touchDownStored);
			uint32_t timeDelta = SDL_GetTicks() - io_touchDownTimeStamp;
			if (fvec_length2(&delta) < IO_TOUCH_ALLOWED_SHAKE2 && timeDelta < IO_SHORT_TOUCH_TIMEOUT_MS)
				io_shortTouch = true;
			break;
		case SDL_WINDOWEVENT:
			if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
				io_onResize(event.window.data1, event.window.data2);
			break;
		case SDL_MOUSEWHEEL:
			io_scroll = sign(event.wheel.y);
			break;
		default:
			break;
		}
	}

	if (io_longTouchProgress >= 0){
		FVector storedDelta = fvec_sub(&io_mouse, &io_touchDownStored);
		if (fvec_length2(&storedDelta) > IO_TOUCH_ALLOWED_SHAKE2)
			io_longTouchProgress = -1;
	}
}

float io_deltaTime(){
	uint64_t dt = SDL_GetPerformanceCounter() - io_deltaTimeStored;
	io_deltaTimeStored = SDL_GetPerformanceCounter();
	return (float)(dt / (float)SDL_GetPerformanceFrequency());
}

bool io_skipFrame(){
	uint64_t now = SDL_GetPerformanceCounter();
	double delta = (float)((now - io_timeStampStored) * 1000) / SDL_GetPerformanceFrequency();
	if (delta < IO_DT_MIN_MS)
		return true;
	else {
		io_timeStampStored = now;
		return false;
	}
}