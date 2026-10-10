#version 150

in vec4 colorForFS;
in vec2 textureCoordinatesForFS;

uniform sampler2D tex;
uniform int screenWidth;
uniform int screenHeight;
uniform int rvltCounterOffset;
uniform float time;
uniform vec3 cameraPosition;
uniform vec3 cameraRx;
uniform vec3 cameraRy;
uniform vec3 cameraRz;
uniform float lightLevel;
uniform int monteEnabled;
uniform int sampleCount;

struct Material {
	vec3 diffuse;
	vec3 emission;
};

struct Sphere {
	vec3 position;
	float radius;
	Material material;
};

struct Plane {
	float originDistance;
	vec3 normal;
	Material material;
};

struct Ring {
	Plane plane;
	vec3 position;
	float innerRadius;
	float outerRadius;
};

uniform Plane sceneDataPlanes[7];
uniform int planesCount;
uniform Sphere sceneDataSpheres[7];
uniform int spheresCount;
uniform Ring sceneDataRings[2];
uniform int ringsCount;

const float MAX_LIGHT = 12.0;
const float TAU = 6.2831853071;
const float PI = TAU / 2;

float rand(vec2 n) { 
	return fract(sin(dot(n, vec2(12.9898, 4.1414))) * 43758.5453);
}

float sampleCounter;
vec3 obtainRV(){
	// return vec3(rand(vec2(sampleCounter++, sampleCounter++)) * 2.0 - 1.0, 
	//             rand(vec2(sampleCounter++, sampleCounter++)) * 2.0 - 1.0, 
	//             rand(vec2(sampleCounter++, sampleCounter++)) * 2.0 - 1.0);

	// float r1 = rand(vec2(sampleCounter++, sampleCounter)) * TAU;
	// float r2 = rand(vec2(sampleCounter++, sampleCounter)) * TAU;
	// return vec3(-sin(r1) * cos(r2), cos(r1) * cos(r2), sin(r2));

	// float x = rand(vec2(sampleCounter++, sampleCounter)) * TAU;
	// float y = rand(vec2(sampleCounter++, sampleCounter)) * 2.0 - 1.0;
	
	float g = 1.32471795724474602596;
	float a1 = 1.0 / g;
	float a2 = 1.0 / (g * g);
	float x = mod((0.5 + a1 * sampleCounter), 1.0) * TAU;
	float y = mod((0.5 + a2 * sampleCounter), 1.0) * 2.0 - 1.0;
	sampleCounter += 1;
	float p = sqrt(1.0 - y * y);
	return vec3(cos(x) * p, sin(x) * p, y);
}

float project(vec3 v0, vec3 v1){ //0 on 1
	return dot(v0, v1) / dot(v1, v1);
}


struct Camera{
	vec3 position;
	vec3 rx;
	vec3 ry;
	vec3 rz;
	float vFOV;
	float planeH;
	float planeW;// = tan(vFOV / 2) * 2, planeW = (framebuffer.w * planeH) / framebuffer.h;
	float pixsize;
} camera;

Camera createCamera(float vfov, vec3 pos){
	Camera c;
	c.vFOV = vfov; 
	c.position = pos;
	c.planeH = tan(c.vFOV / 2) * 2;
	c.planeW = screenWidth * c.planeH / screenHeight;
	c.pixsize = c.planeH / screenHeight;

	return c;
}

void cameraRotate(float rotation){
	camera.ry = normalize(vec3(sin(rotation), cos(rotation), 0.0) - camera.position);
	camera.rx = normalize(cross(camera.ry, camera.rz));
}

vec3 camera_GetRayDirForPixel(Camera c, float x, float y){
	return normalize((-.5f * c.planeW + (x + .5f) * c.pixsize) * cameraRx +
	                 cameraRy +
	                 (-.5f * c.planeH + (y + .5f) * c.pixsize) * cameraRz);
}

struct Ray {
	vec3 origin;
	vec3 direction;
};

Ray createRay(vec3 o, vec3 d){
	Ray r;
	r.origin = o;
	r.direction = d;
	return r;
}

Material createMaterial(vec3 d, vec3 e){
	Material m;
	m.diffuse = d;
	m.emission = e;
	return m;
}

struct RaycastResult {
	bool hasHit;
	vec3 hitPosition;
	vec3 surfaceNormal;
	float distance;
	Material material;
};

