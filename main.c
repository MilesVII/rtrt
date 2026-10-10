#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <time.h>

#include "drawing.h"
#include "utils.h"
#include "io.h"

void io_onResize(int w, int h){
	drawing_resize(w, h);
	io_screenRect.maxX = w - 1;
	io_screenRect.maxY = h - 1;
}

Vector3 cameraPosition, cameraRx, cameraRy, cameraRz;
FVector cameraRotation;
float sampleCount;
int monteEnabled;
const float SAMPLES_MIN = 10;

void cameraInit(){
	cameraPosition.x = 0;
	cameraPosition.y = 0;
	cameraPosition.z = 0;
	
	cameraRx.x = 1.0f;
	cameraRx.y = 0;
	cameraRx.z = 0;
	
	cameraRy.x = 0;
	cameraRy.y = 1.0f;
	cameraRy.z = 0;
	
	cameraRz.x = 0;
	cameraRz.y = 0;
	cameraRz.z = 1.0f;
	
	cameraRotation.x = 0;
	cameraRotation.y = 0;
	sampleCount = 10;
	monteEnabled = 0;
}

void camera(){
	float fAxis = 0;
	float sAxis = 0;
	float eAxis = 0;
	float step = .2f;
	float rotStep = .0064f;

	if (io_keyPressed[IO_W])
		fAxis += step;
	if (io_keyPressed[IO_S])
		fAxis -= step;
	if (io_keyPressed[IO_D])
		sAxis += step;
	if (io_keyPressed[IO_A])
		sAxis -= step;
	if (io_keyPressed[IO_E])
		eAxis += step;
	if (io_keyPressed[IO_Q])
		eAxis -= step;
	if (io_scroll != 0){
		if (sign(io_scroll) > 0)
			sampleCount *= 1.1;
		else
			sampleCount /= 1.1;
		sampleCount = max(SAMPLES_MIN, sampleCount);
	}
	cameraRotation.x += -io_mouseDelta.x * rotStep;
	cameraRotation.y += io_mouseDelta.y * rotStep;
	cameraRotation.y = clamp(cameraRotation.y, F_TAU * -.25f, F_TAU * .25f);
	float lightControl = io_mouse.y / (float)io_screenRect.maxY;
	if (io_touchDown){
		if (io_keyPressed[IO_Z]){
			++drawing_sceneId;
			drawing_sceneId %= SCENES_COUNT;
		} else {
			if (monteEnabled)
				monteEnabled = 0;
			else
				monteEnabled = 1;
		}
	}


	Vector3 direction = {-sin(cameraRotation.x) * cos(cameraRotation.y), cos(cameraRotation.x) * cos(cameraRotation.y), sin(cameraRotation.y)};
	Vector3 ceiling = {0, 0, 1.0f};

	cameraRy = vec3_normalize(direction);
	cameraRx = vec3_normalize(vec3_cross(cameraRy, ceiling));
	cameraRz = vec3_normalize(vec3_cross(cameraRx, cameraRy));
	cameraPosition = vec3_sum(cameraPosition, vec3_scl(cameraRx, sAxis)); 
	cameraPosition = vec3_sum(cameraPosition, vec3_scl(cameraRy, fAxis)); 
	cameraPosition = vec3_sum(cameraPosition, vec3_scl(cameraRz, eAxis)); 

	drawing_sendControls(cameraPosition, cameraRx, cameraRy, cameraRz, lightControl, round(sampleCount), monteEnabled);
}

int main(int argc, char* args[]) {
	srand(time(NULL));

	Vector windowSize = drawing_init("SNR", .75, .75);
	io_screenRect.minX = 0;
	io_screenRect.minY = 0;
	io_screenRect.maxX = windowSize.x - 1;
	io_screenRect.maxY = windowSize.y - 1;

	io_init();
	cameraInit();

	char fpscaption[256];

	while(!io_quit) {
		if (io_skipFrame())
			continue;
		float dt = io_deltaTime();

		io_update(dt, windowSize.y);

		FRect fScreen = frect_convert(&io_screenRect);
		drawing_rect(&fScreen, 0xFF00FFFF);

		sprintf(fpscaption, "%16s : %.2f\n%16s : %7.4f %7.4f %7.4f\n%16s : %7.4f, %7.4f, %7.4f\n%16s : %.1f", 
		                    "fps", 1.0 / dt, 
		                    "camera position", cameraPosition.x, cameraPosition.y, cameraPosition.z, 
		                    "camera ry", cameraRy.x, cameraRy.y, cameraRy.z, 
		                    "sample count", sampleCount);
		drawing_textExt(fpscaption, 20, 20, 0xFF0000FF, rect_width(&io_screenRect), ALIGN_START, ALIGN_END, &fScreen);
		
		camera();
		drawing_update();
	}
	drawing_terminate();
	return 0;
}
