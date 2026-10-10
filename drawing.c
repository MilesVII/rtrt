#include <stdbool.h>
#include <stdio.h>
#include <math.h>

#define GLEW_STATIC
#include <GL/glew.h>
#include <SDL2/SDL.h>
//#include <SDL2/SDL_opengl.h>
#include <SDL2_ttf/SDL_ttf.h>

#include "drawing.h"
#include "utils.h"

struct GLVertex {
	FVector position;
	GLuint texture;
	FVector textureCoordinates;
	uint32_t color;
};
typedef struct GLVertex GLVertex;

struct Texture {
	GLuint name;
	Vector size;
};

static const float DRAWING_ARC_ANGULAR_STEP = F_TAU / 360.0;
static const float DRAWING_LINE_WIDTH = 1;
static const bool DRAWING_SWAP_COLOR_BYTES_FOR_SHADER = true;
const int RVLT_SIZE = 1000;
static bool drawing_fogOfWarEnabled = false;
static Vector drawing_screenSize;

static SDL_Window* drawing_window;
static TTF_Font* drawing_font;
static GLint drawing_glVertexPositionsAttribute, drawing_glVertexTextureCoordinatesAttribute, drawing_glVertexColorAttribute;
static GLuint drawing_glProgram, drawing_glVBO, drawing_glWhitePixel;
static List drawing_glVertices;
static List drawing_glSlaughterhouse;
static void drawing_drawSequence(GLuint texture, int from, int to);

static GLuint drawing_glFOWFramebuffer;
static Texture drawing_glFOWTexture;
static GLuint drawing_glFOWRenderbuffer;
static GLuint drawing_glFOWVBO;
static List drawing_glFOWVertices;

static SDL_Color drawing_convertColor(uint32_t color){
	SDL_Color processedColor = {decodeColorR(color), decodeColorG(color), decodeColorB(color), decodeColorA(color)};
	return processedColor;
}

static void drawing_sdlToRect(SDL_Rect* source, Rect* dest){
	dest->minX = source->x;
	dest->minY = source->y;
	dest->maxX = source->x + source->w;
	dest->maxY = source->y + source->h;
}
static void drawing_rectToSDL(Rect* source, SDL_Rect* dest){
	dest->x = source->minX;
	dest->y = source->minY;
	dest->w = source->maxX - source->minX;
	dest->h = source->maxY - source->minY;
}

static GLVertex drawing_addVertexCustomDestination(float positionX, float positionY, float textureX, float textureY, GLuint texture, uint32_t color, List* destination){
	GLVertex glv;
	glv.position.x = remap(positionX, 0, (float)drawing_screenSize.x, -1.0, 1.0);
	glv.position.y = remap(positionY, 0, (float)drawing_screenSize.y, -1.0, 1.0);
	glv.textureCoordinates.x = textureX;
	glv.textureCoordinates.y = textureY;
	glv.texture = texture;
	if (DRAWING_SWAP_COLOR_BYTES_FOR_SHADER)
		color = ((color>>24)&0xff) | ((color<<8)&0xff0000) | ((color>>8)&0xff00) | ((color<<24)&0xff000000);
	glv.color = color;
	list_add(destination, &glv);
	return glv;
}

static GLVertex drawing_addVertex(float positionX, float positionY, float textureX, float textureY, GLuint texture, uint32_t color){
	drawing_addVertexCustomDestination(positionX, positionY, textureX, textureY, texture, color, &drawing_glVertices);
}

static void drawing_postponeTextureDeletion(Texture* toBeRemoved){
	list_add(&drawing_glSlaughterhouse, &toBeRemoved);
}

static Texture drawing_createTextureOnStack(Bitmap* bmp){
	Texture gfx;
	gfx.size = vec_create(bmp->w, bmp->h);
	glGenTextures(1, &gfx.name);
	drawing_rewriteTexture(bmp, &gfx);
	return gfx;
}

