#ifndef GEOMETRY_H
#define GEOMETRY_H

typedef struct f3 {
    float x, y, z;
} f3;

typedef struct f4 {
    float x, y, z, w;
} f4;

typedef struct vol3 {
    f3 top, bottom;
} vol3;

typedef struct sphere {
    f3 center;
    float radius;
    f3 velocity;
} sphere;

typedef struct ray {
    f3 start, direction;
} ray;

float dot3(const f3 *a, const f3 *b);
float dot4(const f4 *a, const f4 *b);
f4 mult4  (const float* matrix, const f4 *other);
float norm3(const f3 *a);
float norm4(const f4 *a);
void normalize3(f3 *a);
void normalize4(f4 *a);
float intersect_sphere(const ray *ray, const sphere *s, float max_t, f3 *intersection, f3 *normal);
#endif
