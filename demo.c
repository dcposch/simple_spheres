#include <stdlib.h>
#include <SDL/SDL.h>
#include <time.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#include "geometry.h"

#define WIDTH 640
#define HEIGHT 480 
#define BPP 4
#define GRID_SIZE 2
#define RADIUS 0.2
#define TARGET_US 16666 /* 16.6 ms / frame = 60 fps */

typedef struct light {
    f4 location;
    f3 color;
} light;

typedef struct graphics_state {
    /* params */
    int frame;
    float projection_matrix[16];
    int width, height;
    int *stencil;
    SDL_Surface *screen;

    /* lighting */
    light *lights;
    int light_count;

    /* scene */
    sphere *object_list;
    int object_count;
    vol3 bounds;

    /* workers */
    int num_workers;
    pthread_t *worker_threads;
    int *worker_pixel_ranges;
    sem_t sema_start_render;
    sem_t sema_finish_render;
} graphics_state;

void set_pixel(graphics_state *state, int x, int ytimesw, Uint8 r, Uint8 g, Uint8 b)
{
    Uint32 colour = SDL_MapRGB( state->screen->format, r, g, b );
    ((Uint32*)state->screen->pixels)[x + ytimesw] = colour;
}

f3 expose(f3 incident, float e){
    return (f3){
        (float)(1.0 - exp(-e*incident.x)),
        (float)(1.0 - exp(-e*incident.y)),
        (float)(1.0 - exp(-e*incident.z))
    };
}

typedef struct graphics_worker_arg {
    int i;
    graphics_state *state;
} graphics_worker_arg;

float intersect(const ray *ray, const graphics_state *state, f3 *intersection, f3 *normal){
    /* main raytrace loop: go through geometry, testing for intersections */
    int i;
    float tmin = 1000000;
    for(i = 0; i < state->object_count; i++){
        /* get geometry */
        sphere *s = state->object_list + i;
        float t = intersect_sphere(ray, s, tmin, intersection, normal);
        if(t > 0 && t < tmin)
            tmin = t;
    }
    return (tmin < 100000) ? tmin : -1;
}
float trace(const ray *ray, const graphics_state *state, f3 *light, int max_depth){
    *light = (f3){0,0,0};
    f3 intersection, normal;
    float t = intersect(ray, state, &intersection, &normal);
    if(t < 0)
        return -1;

    /* shade; no shadows for now */
    f3 specular = (f3){0.4f, 0.4f, 0.4f};
    f3 diffuse = (f3){1,1,0.2f};
    int j;
    for(j = 0; j < state->light_count; j++){
        f4 light_location = state->lights[j].location;
        f3 light_direction = (f3){
            light_location.x - intersection.x*light_location.w,
            light_location.y - intersection.y*light_location.w,
            light_location.z - intersection.z*light_location.w};
        /* shadows -- these are slow.
        struct ray light_ray;
        light_ray.start = intersection;
        light_ray.direction = light_direction;
        f3 tmp_intersect, tmp_normal; 
        if(intersect(&light_ray, state, &tmp_intersect, &tmp_normal) > 0)
            continue;*/
        float cos_incident = dot3(&normal, &light_direction) / norm3(&light_direction);
        if(cos_incident < 0)
            cos_incident = 0;
        cos_incident += 0.04f;
        light->x += diffuse.x*cos_incident*state->lights[j].color.x;
        light->y += diffuse.y*cos_incident*state->lights[j].color.y;
        light->z += diffuse.z*cos_incident*state->lights[j].color.z;
    }

    /* recurse to get specular reflection */
    if(max_depth > 0){
        float bounce = dot3(&ray->direction, &normal);
        struct ray newRay;
        newRay.direction = (f3){
            ray->direction.x - 2*bounce*normal.x,
            ray->direction.y - 2*bounce*normal.y,
            ray->direction.z - 2*bounce*normal.z};
        newRay.start = intersection;
        f3 newLight;
        trace(&newRay, state, &newLight, max_depth-1);
        /* special case: specular highlights */
        int j;
        for(j = 0; j < state->light_count; j++){
            f4 light_location = state->lights[j].location;
            f3 light_direction = (f3){
                light_location.x - intersection.x*light_location.w,
                light_location.y - intersection.y*light_location.w,
                light_location.z - intersection.z*light_location.w};
            float light_cos = dot3(&light_direction, &newRay.direction);
            float thresh = 0.85;
            if(light_cos > thresh){
                float highlight = (light_cos - thresh)/(1-thresh);
                highlight *= highlight * highlight * 3;
                newLight.x += highlight*state->lights[j].color.x;
                newLight.y += highlight*state->lights[j].color.y;
                newLight.z += highlight*state->lights[j].color.z;
            }
        }
        light->x += specular.x*newLight.x;
        light->y += specular.y*newLight.y;
        light->z += specular.z*newLight.z;
    }
    return t;
}