static void drawing_rebindVerticesToBuffer(){
	drawing_glVertexPositionsAttribute = glGetAttribLocation(drawing_glProgram, "position");
	glEnableVertexAttribArray(drawing_glVertexPositionsAttribute);
	glVertexAttribPointer(drawing_glVertexPositionsAttribute, 2, GL_FLOAT, GL_FALSE, sizeof(GLVertex), (GLvoid*)offsetof(GLVertex, position));

	drawing_glVertexTextureCoordinatesAttribute = glGetAttribLocation(drawing_glProgram, "textureCoordinates");
	glEnableVertexAttribArray(drawing_glVertexTextureCoordinatesAttribute);
	glVertexAttribPointer(drawing_glVertexTextureCoordinatesAttribute, 2, GL_FLOAT, GL_FALSE, sizeof(GLVertex), (GLvoid*)offsetof(GLVertex, textureCoordinates));
	
	drawing_glVertexColorAttribute = glGetAttribLocation(drawing_glProgram, "color");
	glEnableVertexAttribArray(drawing_glVertexColorAttribute);
	glVertexAttribPointer(drawing_glVertexColorAttribute, 4,  GL_UNSIGNED_BYTE, GL_TRUE, sizeof(GLVertex), (GLvoid*)offsetof(GLVertex, color));
}

static GLuint drawing_compileShaderProgram(const char *vs_src, const char *fs_src) {
	GLchar *log = NULL;
	GLint log_length, success;
	GLuint fragment_shader, program, vertex_shader;

	/* Vertex shader */
	vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, &vs_src, NULL);
	glCompileShader(vertex_shader);
	glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
	glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &log_length);
	log = malloc(log_length);
	if (!success) {
		printf("vertex shader compile error\n");
		char log[1000];
		glGetShaderInfoLog(vertex_shader, 999, &log_length, log);
		printf("%s\n", log);
		exit(EXIT_FAILURE);
	}

	/* Fragment shader */
	fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader, 1, &fs_src, NULL);
	glCompileShader(fragment_shader);
	glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
	glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &log_length);
	if (!success) {
		printf("fragment shader compile error\n");
		char log[1000];
		glGetShaderInfoLog(fragment_shader, 999, &log_length, log);
		printf("%s\n", log);
		exit(EXIT_FAILURE);
	}

	/* Link shaders */
	program = glCreateProgram();
	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);
	glLinkProgram(program);
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);
	if (!success) {
		printf("shader link error");
		char log[1000];
		glGetShaderInfoLog(fragment_shader, 999, &log_length, log);
		printf("%s\n", log);
		exit(EXIT_FAILURE);
	}

	/* Cleanup. */
	free(log);
	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);
	return program;
}

Vector drawing_init(char* title, float w, float h){
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0){
		printf("SDL Init error: %s\n", SDL_GetError());
		SDL_Delay(7000);
		exit(1);
	}
	if(TTF_Init() == -1) {
		printf("SDL TTF init error: %s\n", TTF_GetError());
		exit(1);
	}
	drawing_font = TTF_OpenFont("assets/ProggyClean.ttf", 16);

	Vector windowSize;
	if (w <= 1 && h <= 1){
		windowSize = drawing_getScreenSize();
		windowSize.x *= w;
		windowSize.y *= h;
	} else {
		windowSize.x = w;
		windowSize.y = h;
	}
	Uint32 flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL;
	drawing_window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
	                                   windowSize.x, windowSize.y, flags);

	SDL_GLContext gl_context;
	drawing_glVertices = list_createCustomSize(sizeof(GLVertex), 4096);
	drawing_glSlaughterhouse = list_create(sizeof(Texture*));
	drawing_glFOWVertices = list_createCustomSize(sizeof(GLVertex), 2048);
	drawing_glFOWVertices.doNotShrink = true;

	gl_context = SDL_GL_CreateContext(drawing_window);
	glewInit();

	char* vs = utils_loadAsString("shaders/vertex.glsl");
	char* fs = utils_loadAsString("shaders/fragment.glsl");
	drawing_glProgram = drawing_compileShaderProgram(vs, fs);
	glUseProgram(drawing_glProgram);
	free(vs);
	free(fs);
	
	glGenBuffers(1, &drawing_glVBO);
	glBindBuffer(GL_ARRAY_BUFFER, drawing_glVBO);

	drawing_rebindVerticesToBuffer();
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	drawing_clear(0x808080FF);

	uint32_t white[4] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
	Bitmap whiteBitmap;
	whiteBitmap.data = white;
	whiteBitmap.w = 2;
	whiteBitmap.h = 2;
	Texture t = drawing_createTextureOnStack(&whiteBitmap);
	drawing_glWhitePixel = t.name;

	drawing_screenSize = windowSize;

	GLint swul = glGetUniformLocation(drawing_glProgram, "screenWidth");
	glUniform1i(swul, drawing_screenSize.x);
	GLint shul = glGetUniformLocation(drawing_glProgram, "screenHeight");
	glUniform1i(shul, drawing_screenSize.y);

	return windowSize;
}

