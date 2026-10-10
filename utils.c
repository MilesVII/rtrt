#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "utils.h"

uint32_t encodeColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a){
	return ((uint32_t)r << 24) + ((uint32_t)g << 16) + ((uint32_t)b << 8) + a;
}
void decodeColorRGBA(uint32_t color, uint8_t* r, uint8_t* g, uint8_t* b, uint8_t* a){
	*r = decodeColorR(color);
	*g = decodeColorG(color);
	*b = decodeColorB(color);
	*a = decodeColorA(color);
}
void decodeColorRGB(uint32_t color, uint8_t* r, uint8_t* g, uint8_t* b){
	*r = decodeColorR(color);
	*g = decodeColorG(color);
	*b = decodeColorB(color);
}
uint8_t decodeColorR(uint32_t color){
	return color >> 24 & 0xFF;
}
uint8_t decodeColorG(uint32_t color){
	return color >> 16 & 0xFF;
}
uint8_t decodeColorB(uint32_t color){
	return color >> 8 & 0xFF;
}
uint8_t decodeColorA(uint32_t color){
	return color & 0xFF;
}

Rect rect_create(int minX, int minY, int maxX, int maxY){
	Rect r;
	r.minX = minX;
	r.minY = minY;
	r.maxX = maxX;
	r.maxY = maxY;
	return r;
}
FRect frect_create(float minX, float minY, float maxX, float maxY){
	FRect r;
	r.minX = minX;
	r.minY = minY;
	r.maxX = maxX;
	r.maxY = maxY;
	return r;
}
void rect_set(Rect* rect, int minX, int minY, int maxX, int maxY){
	rect->minX = minX;
	rect->minY = minY;
	rect->maxX = maxX;
	rect->maxY = maxY;
}
void rect_clamp(Rect* original, const Rect* clampingRegion){
	int w = original->maxX - original->minX;
	int h = original->maxY - original->minY;
	if (original->minX < clampingRegion->minX){
		original->minX = clampingRegion->minX;
		original->maxX = original->minX + w;
	} else if (original->maxX > clampingRegion->maxX){
		original->maxX = clampingRegion->maxX;
		original->minX = original->maxX - w;
	}

	if (original->minY < clampingRegion->minY){
		original->minY = clampingRegion->minY;
		original->maxY = original->minY + h;
	} else if (original->maxY > clampingRegion->maxY){
		original->maxY = clampingRegion->maxY;
		original->minY = original->maxY - h;
	}
}

void rect_move(Rect* rect, FVector* offset){
	rect->minX += offset->x;
	rect->maxX += offset->x;
	rect->minY += offset->y;
	rect->maxY += offset->y;
}
FRect rect_project(Rect* src, FRect* dst){
	FRect r;
	r.minX = remap((float)src->minX, (float)dst->minX, (float)dst->maxX, 0.0, 1.0);
	r.maxX = remap((float)src->maxX, (float)dst->minX, (float)dst->maxX, 0.0, 1.0);
	r.minY = remap((float)src->minY, (float)dst->minY, (float)dst->maxY, 0.0, 1.0);
	r.maxY = remap((float)src->maxY, (float)dst->minY, (float)dst->maxY, 0.0, 1.0);
	return r;
}

