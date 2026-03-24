#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "raylib.h"
#include "boids_types.h"
#include "boids_core.h"
#include "boids_manager.h"
#include "boids_simulation.h"
#include <time.h>
#ifdef _OPENMP
    #include <omp.h>
#endif

double get_wall_time() {
    #ifdef _OPENMP
    return omp_get_wtime();
    #else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
    #endif
}

double get_cpu_time() {
    return (double)clock() / CLOCKS_PER_SEC;
}

void draw_boids(const BoidSystem* boids, int num_boids) {
    for (int i = 0; i < num_boids; i++) {
        DrawRectangleV((Vector2){ B_X(boids, i), B_Y(boids, i) }, (Vector2){4, 4}, BLUE);
    }
}

void set_window(const int num_boids) {
    char title[128];
    #ifdef USE_SOA
    const char* layout = "SoA";
    #else
    const char* layout = "AoS";
    #endif
    #if defined(USE_ATOMIC)
    const char* sync = "Atomic";
    #elif defined(_OPENMP)
    const char* sync = "Histogram";
    #else
    const char* sync = "Sequential";
    #endif
    snprintf(title, sizeof(title), "Boids [%d] | %s | %s", num_boids, layout, sync);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, title);
    SetTargetFPS(60);
}

SimulationResult boids_simulation(const int num_boids, const int frames, bool show_graphics, bool return_data) {
    // Double Buffering per evitare conflitti di lettura/scrittura durante l'aggiornamento dei Boids
    // e struttura aggiuntiva per mantenere i boids ordinati per cella
    BoidSystem struct_old_boids, struct_new_boids;

    init_boids_system(&struct_old_boids, num_boids);
    init_boids_system(&struct_new_boids, num_boids);
    BoidSystem *old_boids = &struct_old_boids;
    BoidSystem *new_boids = &struct_new_boids;

    // Definizione del contesto della griglia per passare facilmente le strutture dati correlate alla griglia alle funzioni
    SimulationContext sim;
    init_simulation_context(&sim, num_boids);

    // Inizializza i Boids con posizioni e velocità randomiche
    init_boids(old_boids, num_boids);

    if (show_graphics) {
        set_window(num_boids);
    }

    // Inizio misurazione tempo
    double wall_start = get_wall_time();
    double cpu_start  = get_cpu_time();

    int frame = 0;
    // Loop principale: continua finché la finestra è aperta (se show_graphics) o fino a raggiungere il numero di frame (se non show_graphics)
    while (show_graphics ? !WindowShouldClose() : frame < frames) {
        const float dt = show_graphics ? GetFrameTime() : 0.016f;

        update_indices(old_boids, num_boids, &sim);
        // Per ridurre l'overhead di ordinamento, lo facciamo solo ogni SORTING_FREQUENCY frame.
        // Questo è un compromesso tra avere boids più vicini in memoria e il costo di ordinamento.
        if (frame % SORTING_FREQUENCY == 0) {
            sort_boids_by_cell(old_boids, num_boids, sim.sorted_ind, new_boids);
            BoidSystem *temp = old_boids;
            old_boids = new_boids;
            new_boids = temp;
        }

        // Estrazione parametri per il loop parallelo (evita overhead di dereferenziazione struct)
        const float p_sq = sim.protected_range_sq;
        const float v_sq = sim.visual_range_sq;
        const int g_rows = sim.grid_rows;
        const int g_cols = sim.grid_cols;
        const int* restrict c_cell = sim.counting_cell;
        const int* restrict c_offs = sim.cell_offsets;
        const int* restrict s_ind  = sim.sorted_ind;

        #pragma omp parallel for schedule(runtime)
        for (int i = 0; i < num_boids; i++) {
            // Applica le regole Separation, Alignment e Cohesion
            // ed i cambi di direzione quando è sul bordo, controlla i limiti di velocità e aggiorna la posizione
            update_and_move_boid(i, old_boids, new_boids, dt, &sim, p_sq, v_sq, g_rows, g_cols, c_cell, c_offs, s_ind);
        }

        if (show_graphics) {
            BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawRectangleLines(MARGIN_X, MARGIN_Y, SCREEN_WIDTH - 2 * MARGIN_X,
                            SCREEN_HEIGHT - 2 * MARGIN_Y, LIGHTGRAY);
            draw_boids(new_boids, num_boids);
            DrawFPS(10, 10);
            EndDrawing();
        }
        frame++;

        // Swap
        BoidSystem *temp = old_boids;
        old_boids = new_boids;
        new_boids = temp;
    }

    double wall_end = get_wall_time();
    double cpu_end  = get_cpu_time();

    if (show_graphics) CloseWindow();

    // Salvataggio posizioni finali per verifica correttezza
    Boid *dump = NULL;
    if (return_data) {
        dump = malloc(num_boids * sizeof(Boid));
        for (int i = 0; i < num_boids; i++) {
            // Le tue macro astraggono la lettura, che sia AoS o SoA!
            dump[i].x  = B_X(old_boids, i);
            dump[i].y  = B_Y(old_boids, i);
            dump[i].vx = B_VX(old_boids, i);
            dump[i].vy = B_VY(old_boids, i);
        }
    }

    // free
    free_boids_system(&struct_old_boids);
    free_boids_system(&struct_new_boids);
    free_simulation_context(&sim);

    return (SimulationResult){ wall_end - wall_start, cpu_end - cpu_start, dump};; // Per lo speed-up
}