void drawing_resize(int width, int height){
	drawing_screenSize.x = width;
	drawing_screenSize.y = height;
	glViewport(0, 0, width, height);
}

Vector drawing_getScreenSize(){
	Vector v;
	SDL_DisplayMode dm;
	if (SDL_GetDesktopDisplayMode(0, &dm) != 0) {
		printf("SDL Desktop Display Mode Retrieval error: %s\n", SDL_GetError());
		v.x = 800;
		v.y = 420;
		return v;
	}
	v.x = dm.w;
	v.y = dm.h;
	return v;
}

void drawing_clear(uint32_t color){
	uint8_t r, g, b, a;
	decodeColorRGBA(color, &r, &g, &b, &a);
	glClearColor((float)r / 255.0, (float)g / 255.0, (float)b / 255.0, (float)a / 255.0);
	glClear(GL_COLOR_BUFFER_BIT);
}

void drawing_rewriteTexture(Bitmap* bmp, Texture* gfx){
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gfx->name);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bmp->w, bmp->h, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, bmp->data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
}

Texture* drawing_createTexture(Bitmap* bmp){
	Texture* gfx = malloc(sizeof(Texture));
	gfx->size = vec_create(bmp->w, bmp->h);
	glGenTextures(1, &gfx->name);
	drawing_rewriteTexture(bmp, gfx);
	return gfx;
}

void drawing_texture(Texture* gfx, FRect* destination){
	drawing_addVertex(destination->minX, destination->minY, 0.0, 0.0, gfx->name, 0xFFFFFFFF);
	drawing_addVertex(destination->minX, destination->maxY, 0.0, 1.0, gfx->name, 0xFFFFFFFF);
	drawing_addVertex(destination->maxX, destination->maxY, 1.0, 1.0, gfx->name, 0xFFFFFFFF);

	drawing_addVertex(destination->minX, destination->minY, 0.0, 0.0, gfx->name, 0xFFFFFFFF);
	drawing_addVertex(destination->maxX, destination->minY, 1.0, 0.0, gfx->name, 0xFFFFFFFF);
	drawing_addVertex(destination->maxX, destination->maxY, 1.0, 1.0, gfx->name, 0xFFFFFFFF);
}
void drawing_circle(FVector* position, float radius, uint32_t color){
	drawing_circleRaw(position->x, position->y, radius, color);
}
void drawing_circleRaw(float x, float y, float radius, uint32_t color){
	int triangles = round(F_TAU / DRAWING_ARC_ANGULAR_STEP);
	FVector center = {x, y};
	FVector ray = {radius, 0};
	FVector previousRay = ray;
	ray = fvec_rotate(&ray, DRAWING_ARC_ANGULAR_STEP);

	for (int i = 0; i < triangles; ++i){
		FVector a = fvec_add(&center, &ray);
		FVector b = fvec_add(&center, &previousRay);

		drawing_addVertex(center.x, center.y, 0.0, 0.0, drawing_glWhitePixel, color);
		drawing_addVertex(a.x, a.y, 0.0, 1.0, drawing_glWhitePixel, color);
		drawing_addVertex(b.x, b.y, 1.0, 1.0, drawing_glWhitePixel, color);

		previousRay = ray;
		ray = fvec_rotate(&ray, DRAWING_ARC_ANGULAR_STEP);
	}
}