int rect_width(Rect* rect){
	return rect->maxX - rect->minX + 1;
}
int rect_height(Rect* rect){
	return rect->maxY - rect->minY + 1;
}
float frect_width(FRect* rect){
	return rect->maxX - rect->minX;
}
float frect_height(FRect* rect){
	return rect->maxY - rect->minY;
}
FRect frect_project(FRect* src, FRect* dst){
	FRect r;
	r.minX = remap(src->minX, 0.0, 1.0, (float)dst->minX, (float)dst->maxX);
	r.maxX = remap(src->maxX, 0.0, 1.0, (float)dst->minX, (float)dst->maxX);
	r.minY = remap(src->minY, 0.0, 1.0, (float)dst->minY, (float)dst->maxY);
	r.maxY = remap(src->maxY, 0.0, 1.0, (float)dst->minY, (float)dst->maxY);
	return r;
}
FRect frect_convert(Rect* r){
	FRect fr = {r->minX, r->minY, r->maxX, r->maxY};
	return fr;
}
void frect_clamp(Rect* original, const Rect* clampingRegion){
	float w = original->maxX - original->minX;
	float h = original->maxY - original->minY;
	if (original->minX < clampingRegion->minX){
		original->minX = clampingRegion->minX;
		original->maxX = original->minX + w;
	} else if (original->maxX > clampingRegion->maxX){
		original->maxX = clampingRegion->maxX;
		original->minX = original->maxX - w;
	}

	if (original->minY < clampingRegion->minY){
		original->minY = clampingRegion->minY;
		original->maxY = original->minY + h;
	} else if (original->maxY > clampingRegion->maxY){
		original->maxY = clampingRegion->maxY;
		original->minY = original->maxY - h;
	}
}

float frand(){
	return (double)rand() / (double)RAND_MAX;
}

Vector vec_create(int x , int y){
	Vector v = {x, y};
	return v;
}
Vector vec_convert(const FVector* fv){
	Vector v = {round(fv->x), round(fv->y)};
	return v;
}
Vector vec_add(const Vector* a, const Vector* b){
	Vector v = {a->x + b->x, a->y + b->y};
	return v;
}
Vector vec_sub(const Vector* a, const Vector* b){
	Vector v = {a->x - b->x, a->y - b->y};
	return v;
}
Vector vec_mul(const Vector* a, const Vector* b){
	Vector v;
	v.x = a->x * b->x;
	v.y = a->y * b->y;
	return v;
}
Vector vec_scl(const Vector* a, float x){
	Vector v = {a->x * x, a->y * x};
	return v;
}
Vector vec_div(const Vector* a, const Vector* b){
	Vector v = {a->x / b->x, a->y / b->y};
	return v;
}
float vec_length2(const Vector* v){
	return v->x * v->x + v->y * v->y;
}
float vec_distance2(const Vector* a, const Vector* b){
	Vector _ = vec_sub(a, b);
	return vec_length2(&_);
}
float vec_angleOrigin(const Vector* origin, const Vector* direction){
	Vector _ = vec_sub(direction, origin);
	return vec_angle(_);
}
FVector fvec_create(float x, float y){
	FVector v = {x, y};
	return v;
}
FVector fvec_convert(const Vector* v){
	FVector fv = {v->x, v->y};
	return fv;
}
FVector fvec_add(const FVector* a, const FVector* b){
	FVector fv = {a->x + b->x, a->y + b->y};
	return fv;
}
FVector fvec_sub(const FVector* a, const FVector* b){
	FVector fv = {a->x - b->x, a->y - b->y};
	return fv;
}
FVector fvec_mul(const FVector* a, const FVector* b){
	FVector fv = {a->x * b->x, a->y * b->y};
	return fv;
}
FVector fvec_scl(const FVector* a, float x){
	FVector fv = {a->x * x, a->y * x};
	return fv;
}
FVector fvec_div(const FVector* a, const FVector* b){
	FVector v = {a->x / b->x, a->y / b->y};
	return v;
}
FVector fvec_rotate(const FVector* a, float angle){
	angle = F_TAU - angle;
	FVector fv = {a->x * cos(angle) - a->y * sin(angle), a->x * sin(angle) + a->y * cos(angle)};
	return fv;
}
FVector fvec_norm(const FVector* a){
	return fvec_scl(a, 1.0 / (float)sqrt(fvec_length2(a)));
}
float fvec_length2(const FVector* v){
	return v->x * v->x + v->y * v->y;
}
float fvec_distance2(const FVector* a, const FVector* b){
	FVector _ = fvec_sub(a, b);
	return fvec_length2(&_);
}
float fvec_angleOrigin(const FVector* origin, const FVector* direction){
	FVector _ = fvec_sub(direction, origin);
	return fvec_angle(_);
}
float fvec_dotProduct(const FVector* a, const FVector* b){
	return a->x * b->x + a->y * b->y;
}
Vector3 vec3(float x, float y, float z){
	Vector3 v = {x, y, z};
	return v;
}
Vector3 vec3_sum(Vector3 a, Vector3 b){
	Vector3 v = {
		a.x + b.x,
		a.y + b.y,
		a.z + b.z
	};
	return v;
}
Vector3 vec3_sub(Vector3 a, Vector3 b){
	Vector3 v = {
		a.x - b.x,
		a.y - b.y,
		a.z - b.z
	};
	return v;
}
Vector3 vec3_scl(Vector3 a, float b){
	Vector3 v = {
		a.x * b,
		a.y * b,
		a.z * b
	};
	return v;
}
Vector3 vec3_normalize(Vector3 a){
	float l = sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
	Vector3 v = {
		a.x * (1.0f / l),
		a.y * (1.0f / l),
		a.z * (1.0f / l)
	};
	return v;
}
Vector3 vec3_cross(Vector3 a, Vector3 b){
	Vector3 v = {
		a.y * b.z - a.z * b.y,
		a.z * b.x - a.x * b.z,
		a.x * b.y - a.y * b.x
	};
	return v;
}

