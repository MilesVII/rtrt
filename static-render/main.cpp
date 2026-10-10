//#include <cstdio>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

const float TAU = 6.28318530718f;
struct vec3{
	float x = 0, y = 0, z = 0;
	vec3(){
	}
	vec3(float xx, float yy, float zz){
		x = xx; y = yy; z = zz;
	}

	float length(){
		return sqrt(x * x + y * y + z * z);
	}

	float length2(){
		return x * x + y * y + z * z;
	}

	vec3 normalized(){
		return *this * (1 / length());
	}

	friend vec3 operator+(vec3 a, vec3 b){
		return vec3(a.x + b.x, a.y + b.y, a.z + b.z);
	}
	friend void operator+=(vec3& a, vec3 b){
		a.x += b.x; a.y += b.y; a.z += b.z;
	}
	friend vec3 operator-(vec3 a, vec3 b){
		return a + (b * -1);
	}
	friend vec3 operator-(vec3 a){
		return vec3(0, 0, 0) - a;
	}
	friend vec3 operator*(vec3 a, vec3 b){
		return vec3(a.x * b.x, a.y * b.y, a.z * b.z);
	}
	friend vec3 operator*(vec3 a, float b){
		return vec3(a.x * b, a.y * b, a.z * b);
	}
	friend vec3 operator*(float b, vec3 a){
		return vec3(a.x * b, a.y * b, a.z * b);
	}
	friend vec3 operator*=(vec3& a, float b){
		a.x *= b; a.y *= b; a.z *= b;
		return a;
	}
	friend vec3 operator*=(vec3& a, vec3 b){
		a.x *= b.x; a.y *= b.y; a.z *= b.z;
		return a;
	}
	friend vec3 operator/=(vec3& a, float b){
		a.x /= b; a.y /= b; a.z /= b;
		return a;
	}
	friend bool operator==(vec3 b, vec3 a){
		return a.x == b.x && a.y == b.y && a.z == b.z;
	}
	friend bool operator!=(vec3 b, vec3 a){
		return !(a == b);
	}
};

float distance (vec3 v1, vec3 v2){
	return (v1 - v2).length();
}

struct Ray{
	vec3 origin, direction;
	Ray(vec3 o, vec3 d){
		origin = o;
		direction = d;
	}
};

struct Material{
	vec3 diffuse, emission;

	Material(){
		diffuse = vec3();
		emission = vec3();
	};

	Material(vec3 d, vec3 e){
		diffuse = d;
		emission = e;
	}
};

float max (float a, float b){
	return a>b?a:b;
}