void drawing_rect(FRect* rect, uint32_t color){
	drawing_rectRaw(rect->minX, rect->minY, rect->maxX, rect->maxY, color);
}
void drawing_rectVV(FVector* position, FVector* size, uint32_t color){
	drawing_rectRaw(position->x, position->y, position->x + size->x, position->y + size->y, color);
}
void drawing_rectRaw(float minX, float minY, float maxX, float maxY, uint32_t color){
	drawing_addVertex(minX, minY, 0.0, 0.0, drawing_glWhitePixel, color);
	drawing_addVertex(minX, maxY, 0.0, 1.0, drawing_glWhitePixel, color);
	drawing_addVertex(maxX, maxY, 1.0, 1.0, drawing_glWhitePixel, color);

	drawing_addVertex(minX, minY, 0.0, 0.0, drawing_glWhitePixel, color);
	drawing_addVertex(maxX, minY, 0.0, 1.0, drawing_glWhitePixel, color);
	drawing_addVertex(maxX, maxY, 1.0, 1.0, drawing_glWhitePixel, color);
}
void drawing_triangle(FVector* a, FVector* b, FVector* c, uint32_t color){
	drawing_triangleRaw(a->x, a->y, b->x, b->y, c->x, c->y, color);
}
void drawing_triangleRaw(float aX, float aY, float bX, float bY, float cX, float cY, uint32_t color){
	drawing_addVertex(aX, aY, 0.0, 0.0, drawing_glWhitePixel, color);
	drawing_addVertex(bX, bY, 0.0, 1.0, drawing_glWhitePixel, color);
	drawing_addVertex(cX, cY, 1.0, 1.0, drawing_glWhitePixel, color);
}
void drawing_line(FVector* from, FVector* to, uint32_t color){
	drawing_lineRaw(from->x, from->y, to->x, to->y, color);
}
void drawing_lineRaw(float fromX, float fromY, float toX, float toY, uint32_t color){
	FVector from = {fromX, fromY};
	FVector to = {toX, toY};
	FVector a, b, c, d;

	FVector ray = fvec_sub(&to, &from);
	FVector offset = fvec_norm(&ray);
	float halfLineWidth = (float)DRAWING_LINE_WIDTH / 2.0;
	offset = fvec_scl(&offset, halfLineWidth);
	offset = fvec_rotate(&offset, F_TAU * .25);
	a = fvec_add(&from, &offset);
	offset = fvec_rotate(&offset, F_TAU * .5);
	b = fvec_add(&from, &offset);

	c = fvec_add(&a, &ray);
	d = fvec_add(&b, &ray);

	FVector v;
	GLVertex glv;
	drawing_addVertex(a.x, a.y, 0.0, 0.0, drawing_glWhitePixel, color);
	drawing_addVertex(b.x, b.y, 0.0, 1.0, drawing_glWhitePixel, color);
	drawing_addVertex(c.x, c.y, 1.0, 1.0, drawing_glWhitePixel, color);

	drawing_addVertex(c.x, c.y, 0.0, 0.0, drawing_glWhitePixel, color);
	drawing_addVertex(d.x, d.y, 0.0, 1.0, drawing_glWhitePixel, color);
	drawing_addVertex(b.x, b.y, 1.0, 1.0, drawing_glWhitePixel, color);
}

void drawing_path(FVector* path, int pathNodes, uint32_t color){
	for (int i = 0; i < pathNodes - 1; ++i)
		drawing_line(&(path[i]), &(path[i + 1]), color);
}
void drawing_arc(FVector* center, float radius, float sectorRelative, float offsetRelative, uint32_t color){
	FVector radial = {radius, 0};
	FVector intermediator;
	FVector v0, v1;
	radial = fvec_rotate(&radial, F_TAU * offsetRelative);
	int steps = ceil(F_TAU * sectorRelative / DRAWING_ARC_ANGULAR_STEP);
	for (int i = 0; i < steps - 1; ++i){
		intermediator = radial;
		radial = fvec_rotate(&radial, DRAWING_ARC_ANGULAR_STEP);
		v0 = fvec_add(center, &intermediator);
		v1 = fvec_add(center, &radial);
		drawing_line(&v0, &v1, color);
	}
}