f3 graphics_render_pixel(graphics_state *state, int x, int y){
    /* antialiasing */
    f3 avg_light = (f3){0,0,0};
    int i;
    int antialias = 4;
    for(i = 0; i < antialias; i++){
        f3 normalized = (f3){ 
            (((float)x + (float)(i%2)/4) / state->width)*2-1, 
            1-(((float)y  + (float)(i/2)/4)/ state->height)*2, 
            0 };
        ray r;
        r.start = *((f3*)state->projection_matrix + 3);
        r.direction = (f3) { 
            normalized.x/2, 
            normalized.y/2, 
            sqrtf(1.0 - (normalized.x*normalized.x + normalized.y*normalized.y)/4) };
        f3 incident_light;
        trace(&r, state, &incident_light, 1);
        avg_light.x += incident_light.x/antialias;
        avg_light.y += incident_light.y/antialias;
        avg_light.z += incident_light.z/antialias;
    }
    f3 color = expose(avg_light, 1);
    return color;
}

void* graphics_worker_thread(graphics_worker_arg *arg){
    printf("worker thread %d started\n", arg->i);
    graphics_state *state = arg->state;
    int x0 = state->worker_pixel_ranges[4*arg->i];
    int y0 = state->worker_pixel_ranges[4*arg->i + 1];
    int x1 = state->worker_pixel_ranges[4*arg->i + 2];
    int y1 = state->worker_pixel_ranges[4*arg->i + 3];

    /* raytrace */
    while(1){
        sem_wait(&state->sema_start_render);

        int x, y;
        for(y = y0; y < y1; y++) 
        {
            int ytimesw = y * state->width;
            for(x = x0; x < x1; x++) 
            {
                if(!state->stencil[x + ytimesw]){
                    set_pixel(state, x, ytimesw, 0, 0, 0);
                    continue;
                }

                f3 color = graphics_render_pixel(state, x, y);
                set_pixel(state, x, ytimesw, 
                    (Uint8)(color.x*255.9f),
                    (Uint8)(color.y*255.9f),
                    (Uint8)(color.z*255.9f));
            }
        }

        /* let the main thread know we're done rendering */
        sem_post(&state->sema_finish_render);
    }
    return NULL;
}

void add_sphere_to_stencil(sphere *s, graphics_state *state){
    f3 *eye = (f3*)state->projection_matrix + 3;
    f3 diff = (f3){
        s->center.x - eye->x, 
        s->center.y - eye->y, 
        s->center.z - eye->z};
    int x0 = (int)state->width *(0.5f+((diff.x-RADIUS)/norm3(&diff))) - 1;
    int x1 = (int)state->width *(0.5f+((diff.x+RADIUS)/norm3(&diff))) + 1;
    int y0 = (int)state->height*(0.5f-((diff.y+RADIUS)/norm3(&diff))) - 1;
    int y1 = (int)state->height*(0.5f-((diff.y-RADIUS)/norm3(&diff))) + 1;
    if(x0 < 0) x0 = 0;
    if(x1 >= state->width) x1 = state->width - 1;
    if(y0 < 0) y0 = 0;
    if(y1 >= state->height) y1 = state->height - 1;

    int x,y;
    for(y = y0; y <= y1; y++)
        for(x = x0; x <= x1; x++)
            state->stencil[x + y*state->width] = 1;
}

int draw_screen(graphics_state *state){
    SDL_Surface *screen = state->screen;
    if(SDL_MUSTLOCK(screen) && SDL_LockSurface(screen) < 0) 
        return 1;

    /* calculate stencil buffer, so we don't have to raytrace the whole screen */
    memset(state->stencil, 0, state->width*state->height*sizeof(int));
    int i;
    for(i = 0; i < state->object_count; i++){
        add_sphere_to_stencil(state->object_list + i, state);
    }

    /* tell the worker threads to start rendering, then wait for them to finish */
    for(i = 0; i < state->num_workers; i++)
        sem_post(&state->sema_start_render);
    for(i = 0; i < state->num_workers; i++)
        sem_wait(&state->sema_finish_render);

    /* swap buffers */
    if(SDL_MUSTLOCK(screen))
        SDL_UnlockSurface(screen);
    SDL_Flip(screen); 
    return 0;

}

long long total_ns(const struct timespec *t){
    return (long long)t->tv_sec*1000000000 + t->tv_nsec;
}

