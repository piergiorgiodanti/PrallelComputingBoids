#include "boids_types.h"

void free_boids_system(BoidSystem *b);
void free_simulation_context(const SimulationContext *ctx);
void init_boids_system(BoidSystem *b, int num_boids);
void init_boids(const BoidSystem* boids, int num_boids);
void init_simulation_context(SimulationContext *ctx, int num_boids);