static void drawing_flipBitmapRows(Bitmap* bmp){
	uint32_t bufferPixel;
	for (int y = 0; y < floor(bmp->h / 2.0); ++y){
		int targetY = bmp->h - y - 1;
		for (int x = 0; x < bmp->w; ++x){
			uint32_t* a = get(bmp->data, x,       y, bmp->w);
			uint32_t* b = get(bmp->data, x, targetY, bmp->w);
			bufferPixel = *a;
			*a = *b;
			*b = bufferPixel;
		}
	}
}
Texture* drawing_generateTextTexture(char* text, uint32_t color, int widthLimit){
	SDL_Surface* ts = TTF_RenderUTF8_Blended_Wrapped(drawing_font, text, drawing_convertColor(color), widthLimit);
	SDL_Surface* s = SDL_ConvertSurfaceFormat(ts, SDL_PIXELFORMAT_RGBA8888, 0);
	SDL_FreeSurface(ts);
	Bitmap bmp;
	bmp.w = s->w;
	bmp.h = s->h;
	SDL_LockSurface(s);
	bmp.data = (uint32_t*)s->pixels;
	drawing_flipBitmapRows(&bmp);
	Texture* t = drawing_createTexture(&bmp);
	SDL_FreeSurface(s);
	return t;
}
void drawing_text(Texture* gfx, float x, float y, Align halign, Align valign, FRect* clampingRegion){
	SDL_Rect textRect;

	textRect.w = gfx->size.x;
	textRect.h = gfx->size.y;
	textRect.x = x;
	textRect.y = y;

	switch (halign){
	case (ALIGN_START):
		break;
	case (ALIGN_MIDDLE):
		textRect.x -= textRect.w / 2;
		break;
	case (ALIGN_END):
		textRect.x -= textRect.w;
		break;
	}
	switch (valign){
	case (ALIGN_START):
		break;
	case (ALIGN_MIDDLE):
		textRect.y -= textRect.h / 2;
		break;
	case (ALIGN_END):
		textRect.y -= textRect.h;
		break;
	}
	if (clampingRegion != NULL){
		int clampW = abs(clampingRegion->maxX - clampingRegion->minX) + 1;
		assert(clampW >= textRect.w);
		Rect temporaryRect, iClamping;
		drawing_sdlToRect(&textRect, &temporaryRect);
		iClamping = rect_create(clampingRegion->minX, 
		                        clampingRegion->minY, 
		                        clampingRegion->maxX, 
		                        clampingRegion->maxY);
		rect_clamp(&temporaryRect, &iClamping);
		drawing_rectToSDL(&temporaryRect, &textRect);
	}

	Rect r;
	drawing_sdlToRect(&textRect, &r);
	FRect fr = frect_create(r.minX, r.minY, r.maxX, r.maxY);
	drawing_texture(gfx, &fr);
}

void drawing_textExt(char* text, float x, float y, uint32_t color, int widthLimit, Align halign, Align valign, FRect* clampingRegion){
	Texture* gfx = drawing_generateTextTexture(text, color, widthLimit);
	drawing_text(gfx, x, y, halign, valign, clampingRegion);
	drawing_postponeTextureDeletion(gfx);
}