RaycastResult castSphere(Sphere s, Ray ray){
	RaycastResult rr;
	float projected = project(s.position - ray.origin, ray.direction);
	float sphereCenterToProjectedDistance = distance(ray.origin + ray.direction * projected, s.position);
	float rayToSphereShellDistance = projected - sqrt(s.radius * s.radius - sphereCenterToProjectedDistance * sphereCenterToProjectedDistance);

	if (sphereCenterToProjectedDistance > s.radius || rayToSphereShellDistance <= 0){
		rr.hasHit = false;
		return rr;
	} else {;
		rr.hasHit = true;
		rr.hitPosition = ray.origin + ray.direction * rayToSphereShellDistance;
		rr.distance = rayToSphereShellDistance;
		rr.surfaceNormal = normalize(rr.hitPosition - s.position);
		rr.material = s.material;
		return rr;
	}
}

float distanceToPlaneCore(Plane p, vec3 point){
	return project(point, p.normal) - p.originDistance;
}

float distanceToPlane(Plane p, Ray r){
	return distanceToPlaneCore(p, r.origin) /
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

RaycastResult castRing(Ring ring, Ray r){
	RaycastResult rr = castPlane(ring.plane, r);

	if (rr.hasHit){
		float d = distance(rr.hitPosition, ring.position);
		if (ring.innerRadius < d && d < ring.outerRadius){
			float dr = floor(d);
			rr.material.diffuse *= rand(vec2(dr, dr));
			return rr;
		} else {
			rr.hasHit = false;
			return rr;
		}
	} else
		return rr;
}

vec3 getRandomHemisphereDirection(vec3 normal){
	vec3 v = obtainRV();
	v = normalize(v);
	if (dot(v, normal) < 0.0)
		v *= -1.0;
	return v;
}

RaycastResult raycast(Ray r){
	RaycastResult rr, fr;
	fr.distance = -1;
	fr.hasHit = false;

	for (int i = 0; i < spheresCount; ++i){
		rr = castSphere(sceneDataSpheres[i], r);
		if (rr.hasHit && (!fr.hasHit || rr.distance < fr.distance)){
			fr = rr;
		}
	}
	for (int i = 0; i < planesCount; ++i){
		rr = castPlane(sceneDataPlanes[i], r);
		if (rr.hasHit && (!fr.hasHit || rr.distance < fr.distance)){
			fr = rr;
		}
	}
	for (int i = 0; i < ringsCount; ++i){
		rr = castRing(sceneDataRings[i], r);
		if (rr.hasHit && (!fr.hasHit || rr.distance < fr.distance)){
			fr = rr;
		}
	}
	return fr;
}

vec3 getRandomToLightSource(vec3 point) {
    vec3 randomPointOnSphere =
        sceneDataSpheres[0].position +
        sceneDataSpheres[0].radius * obtainRV();
    vec3 direction = normalize(randomPointOnSphere - point);

    return direction;
}

float sphereSurfaceArea(float radius) {
    return 4 * PI * radius * radius;
}

float circleArea(float radius) {
    return PI * radius * radius;
}

float getHitFraction(vec3 point) {
    float dist = length(point - sceneDataSpheres[0].position);

    float wholeArea = sphereSurfaceArea(dist);
    float holeArea = circleArea(sceneDataSpheres[0].radius);

    return holeArea / wholeArea;
}
    
float getMissFraction(vec3 point) {
    return 1.0 - getHitFraction(point);
}

vec3 monteCast(RaycastResult rr, int bounce, int samples){
	vec3 color = vec3(0.0, 0.0, 0.0);
	RaycastResult mcr;

	for (int i = 0; i < samples; ++i){
		Ray r = createRay(rr.hitPosition + rr.surfaceNormal * .001, getRandomToLightSource(rr.hitPosition));

        if (dot(r.direction, rr.surfaceNormal) > 0) {
            mcr = raycast(r);

            if (mcr.hasHit){
                color += mcr.material.emission;
            }
        }
	}

	color /= samples;
    color *= getHitFraction(rr.hitPosition);

	color *= rr.material.diffuse;
	color += rr.material.emission;
	return color;
}

void main() {
	sampleCounter = floor(rand(gl_FragCoord.xy) * 57217) + time;

	camera = createCamera(TAU / 8.0, cameraPosition);

	vec4 color = texture(tex, textureCoordinatesForFS) * vec4(colorForFS);
	
	if (color.x == 1.0 && color.y == 0.0 && color.z == 1.0 && color.w == 1.0){
		Ray caster = createRay(camera.position, camera_GetRayDirForPixel(camera, gl_FragCoord.x, gl_FragCoord.y));
		RaycastResult rr = raycast(caster);
		if (rr.hasHit){
			if (monteEnabled == 1){
				vec3 t = monteCast(rr, 0, sampleCount);
				color.xyz = t.xyz;
			} else {
				color.xyz = rr.material.diffuse;
			}
		} else {
			color.x = 0.0;
			color.y = 0.0;
			color.z = 0.0;
		}
	}

	gl_FragColor = color;
}