float dot(vec3 a, vec3 b){
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

vec3 cross(vec3 v0, vec3 v1){
	return vec3(v0.y * v1.z - v0.z * v1.y,
	               v0.z * v1.x - v0.x * v1.z,
	               v0.x * v1.y - v0.y * v1.x);
}

float project(vec3 v0, vec3 v1){ //0 on 1
	return dot(v0, v1) / dot(v1, v1);
}

float clamp(float x, float min, float max){
	if (max < min)
		std::swap<float>(min, max);
	if (x > max)
		return max;
	else if (x < min)
		return min;
	else
		return x;
}

void clamp(float* x, float min, float max){
	if (max < min)
		std::swap<float>(min, max);
	if (*x > max)
		*x = max;
	else if (*x < min)
		*x = min;
}

vec3 clamp(vec3 x, vec3 min, vec3 max){
	clamp(&x.x, min.x, max.x);
	clamp(&x.y, min.y, max.y);
	clamp(&x.z, min.z, max.z);
	return x;
}
vec3 clamp(vec3 x){
	return clamp(x, vec3(0, 0, 0), vec3(1, 1, 1));
}


float lerp (float x, float a, float b){
	return a + (b - a) * x;
}

float remap (float x, float s1, float s2, float a, float b){
	return (x - s1) / (s2 - s1) * (b - a) + a;
}

vec3 remap (float x, float s1, float s2, vec3 v1, vec3 v2){
	return (x - s1) / (s2 - s1) * (v2 - v1) + v1;
}

vec3 remapClamp (float x, float s1, float s2, vec3 v1, vec3 v2){
	vec3 r = (x - s1) / (s2 - s1) * (v2 - v1) + v1;
	//return r.normalized();
	clamp(&r.x, v1.x, v2.x);
	clamp(&r.y, v1.y, v2.y);
	clamp(&r.z, v1.z, v2.z);
	return r;
}

void unpackColor(unsigned in, float* r, float* g, float* b, float* a){
	unsigned mask = 0xFF000000;
	*a = ((mask & in) >> 6 * 4) / 255.0f;
	mask = mask >> 2 * 4;
	*b = ((mask & in) >> 4 * 4) / 255.0f;
	mask = mask >> 2 * 4;
	*g = ((mask & in) >> 2 * 4) / 255.0f;
	mask = mask >> 2 * 4;
	*r = ((mask & in)         ) / 255.0f;
}

/*vec3 unpackColor(unsigned in){
	vec3 v;
	unsigned mask = 0xFF000000;
	unpackColor(in, &v.x, &v.y, &v.z, &v.x);
	//a = ((mask & in) >> 6 * 4) / 255.0f;
	mask = mask >> 2 * 4;
	v.z = ((mask & in) >> 4 * 4) / 255.0f;
	mask = mask >> 2 * 4;
	v.y = ((mask & in) >> 2 * 4) / 255.0f;
	mask = mask >> 2 * 4;
	v.x = ((mask & in)         ) / 255.0f;
	return v;
}*/

unsigned packColor(float r, float g, float b, float a){
	return ((unsigned)(a * 255) << 6 * 4) | ((unsigned)(b * 255) << 4 * 4) | ((unsigned)(g * 255) << 2 * 4) | (unsigned)(r * 255);
}

unsigned packColor(vec3 v, float a){
	return packColor(v.x, v.y, v.z, a);
}

unsigned colorLerp(unsigned c1, unsigned c2, float x){
	float r1, g1, b1, a1, r2, g2, b2, a2;
	unpackColor(c1, &r1, &g1, &b1, &a1);
	unpackColor(c2, &r2, &g2, &b2, &a2);
	return packColor(r1 + (r2 - r1) * x,
	                 g1 + (g2 - g1) * x,
	                 b1 + (b2 - b1) * x,
	                 a1 + (a2 - a1) * x);
}

struct Sphere{
	vec3 position;
	float radius;
	Material material;
};

struct Plane{
	float originDistance;
	vec3 normal;
	Material material;

	Plane(){
		originDistance = 0;
	}

	Plane(vec3 position, vec3 direction){
		originDistance = project(position, direction);
		this->normal = direction.normalized();
	}
};

struct RaycastResult
{
	bool hasHit;
	vec3 hitPosition, surfaceNormal;
	float distance;
	Material material;
};

struct Bitmap{
	int w = 0;
	int h = 0;
	std::vector<unsigned> pixels;

	unsigned* get(int x, int y){
		if (x >= w || y >= h)
			return &pixels[0];
		return &pixels[x + y * w];
	}

	void resize(int x, int y){
		w = x; h = y;
		pixels.resize(x * y);
	}

	void flip(){
		for (int x = 0; x < w; x++)
			for (int y = 0; y < h / 2; y++)
				std::swap(*get(x, y), *get(x, h - y - 1));
	}

	void fill(unsigned color){
		for (int x = 0; x < w; x++)
			for (int y = 0; y < h; y++)
				*get(x, y) = color;
	}
} framebuffer;

struct Camera{
	vec3 position;
	vec3 rx, ry, rz;
	float vFOV;
	float planeH, planeW;// = tan(vFOV / 2) * 2, planeW = (framebuffer.w * planeH) / framebuffer.h;
	float pixsize;// = planeH / framebuffer.h;

	Camera(float vfov, vec3 pos, vec3 whereToLook, vec3 whereIsTheCeiling){
		vFOV = vfov; position = pos;
		planeH = tan(vFOV / 2) * 2;
		planeW = (framebuffer.w * planeH) / framebuffer.h;
		pixsize = planeH / framebuffer.h;

		ry = (whereToLook - position).normalized();
		rx = cross(ry, whereIsTheCeiling).normalized();
		rz = cross(rx, ry).normalized();
	}

	vec3 getRayDirForPixel(float x, float y){
		//position+(0.1.0)+(-.5, 0, -.5) +((x + .5) * pixsize, 0, (y + .5) * pixsize)

		return ((-.5f * planeW + (x + .5f) * pixsize) * rx +
		       ry +
		       (-.5f * planeH + (y + .5f) * pixsize) * rz).normalized();
	}
};

int saveToFile(Bitmap* bmp, const char* path){
	return stbi_write_png(path, bmp->w, bmp->h, 4, bmp->pixels.data(), sizeof(unsigned) * bmp->w);
}

RaycastResult castSphere(Sphere s, Ray ray){
	RaycastResult rr;
	float projected = project(s.position - ray.origin, ray.direction);
	float sphereCenterToProjectedDistance = distance(ray.origin + ray.direction * projected, s.position);//((s.position - ray.origin) - ray.direction * projected).length();//sqrt((s.position - ray.origin).length2() - projected * projected);
	float rayToSphereShellDistance = projected - sqrt(s.radius * s.radius - sphereCenterToProjectedDistance * sphereCenterToProjectedDistance);

	if (sphereCenterToProjectedDistance > s.radius || rayToSphereShellDistance <= 0){
		rr.hasHit = false;
		return rr;
	} else {;
		rr.hasHit = true;
		rr.hitPosition = ray.origin + ray.direction * rayToSphereShellDistance;
		rr.distance = rayToSphereShellDistance;
		rr.surfaceNormal = (rr.hitPosition - s.position).normalized();
		rr.material = s.material;
		return rr;
	}
}

float distanceToPlane(Plane p, vec3 point){
	return project(point, p.normal) - p.originDistance;
}

float distanceToPlane(Plane p, Ray r){
	return distanceToPlane(p, r.origin) /
	       -project(r.direction, p.normal);
}

RaycastResult castPlane(Plane p, Ray r){
	RaycastResult rr;
	rr.distance = distanceToPlane(p, r);
	if (rr.distance <= 0){
		rr.hasHit = false;
		return rr;
	} else {
		rr.hasHit =true;
		rr.hitPosition = r.origin + r.direction * rr.distance;
		rr.surfaceNormal = p.normal;
		rr.material = p.material;
		return rr;
	}
}

vec3 getRandomHemisphereDirection(vec3 normal){
	vec3 v;
	do
		v = vec3(rand() / (float)RAND_MAX * 2.0f - 1.0f,
		            rand() / (float)RAND_MAX * 2.0f - 1.0f,
					rand() / (float)RAND_MAX * 2.0f - 1.0f);
	while(v == vec3(0, 0, 0) || v.length2() > 1);
	v = v.normalized();
	if (dot(v, normal) < 0)
		v *= -1.0f;
	return v;
}

RaycastResult cast(Ray r, std::vector<Sphere> spheres, std::vector<Plane> planes){
	RaycastResult rr, fr;
	fr.distance = -1;
	fr.hasHit = false;

	for (unsigned i = 0; i < spheres.size(); i++){
		rr = castSphere(spheres[i], r);
		if (rr.hasHit && (!fr.hasHit || rr.distance < fr.distance)){
			fr = rr;
		}
	}
	for (unsigned i = 0; i < planes.size(); i++){
		rr = castPlane(planes[i], r);
		if (rr.hasHit && (!fr.hasHit || rr.distance < fr.distance)){
			fr = rr;
		}
	}
	return fr;
}

vec3 monteCast(RaycastResult rr, std::vector<Sphere> spheres, std::vector<Plane> planes, int bounce, int monte){
	vec3 color = vec3(0, 0, 0);
	RaycastResult mcr;
	for (int i = 0; i < monte; ++i){
		mcr = cast(Ray(rr.hitPosition + rr.surfaceNormal * .001f, getRandomHemisphereDirection(rr.surfaceNormal)), spheres, planes);
		if (mcr.hasHit){
			if (bounce == 0)
				color += mcr.material.emission;
			else
				color += monteCast(mcr, spheres, planes, bounce - 1, monte);
		}
	}
	color /= (float) monte;
	color *= rr.material.diffuse;
	color += rr.material.emission;
	return color;
}

void render(Camera camera, std::vector<Sphere> spheres, std::vector<Plane> planes){
	framebuffer.fill(0xFF000000);
	RaycastResult rr, monte;
	vec3 color;
	for (int x = 0; x < framebuffer.w; x++)
		for (int y = 0; y < framebuffer.h; y++){
			rr = cast(Ray(camera.position, camera.getRayDirForPixel(x, y)), spheres, planes);
			if (rr.hasHit){
				color = monteCast(rr, spheres, planes, 0, 1500);
			} else color = vec3(0, 0, 0);
			*framebuffer.get(x, y) = packColor(clamp(color), 1);
			std::cout << "\r";
			std::cout << (framebuffer.h * x + y) * 100 / (framebuffer.w * framebuffer.h);
			std::cout << "%                " << std::flush;
		}
}

int main(){
	int c = 0;
	std::cout << c++ << c << std::endl;
	return 0;
	std::cout << "Starting..." << std::flush;
	framebuffer.resize(800, 600);
	Camera camera = Camera(TAU / 8, vec3(0, 0, 0), vec3(0, 1, 0), vec3(0, 0, 1));
	std::vector<Sphere> balls;
	balls.resize(3);
	balls[0].position = vec3(-2.3f, -1, 0);//Sun
	balls[0].radius = 1;
	balls[0].material = Material(vec3(1, 0, 0), vec3(126, 96, 75));

	balls[1].position = vec3(0, 4, .51f);//Moon
	balls[1].radius = .5f;
	balls[1].material = Material(vec3(.32f, .35f, .42f), vec3(0, 0, 0));

	balls[2].position = vec3(1, 7, -.9f);//Venus
	balls[2].radius = 2;
	balls[2].material = Material(vec3(.4f, .4f, 1), vec3(0, 0, 0));

	//balls[3].position = vec3(0, 30, -18);//Earth
	//balls[3].radius = 20;
	//balls[3].material = Material(vec3(.4f, .4f, 1), vec3(0, 0, 0));

	std::vector<Plane> planes;
	planes.resize(0);
	//planes[0] = Plane(vec3(0, 0, -3), vec3(0, 0, 1));
	//planes[0].material = Material(vec3(.7f, .7f, .7f), vec3(0, 0, 0));

	render(camera, balls, planes);
	framebuffer.flip();
	std::cout << saveToFile(&framebuffer, "images.png");
	return 0;
}