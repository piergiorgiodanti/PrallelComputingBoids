#include <stdbool.h>

typedef struct {
    double wall_time;
    double cpu_time;
    Boid *final_dump;
} SimulationResult;

SimulationResult boids_simulation(int num_boids, int frames, bool show_graphics, bool return_data);