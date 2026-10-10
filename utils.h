#pragma once

#include <stdint.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>

#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))
#define abs(a) (((a) < 0) ? (-(a)) : (a)) 
#define clamp(x, min, max) (((x) < (min)) ? (min) : (((x) > (max)) ? (max) : (x)))
#define sign(x) (((x) == 0) ? 0 : (((x) > 0) ? 1 : -1))
#define remap(x, fromMin, fromMax, toMin, toMax) (((x) - (fromMin)) / ((fromMax) - (fromMin)) * ((toMax) - (toMin)) + (toMin))
#define get(a, x, y, w) ((a) + (x) + (y) * (w))
#define getV(a, v, w) ((a) + ((v).x) + ((v).y) * (w))
#define F_TAU 6.28318530718

struct Rect {
	int minX;
	int minY;
	int maxX;
	int maxY;
};
typedef struct Rect Rect;

struct FRect {
	float minX;
	float minY;
	float maxX;
	float maxY;
};
typedef struct FRect FRect;

struct Vector {
	int x;
	int y;
};
typedef struct Vector Vector;

struct FVector {
	float x;
	float y;
};
typedef struct FVector FVector;

struct Vector3 {
	float x;
	float y;
	float z;
};
typedef struct Vector3 Vector3;

struct List {
	void* data;
	size_t sizeOfType;
	int reserved, minimalReserved;
	int length;
	bool doNotShrink;
};
typedef struct List List;

struct Bitset {
	uint8_t* data;
	int size;
};
typedef struct Bitset Bitset;

struct Bitmap {
	uint32_t* data;
	int w, h;
};
typedef struct Bitmap Bitmap;

uint32_t encodeColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
void decodeColorRGBA(uint32_t source, uint8_t* r, uint8_t* g, uint8_t* b, uint8_t* a);
void decodeColorRGB(uint32_t source, uint8_t* r, uint8_t* g, uint8_t* b);
uint8_t decodeColorR(uint32_t source);
uint8_t decodeColorG(uint32_t source);
uint8_t decodeColorB(uint32_t source);
uint8_t decodeColorA(uint32_t source);

Rect rect_create(int minX, int minY, int maxX, int maxY);
FRect frect_create(float minX, float minY, float maxX, float maxY);
void rect_set(Rect* rect, int minX, int minY, int maxX, int maxY);
void rect_clamp(Rect* inner, const Rect* outer);
void rect_move(Rect* rect, FVector* offset);
//bool rect_inside(FVector* position, Rect* rect);
int rect_width(Rect* rect);
int rect_height(Rect* rect);
float frect_width(FRect* rect);
float frect_height(FRect* rect);
FRect frect_project(FRect* src, FRect* dst);
//bool frect_inside(FVector* position, FRect* rect);
FRect frect_convert(Rect* r);
#define rect_inside(position, rect) ((position).x >= (rect).minX && \
                                     (position).x <= (rect).maxX && \
                                     (position).y >= (rect).minY && \
                                     (position).y <= (rect).maxY)
#define frect_inside(position, rect) rect_inside((position), (rect))
void frect_clamp(Rect* inner, const Rect* outer);

Vector vec_create(int x, int y);
Vector vec_convert(const FVector* v);
Vector vec_add(const Vector* a, const Vector* b);
Vector vec_sub(const Vector* a, const Vector* b);
Vector vec_mul(const Vector* a, const Vector* b);
Vector vec_scl(const Vector* a, float x);
Vector vec_div(const Vector* a, const Vector* b);
float vec_length2(const Vector* v);
float vec_distance2(const Vector* a, const Vector* b);
float vec_angleOrigin(const Vector* origin,const Vector* direction);
FVector fvec_create(float x, float y);
FVector fvec_convert(const Vector* v);
FVector fvec_add(const FVector* a, const FVector* b);
FVector fvec_sub(const FVector* a, const FVector* b);
FVector fvec_mul(const FVector* a, const FVector* b);
FVector fvec_scl(const FVector* a, float x);
FVector fvec_div(const FVector* a, const FVector* b);
/* Radians, rotation is CCW */
FVector fvec_rotate(const FVector* a, float angle);
FVector fvec_norm(const FVector* a);
float fvec_length2(const FVector* v);
float fvec_distance2(const FVector* a, const FVector* b);
float fvec_angleOrigin(const FVector* origin,const FVector* direction);
float fvec_dotProduct(const FVector* a, const FVector* b);
#define vec_angle(point) (atan2((point).y, (point).x) > 0 ? atan2((point).y, (point).x) : atan2((point).y, (point).x) + F_TAU)
#define fvec_angle(point) vec_angle(point)

Vector3 vec3(float x, float y, float z);
Vector3 vec3_sum(Vector3 a, Vector3 b);
Vector3 vec3_sub(Vector3 a, Vector3 b);
Vector3 vec3_scl(Vector3 a, float b);
Vector3 vec3_normalize(Vector3 a);
Vector3 vec3_cross(Vector3 a, Vector3 b);

bool pointInsideTriangle(const FVector* p, const FVector* a, const FVector* b, const FVector* c);

List list_create(int sizeOfType);
List list_createCustomSize(int sizeOfType, int reserved);
/* 
 * value will be dereferenced and stored in the list
 * Caution: adding the element invalidates previously requested element pointers */
void list_add(List* list, void* value);
/* Destination list should be big enough to contain all source elements */
void list_copy(List* from, List* to);
#define list_sort(list, comparator) qsort((list).data, (list).length, (list).sizeOfType, (comparator))
void* list_get(List* list, int index);
void* list_getLast(List* list);
/* EXTREME CAUTION: passed pointer is valid only if it was retrieved after last add/remove operation */
void list_removeByValue(List* list, void* value, bool ordered);
/* Caution: removing the element invalidates previously requested pointers */
void list_removeByID(List* list, int id, bool ordered);
void list_clear(List* list);
void list_destroy(List* list);

Bitset bitset_create(int size);
bool bitset_get(Bitset* bs, int i);
void bitset_set(Bitset* bs, int i, bool value);
void bitset_iterate(Bitset* bs, void (*callback)(int, bool));
void bitset_free(Bitset* bs);

Bitmap bitmap_create(int w, int h);
Bitmap bitmap_copy(Bitmap* src);
int bitmap_getSizeInBytes(Bitmap* bmp);
void bitmap_free(Bitmap* bmp);

float frand();
char* utils_loadAsString(char* filename);
//#define CUSTOM_ASSERTIONS
#ifndef NDEBUG
	#ifdef CUSTOM_ASSERTIONS
		#define assert(x) viiassert(x)
	#endif
#endif