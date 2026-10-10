#pragma once

#include "utils.h"

enum Align {ALIGN_START, ALIGN_MIDDLE, ALIGN_END};
typedef enum Align Align;

typedef struct Texture Texture;

Vector drawing_init(char* title, float width, float height);
void drawing_resize(int width, int height);
Vector drawing_getScreenSize();
void drawing_clear(uint32_t color);

void drawing_enableFogOfWar(int worldWidth, int worldHeight);
void drawing_clearFogOfWar();
void drawing_addFogOfWarArea(FVector* origin, FVector* skirt, int count);
void drawing_drawFogOfWar(FRect* destination);
void drawing_disableFogOfWar();

Texture* drawing_createTexture(Bitmap* bmp);
void drawing_rewriteTexture(Bitmap* bmp, Texture* gfx);
void drawing_texture(Texture* gfx, FRect* destination);

void drawing_circle(FVector* position, float radius, uint32_t color);
void drawing_circleRaw(float x, float y, float radius, uint32_t color);
void drawing_rect(FRect* rect, uint32_t color);
void drawing_rectVV(FVector* position, FVector* size, uint32_t color);
void drawing_rectRaw(float minX, float minY, float maxX, float maxY, uint32_t color);
void drawing_triangle(FVector* a, FVector* b, FVector* c, uint32_t color);
void drawing_triangleRaw(float aX, float aY, float bX, float bY, float cX, float cY, uint32_t color);
void drawing_line(FVector* from, FVector* to, uint32_t color);
void drawing_lineRaw(float fromX, float fromY, float toX, float toY, uint32_t color);
void drawing_path(FVector* path, int pathNodes, uint32_t color);
void drawing_arc(FVector* center, float radius, float sectorRelative, float offsetRelative, uint32_t color);

Texture* drawing_generateTextTexture(char* text, uint32_t color, int widthLimit);
void drawing_text(Texture* gfx, float x, float y, Align halign, Align valign, FRect* clampingRegion);
void drawing_textExt(char* text, float x, float y, uint32_t color, int widthLimit, Align halign, Align valign, FRect* clampingRegion);

extern int drawing_sceneId;
#define SCENES_COUNT 4
void drawing_updateScene(int sceneId);
void drawing_sendControls(Vector3 cP, Vector3 cRx, Vector3 cRy, Vector3 cRz, float lC, int sC, int mE);

void drawing_update();

void drawing_terminate();