int init_graphics(graphics_state *state){
    state->frame = 0;
    float matrix[16] = 
        {1.0f, 0.0f, 0.0f, 0.0f,
         0.0f, 1.0f, 0.0f, 0.0f,
         0.0f, 0.0f, 1.0f, 0.0f,
         0.0f, 0.0f, 0.0f, 0.0f};
    memcpy(state->projection_matrix, matrix, sizeof(matrix)); 
    state->width = WIDTH;
    state->height = HEIGHT;
    state->stencil = malloc(sizeof(int)*state->width*state->height);
    return 0;
}

int init_lights(graphics_state *state){
    state->light_count = 2;
    state->lights = malloc(sizeof(light)*state->light_count);
    state->lights[0].color = (f3){1,1,1};
    state->lights[1].color = (f3){0,0,1};

    return 0;
}

int init_model(graphics_state *state){
    /* give it a bounding box */
    state->bounds = (vol3){
    (f3){-1.5f,-1.5f,-1},
    (f3){1.5f,1.5f,1}};

    /* fill it with spheres */
    srand(time(NULL));
    state->object_count = GRID_SIZE*GRID_SIZE*GRID_SIZE;
    state->object_list = malloc(state->object_count*sizeof(sphere));
    int i,j,k;
    for(i = 0; i < GRID_SIZE; i++){
        for(j = 0; j < GRID_SIZE; j++){
            for(k = 0; k < GRID_SIZE; k++){
                /* generate geometry */
                sphere *s = state->object_list + k*GRID_SIZE*GRID_SIZE + j*GRID_SIZE + i;
                s->center = (f3){i - 0.5f*(GRID_SIZE-1), j - 0.5f*(GRID_SIZE-1), k - 0.5f*(GRID_SIZE-1)};
                s->radius = RADIUS;
                s->velocity = (f3){
                    (float)(rand()%1000)/30000, 
                    (float)(rand()%1000)/30000, 
                    (float)(rand()%1000)/30000}; 
            }
        }
    }
    return 0;
}

int init_worker_threads(graphics_state *state){
    sem_init(&state->sema_start_render,0,0);
    sem_init(&state->sema_finish_render,0,0);
    state->num_workers = 2;
    state->worker_threads = malloc(sizeof(pthread_t)*state->num_workers);
    state->worker_pixel_ranges = malloc(sizeof(int)*4*state->num_workers);
    graphics_worker_arg *worker_args = malloc(sizeof(graphics_worker_arg)*state->num_workers);
    
    int i;
    for(i = 0; i < state->num_workers; i++){
        state->worker_pixel_ranges[i*4 + 0] = i*state->width/state->num_workers;
        state->worker_pixel_ranges[i*4 + 1] = 0;
        state->worker_pixel_ranges[i*4 + 2] = (i+1)*state->width/state->num_workers;
        state->worker_pixel_ranges[i*4 + 3] = state->height;
        worker_args[i] = (graphics_worker_arg){i, state};
        if(pthread_create(state->worker_threads + i, NULL, (void*(*)(void*))graphics_worker_thread, &worker_args[i]) != 0){
            perror("could not create worker thread");
            return -1;
        }
    }

    return 0;
}

float absf(float f){
    return f < 0 ? -f : f;
}

