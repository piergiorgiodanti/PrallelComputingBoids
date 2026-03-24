#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "boids_core.h"

#ifdef _OPENMP
    #include <omp.h>
#endif

// Restituisce l'indice lineare della cella data la posizione x, y
static inline int get_cell_index(float x, float y, int cols, int rows) {
    int c = (int)(x / (SCREEN_WIDTH / (float)cols));
    int r = (int)(y / (SCREEN_HEIGHT / (float)rows));
    if (c < 0) c = 0; else if (c >= cols) c = cols - 1;
    if (r < 0) r = 0; else if (r >= rows) r = rows - 1;
    return r * cols + c;
}

void update_and_move_boid(const int current_idx, const BoidSystem* old_boids, BoidSystem* new_boids,
                          const float dt, const SimulationContext *ctx,
                          const float p_range_sq, const float v_range_sq,
                          const int g_rows, const int g_cols,
                          const int* restrict c_cell,
                          const int* restrict c_offsets,
                          const int* restrict s_ind) {

    float close_dx = 0, close_dy = 0;
    float vel_avg_x = 0, vel_avg_y = 0;
    float pos_avg_x = 0, pos_avg_y = 0;
    int neighbors = 0;

    const float x = B_X(old_boids, current_idx);
    const float y = B_Y(old_boids, current_idx);
    float vx = B_VX(old_boids, current_idx);
    float vy = B_VY(old_boids, current_idx);

    // Calcolo riga e colonna correnti per l'iterazione 3x3
    int curr_c = (int)(x / (SCREEN_WIDTH / (float)g_cols));
    int curr_r = (int)(y / (SCREEN_HEIGHT / (float)g_rows));

    // Legge le informazioni solo dai boids nelle celle adiacenti
    for (int dr = -1; dr <= 1; dr++) {
        int tr = curr_r + dr;
        if (tr < 0 || tr >= g_rows) continue;
        for (int dc = -1; dc <= 1; dc++) {
            int tc = curr_c + dc;
            if (tc < 0 || tc >= g_cols) continue;

            int target_cell = tr * g_cols + tc;
            const int cell_start = c_offsets[target_cell];
            const int boids_in_cell = c_cell[target_cell];

            for (int j = 0; j < boids_in_cell; j++) {
                int n_idx = s_ind[cell_start + j];
                if (n_idx == current_idx) continue;

                const float dx = x - B_X(old_boids, n_idx);
                const float dy = y - B_Y(old_boids, n_idx);
                const float dist_sq = dx * dx + dy * dy;

                // Collision
                if (dist_sq < p_range_sq) {
                    close_dx += dx; close_dy += dy;
                }

                // Alignment e Cohesion
                if (dist_sq < v_range_sq) {
                    vel_avg_x += B_VX(old_boids, n_idx);
                    vel_avg_y += B_VY(old_boids, n_idx);
                    pos_avg_x += B_X(old_boids, n_idx);
                    pos_avg_y += B_Y(old_boids, n_idx);
                    neighbors++;
                }
            }
        }
    }

    // Aggiornamento velocità con fattori
    vx += close_dx * AVOID_FACTOR;
    vy += close_dy * AVOID_FACTOR;

    if (neighbors > 0) {
        const float inv_n = 1.0f / (float)neighbors;
        vx += (vel_avg_x * inv_n - vx) * MATCHING_FACTOR + (pos_avg_x * inv_n - x) * CENTERING_FACTOR;
        vy += (vel_avg_y * inv_n - vy) * MATCHING_FACTOR + (pos_avg_y * inv_n - y) * CENTERING_FACTOR;
    }

    // Bordi
    if (x < ctx->left_margin) vx += TURN_FACTOR;
    else if (x > ctx->right_margin) vx -= TURN_FACTOR;
    if (y < ctx->top_margin) vy += TURN_FACTOR;
    else if (y > ctx->bottom_margin) vy -= TURN_FACTOR;

    // Limite velocità
    const float speed_sq = vx * vx + vy * vy;
    if (speed_sq > MAX_SPEED * MAX_SPEED) {
        const float speed = sqrtf(speed_sq);
        vx = (vx / speed) * MAX_SPEED;
        vy = (vy / speed) * MAX_SPEED;
    }
    else if (speed_sq < MIN_SPEED * MIN_SPEED && speed_sq > 0.01f) {
        const float speed = sqrtf(speed_sq);
        vx = (vx / speed) * MIN_SPEED;
        vy = (vy / speed) * MIN_SPEED;
    }

    B_X(new_boids, current_idx) = x + vx * dt;
    B_Y(new_boids, current_idx) = y + vy * dt;
    B_VX(new_boids, current_idx) = vx;
    B_VY(new_boids, current_idx) = vy;
}

