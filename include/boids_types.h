#ifndef BOIDS_TYPES_H
#define BOIDS_TYPES_H

#define SCREEN_WIDTH      1200
#define SCREEN_HEIGHT     800
#define MARGIN_X          200
#define MARGIN_Y          150

#include <stddef.h>
#include "raylib.h"

// Struttura per AoS
typedef struct {
    float x, y, vx, vy;
} Boid;

typedef struct {
#ifdef USE_SOA
    float *x; float *y; float *vx; float *vy;
#else
    Boid *data;
#endif
    int count;
} BoidSystem;

// MACRO DI ACCESSO
#ifdef USE_SOA
#define B_X(b, i)  ((b)->x[(i)])
#define B_Y(b, i)  ((b)->y[(i)])
#define B_VX(b, i) ((b)->vx[(i)])
#define B_VY(b, i) ((b)->vy[(i)])
#else
#define B_X(b, i)  ((b)->data[(i)].x)
#define B_Y(b, i)  ((b)->data[(i)].y)
#define B_VX(b, i) ((b)->data[(i)].vx)
#define B_VY(b, i) ((b)->data[(i)].vy)
#endif

typedef struct {
    int grid_rows; int grid_cols; int total_cells; int num_threads;
    int *counting_cell; int *cell_offsets; int *sorted_ind;
    int *temp_offsets;
    int **local_histograms; int **local_offsets;
    float protected_range_sq; float visual_range_sq;
    float left_margin; float right_margin; float top_margin; float bottom_margin;
} SimulationContext;

// Fattori di simulazione
static const float AVOID_FACTOR      = 0.05f;
static const float MATCHING_FACTOR   = 0.05f;
static const float CENTERING_FACTOR  = 0.0005f;
static const float TURN_FACTOR       = 5.0f;
static const float MAX_SPEED         = 400.0f;
static const float MIN_SPEED         = 200.0f;
static const int   SORTING_FREQUENCY = 10;

#endif