void timestep(graphics_state *state){
    int h = state->frame;
  
    /* eye, moves with time */
    *((f3*)state->projection_matrix + 3) = (f3) { 
        0, //sinf(h / 30.0f), 
        0,
        -4}; //-cosf(h / 30.0f)};

    /* so does the light */
    int i;
    for(i = 0; i < state->light_count; i++){
        light *light = state->lights + i;
        light->location = (f4){
            sinf(h / 50.0f + i)*cosf(h / 100.0f + i),
            sinf(h/100.0f + i),
            cosf(h / 50.0f + i)*cosf(h / 100.0f + i),
            0.2f};
    }

    /* so do the objs */
    for(i = 0; i < state->object_count; i++){
        sphere *s = state->object_list + i;
        s->center.x += s->velocity.x;
        s->center.y += s->velocity.y;
        s->center.z += s->velocity.z;

        /* bounce off the bounding box */
        if(s->center.x < state->bounds.top.x)    
            s->velocity.x = absf(s->velocity.x);
        if(s->center.y < state->bounds.top.y)    
            s->velocity.y = absf(s->velocity.y);
        if(s->center.z < state->bounds.top.z)    
            s->velocity.z = absf(s->velocity.z);
        if(s->center.x > state->bounds.bottom.x) 
            s->velocity.x = -absf(s->velocity.x);
        if(s->center.y > state->bounds.bottom.y) 
            s->velocity.y = -absf(s->velocity.y);
        if(s->center.z > state->bounds.bottom.z) 
            s->velocity.z = -absf(s->velocity.z);

        /* bounce off each other */
        int j;
        for(j = i+1; j < state->object_count; j++){
            sphere *s2 = state->object_list + j;
            f3 diff = (f3){
                s->center.x - s2->center.x,
                s->center.y - s2->center.y,
                s->center.z - s2->center.z};
            float dist = norm3(&diff);
            if(dist < s->radius + s2->radius){
                normalize3(&diff);
                float dot = dot3(&diff, &s->velocity);
                if(dot > 0)
                    /* if they're already flying apart,
                     * don't bounce them back towards each other */
                    continue;
                float dot2 = dot3(&diff, &s2->velocity);
                s->velocity.x  -= diff.x*(dot - dot2);
                s->velocity.y  -= diff.y*(dot - dot2);
                s->velocity.z  -= diff.z*(dot - dot2);
                s2->velocity.x -= diff.x*(dot2 - dot);
                s2->velocity.y -= diff.y*(dot2 - dot);
                s2->velocity.z -= diff.z*(dot2 - dot);
            }
        }
    }

    /* z order */
    /*for(i = 0; i < state->object_count; i++){
        int min_ix = i;
        float min_z = state->object_list[i].center.z;
        int j;
        for(j = i+1; j < state->object_count; j++){
            if(state->object_list[j].center.z < min_z){
                min_z = state->object_list[j].center.z;
                min_ix = j;
            }
        }

        if(i != min_ix){
            sphere temp = state->object_list[i];
            state->object_list[i] = state->object_list[min_ix];
            state->object_list[min_ix] = temp;
        }
    }*/
}

int main(int argc, char **argv){

    /* initialize sdl */
    if (SDL_Init(SDL_INIT_VIDEO) < 0){
        perror("could not initialize sdl");
        return 1;
    }
    SDL_ShowCursor(SDL_DISABLE);
    SDL_Surface *screen = 
        SDL_SetVideoMode(WIDTH, HEIGHT, BPP*8, SDL_FULLSCREEN|SDL_HWSURFACE|SDL_DOUBLEBUF);
    if(!screen)
    {
        perror("could not set video mode");
        SDL_Quit();
        return 1;
    }

    /* initialize graphics state */
    graphics_state state;
    state.screen = screen;
    if(init_model(&state) != 0) return -1;
    if(init_lights(&state) != 0) return -1;
    if(init_graphics(&state) != 0) return -1;
    if(init_worker_threads(&state) != 0) return -1;

    /* main loop */
    int run = 1;
    float ema_nanos = 0, ema_nanos2 = 0, max_nanos = 0;
    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    while(run){

        /* change the scene */
        timestep(&state);

        /* render the scene */
        if(draw_screen(&state)){
            perror("render error");
            return 1;
        }

        /* performance metrics */
        state.frame++;
        clock_gettime(CLOCK_MONOTONIC, &end_time);
        long nanos = total_ns(&end_time) - total_ns(&start_time);
        ema_nanos = 0.9f*ema_nanos + 0.1*nanos;
        ema_nanos2 = 0.9f*ema_nanos2 + 0.1*nanos*nanos;
        if(nanos > max_nanos) max_nanos = nanos;
        if(state.frame % 10 == 0){
            float stdev_nanos = sqrtf(ema_nanos2 - ema_nanos*ema_nanos);
            float fps = 1000000000 / ema_nanos;
            float fps_sigma = 1000000000 / (ema_nanos + stdev_nanos);
            float fps_worst = 1000000000 / max_nanos;
            printf("%.2f +/- %.2f (worst: %.2f) fps\n", 
                fps, fps-fps_sigma, fps_worst);
            max_nanos = 0;
        }
        clock_gettime(CLOCK_MONOTONIC, &start_time);
        int sleep_us = TARGET_US - nanos/1000;
        if(sleep_us > 0)
            usleep(sleep_us);

        /* check if we need to exit */
        SDL_Event event;
        while(SDL_PollEvent(&event)) 
        {
            if(event.type == SDL_QUIT || event.type == SDL_MOUSEBUTTONDOWN){// || event.type == SDL_KEYDOWN){
                run = 0;
                break;
            }
        }
    }

    /* reset resolution to normal, exit */
    SDL_SetVideoMode(1360, 768, 32, SDL_FULLSCREEN|SDL_HWSURFACE|SDL_DOUBLEBUF);
    SDL_Quit();
    return 0;
}