void update_indices(const BoidSystem* boids, int num_boids, SimulationContext* ctx) {
    const int total_cells = ctx->total_cells;
    const int cols = ctx->grid_cols;
    const int rows = ctx->grid_rows;

#ifdef _OPENMP
#if defined(USE_ATOMIC)

    #pragma omp parallel
    {
        #pragma omp for
        for (int c = 0; c < total_cells; c++) {
            ctx->counting_cell[c] = 0;
        }

        #pragma omp for
        for (int i = 0; i < num_boids; i++) {
            int cell = get_cell_index(B_X(boids, i), B_Y(boids, i), cols, rows);
            #pragma omp atomic
            ctx->counting_cell[cell]++;
        }

        #pragma omp single
        {
            ctx->cell_offsets[0] = 0;
            for (int i = 0; i < total_cells; i++) {
                ctx->cell_offsets[i+1] = ctx->cell_offsets[i] + ctx->counting_cell[i];
            }
            // Dato che il numero di celle è realtivamente piccolo, non si ottiene un
            // grande vantaggio parallelizzando questa parte
            memcpy(ctx->temp_offsets, ctx->cell_offsets, total_cells * sizeof(int));
        }

        #pragma omp for
        for (int i = 0; i < num_boids; i++) {
            int cell = get_cell_index(B_X(boids, i), B_Y(boids, i), cols, rows);
            int pos;

            #pragma omp atomic capture
            pos = ctx->temp_offsets[cell]++;

            ctx->sorted_ind[pos] = i;
        }
    }
        #else
    #pragma omp parallel
    {
        const int tid = omp_get_thread_num();
        int* my_hist = ctx->local_histograms[tid];
        int* my_offs = ctx->local_offsets[tid];

        // Reset locale
        memset(my_hist, 0, total_cells * sizeof(int));

        // Conteggio locale
        #pragma omp for
        for (int i = 0; i < num_boids; i++) {
            my_hist[get_cell_index(B_X(boids, i), B_Y(boids, i), cols, rows)]++;
        }

        // Riduzione globale (Somma degli istogrammi)
        #pragma omp for
        for (int c = 0; c < total_cells; c++) {
            int sum = 0;
            for (int t = 0; t < ctx->num_threads; t++) sum += ctx->local_histograms[t][c];
            ctx->counting_cell[c] = sum;
        }

        // Prefissi globali (Punto di inizio di ogni cella), non parallelizzabile per la dipendenza da i-1
        #pragma omp single
        {
            ctx->cell_offsets[0] = 0;
            for (int i = 0; i < total_cells; ++i)
                ctx->cell_offsets[i+1] = ctx->cell_offsets[i] + ctx->counting_cell[i];
        }

        // Calcolo posizioni di scrittura private per thread
        // per evitare collisioni in sorted_ind
        for (int c = 0; c < total_cells; c++) {
            int thread_offset = 0;
            for (int t = 0; t < tid; t++) thread_offset += ctx->local_histograms[t][c];
            my_offs[c] = ctx->cell_offsets[c] + thread_offset;
        }

        // Aggiornamento sorted_ind
        #pragma omp for
        for (int i = 0; i < num_boids; i++) {
            const int cell = get_cell_index(B_X(boids, i), B_Y(boids, i), cols, rows);
            ctx->sorted_ind[my_offs[cell]++] = i;
        }
    }
        #endif
    #else
        // SEQUENZIALE
        memset(ctx->counting_cell, 0, total_cells * sizeof(int));
        for (int i = 0; i < num_boids; i++) ctx->counting_cell[get_cell_index(B_X(boids, i), B_Y(boids, i), cols, rows)]++;

        ctx->cell_offsets[0] = 0;
        for (int i = 0; i < total_cells; i++) ctx->cell_offsets[i+1] = ctx->cell_offsets[i] + ctx->counting_cell[i];

        memcpy(ctx->temp_offsets, ctx->cell_offsets, total_cells * sizeof(int));

        for (int i = 0; i < num_boids; i++) {
            ctx->sorted_ind[ctx->temp_offsets[get_cell_index(B_X(boids, i), B_Y(boids, i), cols, rows)]++] = i;
        }
    #endif
}

void sort_boids_by_cell(const BoidSystem* boids, const int num_boids, int *sorted_ind, const BoidSystem* sorted_boids) {
    #pragma omp parallel for
    for (int i = 0; i < num_boids; i++) {
        // Legge l'indice vecchio dalla mappa
        const int idx = sorted_ind[i];

        // Esegue la copia fisica per riordinare l'array
        B_X(sorted_boids, i) = B_X(boids, idx);
        B_Y(sorted_boids, i) = B_Y(boids, idx);
        B_VX(sorted_boids, i) = B_VX(boids, idx);
        B_VY(sorted_boids, i) = B_VY(boids, idx);

        // Ripristina la mappa per il nuovo ordine fisico
        sorted_ind[i] = i;
    }
}