static float pointOnHalfplane(const FVector* p, const FVector* a, const FVector* b){
	return (p->x - b->x) * (a->y - b->y) - (a->x - b->x) * (p->y - b->y);
}

bool pointInsideTriangle(const FVector* p, const FVector* a, const FVector* b, const FVector* c){
	float d1 = pointOnHalfplane(p, a, b);
	float d2 = pointOnHalfplane(p, b, c);
	float d3 = pointOnHalfplane(p, c, a);

	bool onNegative = (d1 < 0) && (d2 < 0) && (d3 < 0);
	bool onPositive = (d1 > 0) && (d2 > 0) && (d3 > 0);

	return (onNegative || onPositive);
}

const int LIST_RESERVED_BY_DEFAULT = 16;
List list_create(int sizeOfType){
	return list_createCustomSize(sizeOfType, LIST_RESERVED_BY_DEFAULT);
}
List list_createCustomSize(int sizeOfType, int reserved){
	List l;
	l.sizeOfType = sizeOfType;
	l.length = 0;
	l.reserved = reserved;
	l.minimalReserved = reserved;
	l.data = malloc(l.sizeOfType * l.reserved);
	l.doNotShrink = false;
	return l;
}
void list_copy(List* from, List* to){
	assert(from->sizeOfType == to->sizeOfType);
	assert(from->length <= to->reserved);
	memcpy(to->data, from->data, from->sizeOfType * from->length);
	to->length = from->length;
}
void list_add(List* list, void* value){
	++list->length;
	if (list->reserved < list->length){
		list->reserved *= 2;
		list->data = realloc(list->data, list->sizeOfType * list->reserved);
		assert(list->data != NULL);
	}
	void* last = list_get(list, list->length - 1);
	memcpy(last, value, list->sizeOfType);
}
void* list_get(List* list, int index){
	return list->data + list->sizeOfType * index;
}
void* list_getLast(List* list){
	return list_get(list, list->length - 1);
}
void list_removeByValue(List* list, void* value, bool ordered){
	if (list->data <= value && value <= list_getLast(list)){
		for (int i = 0; i < list->length; ++i)
			if (list_get(list, i) == value){
				list_removeByID(list, i, ordered);
				return;
			}
		assert(false);
	}
}
void list_removeByID(List* list, int id, bool ordered){
	if (ordered){
		for (int i = id; i < list->length - 1; ++i)
			memcpy(list_get(list, i), list_get(list, i + 1), list->sizeOfType);
	} else {
		memcpy(list_get(list, id), list_get(list, list->length - 1), list->sizeOfType);
	}
	--list->length;

	if (!list->doNotShrink){
		bool listGrown = list->reserved > list->minimalReserved;
		if (list->length <= (list->reserved / 2) && listGrown){
			list->reserved /= 2;
			list->data = realloc(list->data, list->sizeOfType * list->reserved);
			assert(list->data != NULL);
		}
	}
}
void list_clear(List* list){
	list->length = 0;
	if (!list->doNotShrink){
		list->reserved = list->minimalReserved;
		list->data = realloc(list->data, list->sizeOfType * list->reserved);
	}
}
void list_destroy(List* list){
	list->length = -1;
	list->reserved = -1;
	list->sizeOfType = -1;
	free(list->data);
}