float timer = 0;
int drawing_sceneId = 0;
void drawing_updateScene(int sceneId){
	switch (sceneId){
	case(0): {
		float moonRot = timer * 0.7;
		float moonOrbit = 17.0;
		float venusRot = timer * -0.9;
		float venusOrbit =  12.7;
		float subsunRot = timer * 3.2;
		float subsunOrbit =  20.0;
		float shaderRot = timer * 0.3;
		float shaderOrbit = 11.0;

		glUniform1i(glGetUniformLocation(drawing_glProgram, "planesCount"), 1);
		glUniform1i(glGetUniformLocation(drawing_glProgram, "spheresCount"), 5);
		glUniform1i(glGetUniformLocation(drawing_glProgram, "ringsCount"), 0);

		//Grass
		glUniform1f(glGetUniformLocation(drawing_glProgram, "sceneDataPlanes[0].originDistance"), -6.7f);
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataPlanes[0].normal"), 0.0f, 0.0f, 1.0f);
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataPlanes[0].material.diffuse"), 0.32f, 0.98f, 0.42f);
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataPlanes[0].material.emission"), 0.0f, 0.0f, 0.0f);
		
		//Sun
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataSpheres[0].position"), 0.0f, 0.0f, 0.0f);
		glUniform1f(glGetUniformLocation(drawing_glProgram, "sceneDataSpheres[0].radius"), 10.0f);
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataSpheres[0].material.diffuse"), 1.0f, 0.0f, 0.0f);
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataSpheres[0].material.emission"), 3.0f, 2.1f, 0.4f);

		//Moon
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataSpheres[1].position"), cos(moonRot) * moonOrbit, sin(moonRot) * moonOrbit, 2.1f);
		glUniform1f(glGetUniformLocation(drawing_glProgram, "sceneDataSpheres[1].radius"), 4.0f);
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataSpheres[1].material.diffuse"), 0.32f, 0.35f, 0.42f);
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataSpheres[1].material.emission"), 0.0f, 0.0f, 0.0f);

		//Venus
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataSpheres[2].position"), cos(venusRot) * venusOrbit, sin(venusRot) * venusOrbit, 1.2f);
		glUniform1f(glGetUniformLocation(drawing_glProgram, "sceneDataSpheres[2].radius"), 2.0f);
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataSpheres[2].material.diffuse"), 0.4f, 0.4f, 1.0f);
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataSpheres[2].material.emission"), 0.0f, 0.0f, 0.0f);
		
		//Subsun
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataSpheres[3].position"), cos(subsunRot) * subsunOrbit, sin(subsunRot) * subsunOrbit, 1.2f);
		glUniform1f(glGetUniformLocation(drawing_glProgram, "sceneDataSpheres[3].radius"), 2.0f);
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataSpheres[3].material.diffuse"), 0.0f, 0.0f, 0.0f);
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataSpheres[3].material.emission"), 7.0f, 7.0f, 12.0f);

		//Shader
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataSpheres[4].position"), cos(shaderRot) * shaderOrbit, sin(shaderRot) * shaderOrbit, -3.5f);
		glUniform1f(glGetUniformLocation(drawing_glProgram, "sceneDataSpheres[4].radius"), 3.2f);
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataSpheres[4].material.diffuse"), 0.32f, 0.35f, 0.42f);
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataSpheres[4].material.emission"), 0.0f, 0.0f, 0.0f);
		break;
	}
	case (1):
		glUniform1i(glGetUniformLocation(drawing_glProgram, "planesCount"), 6);
		glUniform1i(glGetUniformLocation(drawing_glProgram, "spheresCount"), 2);
		glUniform1i(glGetUniformLocation(drawing_glProgram, "ringsCount"), 0);

		//Floor
		glUniform1f(glGetUniformLocation(drawing_glProgram, "sceneDataPlanes[0].originDistance"), -7.0f);
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataPlanes[0].normal"), 0.0f, 0.0f, 1.0f);
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataPlanes[0].material.diffuse"), 0.9f, 0.9f, 0.9f);
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataPlanes[0].material.emission"), 0.0f, 0.0f, 0.0f);
		//WallN
		glUniform1f(glGetUniformLocation(drawing_glProgram, "sceneDataPlanes[1].originDistance"), 7.0f);
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataPlanes[1].normal"), -1.0f, 0.0f, 0.0f);
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataPlanes[1].material.diffuse"), 0.0f, 0.9f, 0.0f);
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataPlanes[1].material.emission"), 0.0f, 0.0f, 0.0f);
		//WallE
		glUniform1f(glGetUniformLocation(drawing_glProgram, "sceneDataPlanes[2].originDistance"), 7.0f);
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataPlanes[2].normal"), 0.0f, -1.0f, 0.0f);
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataPlanes[2].material.diffuse"), 0.9f, 0.0f, 0.9f);
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataPlanes[2].material.emission"), 0.0f, 0.0f, 0.0f);
		//WallS
		glUniform1f(glGetUniformLocation(drawing_glProgram, "sceneDataPlanes[3].originDistance"), -7.0f);
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataPlanes[3].normal"), 1.0f, 0.0f, 0.0f);
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataPlanes[3].material.diffuse"), 0.9f, 0.9f, 0.9f);
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataPlanes[3].material.emission"), 0.0f, 0.0f, 0.0f);
		//WallW
		glUniform1f(glGetUniformLocation(drawing_glProgram, "sceneDataPlanes[4].originDistance"), -7.0f);
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataPlanes[4].normal"), 0.0f, 1.0f, 0.0f);
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataPlanes[4].material.diffuse"), 0.9f, 0.9f, 0.9f);
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataPlanes[4].material.emission"), 0.0f, 0.0f, 0.0f);
		//Ceiling
		glUniform1f(glGetUniformLocation(drawing_glProgram, "sceneDataPlanes[5].originDistance"), 2.0f);
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataPlanes[5].normal"), 0.0f, 0.0f, -1.0f);
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataPlanes[5].material.diffuse"), 0.0f, 1.0f, 1.0f);
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataPlanes[5].material.emission"), 1.0f, 1.0f, 1.0f);

		//Ball
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataSpheres[0].position"), 2.8f, 2.0f, -2.8f);
		glUniform1f(glGetUniformLocation(drawing_glProgram, "sceneDataSpheres[0].radius"), 4.2f);
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataSpheres[0].material.diffuse"), 0.7f, 0.0f, 0.0f);
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataSpheres[0].material.emission"), 0.0f, 0.0f, 0.0f);
		
		//Ball
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataSpheres[1].position"), 2.8f, 2.0f, -2.8f);
		glUniform1f(glGetUniformLocation(drawing_glProgram, "sceneDataSpheres[1].radius"), 4.2f);
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataSpheres[1].material.diffuse"), 0.7f, 0.0f, 0.0f);
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataSpheres[1].material.emission"), 0.0f, 0.0f, 0.0f);
		break;
	case(2): {
		float sunaRot = timer * 0.64f + F_TAU / 2.0f;
		float sunbRot = timer * 0.64f;
		float sunOrbit = 8.2f;
		float moonRot = timer * -0.4f;
		float moonOrbit = 17.0f;
		float venusRot = timer * -1.3f;
		float venusOrbit =  0.7f;
		float subsunRot = timer * 3.2f;
		float subsunOrbit =  20.0f;
		float shaderRot = timer * 0.32f;
		float shaderOrbit = 16.0f;

		glUniform1i(glGetUniformLocation(drawing_glProgram, "planesCount"), 1);
		glUniform1i(glGetUniformLocation(drawing_glProgram, "spheresCount"), 5);
		glUniform1i(glGetUniformLocation(drawing_glProgram, "ringsCount"), 0);

		//Grass
		glUniform1f(glGetUniformLocation(drawing_glProgram, "sceneDataPlanes[0].originDistance"), -6.7f);
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataPlanes[0].normal"), 0.0f, 0.0f, 1.0f);
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataPlanes[0].material.diffuse"), 0.8f, 0.8f, 0.8f);
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataPlanes[0].material.emission"), 0.0f, 0.0f, 0.0f);
		
		//SunA
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataSpheres[0].position"), cos(sunaRot) * sunOrbit, sin(sunaRot) * sunOrbit, 2.1f);
		glUniform1f(glGetUniformLocation(drawing_glProgram, "sceneDataSpheres[0].radius"), 7.0f);
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataSpheres[0].material.diffuse"), 0.82f, 0.82f, 1.0f);
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataSpheres[0].material.emission"), 3.0f, 3.1f, 6.2f);
		//SunB
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataSpheres[4].position"), cos(sunbRot) * sunOrbit, sin(sunbRot) * sunOrbit, 2.1f);
		glUniform1f(glGetUniformLocation(drawing_glProgram, "sceneDataSpheres[4].radius"), 4.0f);
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataSpheres[4].material.diffuse"), 0.8f, 0.2f, 0.0f);
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataSpheres[4].material.emission"), 3.2f, 1.5f, 0.0f);

		//Moon
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataSpheres[1].position"), cos(moonRot) * moonOrbit, sin(moonRot) * moonOrbit, 2.1f);
		glUniform1f(glGetUniformLocation(drawing_glProgram, "sceneDataSpheres[1].radius"), 4.0f);
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataSpheres[1].material.diffuse"), 0.32f, 0.35f, 0.42f);
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataSpheres[1].material.emission"), 0.0f, 0.0f, 0.0f);

		//Venus
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataSpheres[2].position"), cos(venusRot) * venusOrbit, sin(venusRot) * venusOrbit, 6.2f);
		glUniform1f(glGetUniformLocation(drawing_glProgram, "sceneDataSpheres[2].radius"), 2.0f);
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataSpheres[2].material.diffuse"), 0.4f, 0.4f, 1.0f);
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataSpheres[2].material.emission"), 0.0f, 0.0f, 0.0f);

		//Shader
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataSpheres[3].position"), cos(shaderRot) * shaderOrbit, sin(shaderRot) * shaderOrbit, -3.5f);
		glUniform1f(glGetUniformLocation(drawing_glProgram, "sceneDataSpheres[3].radius"), 3.2f);
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataSpheres[3].material.diffuse"), 0.32f, 0.35f, 0.42f);
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataSpheres[3].material.emission"), 0.0f, 0.0f, 0.0f);
		break;
	}
	case(3): {
		float sunRot = timer * 0.64f + F_TAU / 2.0f;
		float sunOrbit = 4.2f;

		glUniform1i(glGetUniformLocation(drawing_glProgram, "planesCount"), 1);
		glUniform1i(glGetUniformLocation(drawing_glProgram, "spheresCount"), 1);
		glUniform1i(glGetUniformLocation(drawing_glProgram, "ringsCount"), 1);

		//Grass
		glUniform1f(glGetUniformLocation(drawing_glProgram, "sceneDataPlanes[0].originDistance"), -6.7f);
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataPlanes[0].normal"), 0.0f, 0.0f, 1.0f);
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataPlanes[0].material.diffuse"), 0.8f, 0.8f, 0.8f);
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataPlanes[0].material.emission"), 0.0f, 0.0f, 0.0f);

		//Sun
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataSpheres[0].position"), cos(sunRot) * sunOrbit, sin(sunRot) * sunOrbit, 4.1f);
		glUniform1f(glGetUniformLocation(drawing_glProgram, "sceneDataSpheres[0].radius"), 7.0f);
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataSpheres[0].material.diffuse"), 0.82f, 0.82f, 1.0f);
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataSpheres[0].material.emission"), 4.0f, 4.0f, 4.0f);
		
		//Ring
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataRings[0].position"), 0.0f, 0.0f, -4.2f);
		glUniform1f(glGetUniformLocation(drawing_glProgram, "sceneDataRings[0].innerRadius"), 3.2f);
		glUniform1f(glGetUniformLocation(drawing_glProgram, "sceneDataRings[0].outerRadius"), 6.4f);
		glUniform1f(glGetUniformLocation(drawing_glProgram, "sceneDataRings[0].plane.originDistance"), -4.2f);
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataRings[0].plane.normal"), 0.0f, 0.0f, 1.0f);
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataRings[0].plane.material.diffuse"), 0.8f, 0.1f, 0.1f);
		glUniform3f(glGetUniformLocation(drawing_glProgram, "sceneDataRings[0].plane.material.emission"), 0.0f, 0.0f, 0.0f);
		break;
	}
	default:
		break;
	}
}

