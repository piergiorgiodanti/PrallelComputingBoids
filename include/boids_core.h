#include "boids_types.h"

void update_and_move_boid(const int current_idx, const BoidSystem* old_boids, BoidSystem* new_boids, const float dt, const SimulationContext *ctx,
                          const float p_range_sq, const float v_range_sq, const int g_rows, const int g_cols, const int* restrict c_cell,
                          const int* restrict c_offsets, const int* restrict s_ind);
void update_indices(const BoidSystem* boids, int num_boids, SimulationContext *ctx);
void sort_boids_by_cell(const BoidSystem* boids, int num_boids, int *sorted_ind, const BoidSystem* sorted_boids);