Bitset bitset_create(int size){
	Bitset bs;
	bs.size = size;
	int actualSize = size >> 3;
	if (size % 8 != 0)
		++actualSize;
	bs.data = malloc(sizeof(uint8_t) * actualSize);
	return bs;
}
bool bitset_get(Bitset* bs, int i){
	int byteId = i >> 3;
	uint8_t byte = bs->data[byteId];
	return byte & (1 << (i % 8));
}
void bitset_set(Bitset* bs, int i, bool value){
	int byteId = i >> 3;
	if (value)
		bs->data[byteId] |= (1 << (i % 8));
	else
		bs->data[byteId] &= ~(1 << (i % 8));
}
void bitset_iterate(Bitset* bs, void(*callback)(int, bool)){
	int actualSize = bs->size >> 3;
	if (bs->size % 8 != 0)
		++actualSize;
	uint8_t bitLimit = 8;
	for (int i = 0; i < actualSize; ++i){
		if (i == actualSize - 1)
			bitLimit = bs->size % 8;
		for (uint8_t j = 0; j < bitLimit; ++j){
			bool value = bs->data[i] & (1 << (j % 8));
			callback(i * 8 + j, value);
		}
	}
}
void bitset_free(Bitset* bs){
	free(bs->data);
}

Bitmap bitmap_create(int w, int h){
	Bitmap bitmap;
	bitmap.w = w;
	bitmap.h = h;

	bitmap.data = malloc(w * h * sizeof(uint32_t));

	return bitmap;
}

Bitmap bitmap_copy(Bitmap* src){
	Bitmap bitmap = bitmap_create(src->w, src->h);
	memcpy(bitmap.data, src->data, bitmap_getSizeInBytes(src));

	return bitmap;
}

int bitmap_getSizeInBytes(Bitmap* bmp){
	return bmp->w * bmp->h * sizeof(uint32_t);
}

void bitmap_free(Bitmap* bmp){
	free(bmp->data);
	bmp->data = NULL;

	bmp->w = 0;
	bmp->h = 0;
}

void viiassert(int expression){
	if (!expression)
		printf("Assertion failure occured, use breakpoint on utils.c viiassert()");
}

const int FILE_IO_BUFFER_SIZE = 256;
char* utils_loadAsString(char* filename){
	FILE* f = fopen(filename, "r");
	assert(f != NULL);

	char buffer[FILE_IO_BUFFER_SIZE];
	List result = list_createCustomSize(sizeof(char), 2048);

	while (!feof(f)){
		if (fgets(buffer, FILE_IO_BUFFER_SIZE, f) != NULL)
			for (int i = 0; i < FILE_IO_BUFFER_SIZE - 1; ++i){
				if (buffer[i] == '\0')
					break;
				
				list_add(&result, &buffer[i]);
			}
	}

	char* r = malloc(sizeof(char) * (result.length + 1));
	memcpy(r, list_get(&result, 0), result.length);
	r[result.length] = '\0';

	list_destroy(&result);
	fclose(f);
	return r;
}