void drawing_sendControls(Vector3 cP, Vector3 cRx, Vector3 cRy, Vector3 cRz, float lC, int sC, int mE){
	GLint cpul = glGetUniformLocation(drawing_glProgram, "cameraPosition");
	glUniform3f(cpul, cP.x, cP.y, cP.z);
	GLint cxul = glGetUniformLocation(drawing_glProgram, "cameraRx");
	glUniform3f(cxul, cRx.x, cRx.y, cRx.z);
	GLint cyul = glGetUniformLocation(drawing_glProgram, "cameraRy");
	glUniform3f(cyul, cRy.x, cRy.y, cRy.z);
	GLint czul = glGetUniformLocation(drawing_glProgram, "cameraRz");
	glUniform3f(czul, cRz.x, cRz.y, cRz.z);
	GLint llul = glGetUniformLocation(drawing_glProgram, "lightLevel");
	glUniform1f(llul, lC);
	GLint meul = glGetUniformLocation(drawing_glProgram, "monteEnabled");
	glUniform1i(meul, mE);
	GLint scul = glGetUniformLocation(drawing_glProgram, "sampleCount");
	glUniform1i(scul, sC);
}

static void drawing_drawSequence(GLuint texture, int from, int count){
	if (count == 0)
		return;
	glBindTexture(GL_TEXTURE_2D, texture);
	GLint tul = glGetUniformLocation(drawing_glProgram, "tex");
	glUniform1i(tul, 0);
	GLint ctul = glGetUniformLocation(drawing_glProgram, "time");
	glUniform1f(ctul, timer);
	drawing_updateScene(drawing_sceneId);
	timer += 0.01f;
	glDrawArrays(GL_TRIANGLES, from, count);
}

