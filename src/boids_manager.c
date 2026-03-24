#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include "boids_manager.h"
#include "boids_types.h"

#ifdef _OPENMP
    #include <omp.h>
#endif

void init_boids(const BoidSystem* boids, const int num_boids) {
    // Si assegnano valori iniziali randomici ai boids
    // Questa parte poteva essere parallela, ma non rientrando nella parte
    // da misurare nelle prestazioni è rimasta sequenziale
    unsigned int master_seed = 42;
    for (int i = 0; i < num_boids; i++) {
        const float start_x = MARGIN_X + (rand_r(&master_seed) % (SCREEN_WIDTH - 2 * MARGIN_X));
        const float start_y = MARGIN_Y + (rand_r(&master_seed) % (SCREEN_HEIGHT - 2 * MARGIN_Y));
        const float angle = (float)(rand_r(&master_seed) % 360) * DEG2RAD;
        const float speed = MIN_SPEED + ((float)rand_r(&master_seed) / (float)RAND_MAX) * (MAX_SPEED - MIN_SPEED);
        const float start_vx = cosf(angle) * speed;
        const float start_vy = sinf(angle) * speed;
        B_X(boids, i) = start_x;
        B_Y(boids, i) = start_y;
        B_VX(boids, i) = start_vx;
        B_VY(boids, i) = start_vy;
    }
}

void init_boids_system(BoidSystem *b, int num_boids) {
    b->count = num_boids;

    // Dimensione della linea di cache
    // Allineare la memoria a questa dimensione massimizza l'efficienza delle istruzioni SIMD
    long alignment = sysconf(_SC_LEVEL1_DCACHE_LINESIZE);
    if (alignment <= 0) {
        alignment = 64;
    }

    #ifdef USE_SOA
    size_t size_bytes = sizeof(float) * num_boids;
    size_t aligned_size = ((size_bytes + alignment - 1) / alignment) * alignment;

    // Allocazione SoA allineata
    b->x  = (float *) aligned_alloc(alignment, aligned_size);
    b->y  = (float *) aligned_alloc(alignment, aligned_size);
    b->vx = (float *) aligned_alloc(alignment, aligned_size);
    b->vy = (float *) aligned_alloc(alignment, aligned_size);

    if (!b->x || !b->y || !b->vx || !b->vy) {
        fprintf(stderr, "Errore di allocazione SoA\n");
        exit(-1);
    }
    #else
    size_t size_bytes = sizeof(Boid) * num_boids;
    size_t aligned_size = ((size_bytes + alignment - 1) / alignment) * alignment;

    // Allocazione AoS allineata
    b->data = (Boid *) aligned_alloc(alignment, aligned_size);

    if (!b->data) {
        fprintf(stderr, "Errore di allocazione AoS\n");
        exit(-1);
    }
    #endif
}

void free_boids_system(BoidSystem *b) {
    if (b == NULL) return;

    #ifdef USE_SOA
    free(b->x);
    free(b->y);
    free(b->vx);
    free(b->vy);
    b->x = b->y = b->vx = b->vy = NULL;
    #else
    free(b->data);
    b->data = NULL;
    #endif
}

void free_simulation_context(const SimulationContext *ctx) {
    if (ctx == NULL) return;

    free(ctx->counting_cell);
    free(ctx->cell_offsets);
    free(ctx->sorted_ind);
    free(ctx->temp_offsets);

    #ifdef _OPENMP
    if (ctx->local_histograms) {
        for (int t = 0; t < ctx->num_threads; t++) {
            free(ctx->local_histograms[t]);
            free(ctx->local_offsets[t]);
        }
        free(ctx->local_histograms);
        free(ctx->local_offsets);
    }
    #endif
}

void init_simulation_context(SimulationContext *ctx, int num_boids) {
    #ifdef _OPENMP
    ctx->num_threads = omp_get_max_threads();
    #else
    ctx->num_threads = 1;
    #endif

    // Assegna valori utili alla struttura ctx
    const float scale = sqrtf(1000.0f / (float)num_boids);
    const float v_range = 150.0f * scale;
    ctx->protected_range_sq = (40.0f * scale) * (40.0f * scale);
    ctx->visual_range_sq = v_range * v_range;
    ctx->left_margin = MARGIN_X; ctx->right_margin = SCREEN_WIDTH - MARGIN_X;
    ctx->top_margin = MARGIN_Y; ctx->bottom_margin = SCREEN_HEIGHT - MARGIN_Y;
    ctx->grid_cols = (SCREEN_WIDTH / (int)v_range) + 1;
    ctx->grid_rows = (SCREEN_HEIGHT / (int)v_range) + 1;
    ctx->total_cells = ctx->grid_cols * ctx->grid_rows;

    // Alloca le strutture per la gestione degli indici delle celle
    ctx->counting_cell = calloc(ctx->total_cells, sizeof(int));
    ctx->cell_offsets = malloc((ctx->total_cells + 1) * sizeof(int));
    ctx->sorted_ind = malloc(num_boids * sizeof(int));

    ctx->temp_offsets = malloc(ctx->total_cells * sizeof(int));

    #if defined(_OPENMP) && !defined(USE_ATOMIC)
    // Se siamo nella versione con gli istogrammi si allocano
    ctx->local_histograms = malloc(ctx->num_threads * sizeof(int *));
    ctx->local_offsets = malloc(ctx->num_threads * sizeof(int *));

    // Rileviamo la cache line per evitare False Sharing
    long cache_line = sysconf(_SC_LEVEL1_DCACHE_LINESIZE);
    if (cache_line <= 0) cache_line = 64;

    // Allineiamo la dimensione del buffer alla linea di cache
    size_t size_bytes = ctx->total_cells * sizeof(int);
    size_t aligned_size = ((size_bytes + cache_line - 1) / cache_line) * cache_line;

    for (int t = 0; t < ctx->num_threads; t++) {
        // aligned_alloc garantisce che l'indirizzo di inizio sia multiplo di cache_line
        ctx->local_histograms[t] = aligned_alloc(cache_line, aligned_size);
        memset(ctx->local_histograms[t], 0, aligned_size);
        ctx->local_offsets[t] = aligned_alloc(cache_line, aligned_size);
    }

    #else
    ctx->local_histograms = NULL; ctx->local_offsets = NULL;
    #endif
}