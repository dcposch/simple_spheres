#include "stdlib.h"
#include "math.h"
#include "geometry.h"

float dot3(const f3 *a, const f3 *b){
    return
        a->x*b->x +
        a->y*b->y +
        a->z*b->z;
}

float dot4(const f4 *a, const f4 *b){
    return
        a->x*b->x +
        a->y*b->y +
        a->z*b->z +
        a->w*b->w;
}

f4 mult4(const float* matrix, const f4 *other){
    return (f4){
        dot4((f4*)(matrix +  0), other),
        dot4((f4*)(matrix +  4), other),
        dot4((f4*)(matrix +  8), other),
        dot4((f4*)(matrix + 12), other)};
}

float norm3(const f3*a){
    return sqrtf(dot3(a, a));
}

float norm4(const f4*a){
    return sqrtf(dot3((f3*)a, (f3*)a)) / a->w;
}

void normalize3(f3*a){
    float n = norm3(a);
    a->x /= n;
    a->y /= n;
    a->z /= n;
}

void normalize4(f4*a){
    float n = norm4(a);
    a->w = n;
}

float intersect_sphere(const ray *ray, const sphere *s, float max_t, f3 *intersection, f3 *normal){
    /* compute ray-sphere intersection */
    f3 eye = ray->start;
    f3 dst = (f3){s->center.x-eye.x, s->center.y-eye.y, s->center.z-eye.z};
    float t = dot3(&dst, &ray->direction);
    /* is it in front of the eye? */
    if(t <= 0)
        return -1;
    /* depth test */
    float d = t*t - dot3(&dst, &dst) + s->radius*s->radius;
    /* does it intersect the sphere? */
    if(d <= 0)
        return -1;
    /* is it closer than the closest thing we've hit so far */
    t -= sqrt(d);
    if(t >= max_t)
        return -1;
    
    /* if so, then we have an intersection */
    *intersection = (f3){
        eye.x+t*ray->direction.x,
        eye.y+t*ray->direction.y,
        eye.z+t*ray->direction.z};
    *normal = (f3){
        (intersection->x - s->center.x)/s->radius,
        (intersection->y - s->center.y)/s->radius,
        (intersection->z - s->center.z)/s->radius};
    return t;
}