void drawing_update(){
	glBindBuffer(GL_ARRAY_BUFFER, drawing_glVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLVertex) * drawing_glVertices.length, list_get(&drawing_glVertices, 0), GL_STATIC_DRAW);
	glActiveTexture(GL_TEXTURE0);

	glClear(GL_COLOR_BUFFER_BIT);

	GLint storedTexture;
	int hook = 0;
	for (int i = 0; i < drawing_glVertices.length; i += 3){
		bool lastVertex = i == drawing_glVertices.length - 3;
		GLVertex* vertex = (GLVertex*)list_get(&drawing_glVertices, i);
		if (storedTexture != vertex->texture){
			drawing_drawSequence(storedTexture, hook, i - hook);
			storedTexture = vertex->texture;
			hook = i;
		}
	}
	drawing_drawSequence(storedTexture, hook, drawing_glVertices.length - hook);
	list_clear(&drawing_glVertices);

	for (int i = 0; i < drawing_glSlaughterhouse.length; ++i){
		Texture* t = *((Texture**)list_get(&drawing_glSlaughterhouse, i));
		glDeleteTextures(1, &t->name);
		free(t);
	}
	list_clear(&drawing_glSlaughterhouse);

	SDL_GL_SwapWindow(drawing_window);
}

void drawing_terminate(){
	SDL_DestroyWindow(drawing_window);
	SDL_Quit();
}
