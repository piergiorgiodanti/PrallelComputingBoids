#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>
#include "boids_types.h"
#include "boids_simulation.h"

#ifdef _OPENMP
    #include <omp.h>
#endif

typedef struct {
    double mean;
    double stdev;
} Stats;

typedef struct {
    char mode[32];
    int boids;
    int threads;
    int steps;
    int num_runs;
    char output_file[128];
} Config;

Stats compute_stats(double* times, int n) {
    double sum = 0, sum_sq = 0;
    for (int i = 0; i < n; i++) sum += times[i];
    double mean = sum / n;
    for (int i = 0; i < n; i++) sum_sq += pow(times[i] - mean, 2);
    return (Stats){mean, sqrt(sum_sq / n)};
}

void print_info() {
    printf("============================================\n");
    #ifdef _OPENMP
        printf(" ENGINE: PARALLELO (OpenMP %d threads max)\n", omp_get_max_threads());
    #else
        printf(" ENGINE: SEQUENZIALE\n");
    #endif
    #ifdef USE_SOA
        printf(" LAYOUT: SoA (Structure of Arrays)\n");
    #else
        printf(" LAYOUT: AoS (Array of Structures)\n");
    #endif
    printf("============================================\n\n");
}

void parse_args(int argc, char* argv[], Config* cfg) {
    strcpy(cfg->mode, "");
    cfg->boids = -1;
    cfg->threads = -1;
    cfg->steps = 1000;
    cfg->num_runs = 5;
    strcpy(cfg->output_file, "results.csv");

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--mode") == 0 && i + 1 < argc) strcpy(cfg->mode, argv[++i]);
        else if (strcmp(argv[i], "--boids") == 0 && i + 1 < argc) cfg->boids = atoi(argv[++i]);
        else if (strcmp(argv[i], "--threads") == 0 && i + 1 < argc) cfg->threads = atoi(argv[++i]);
        else if (strcmp(argv[i], "--steps") == 0 && i + 1 < argc) cfg->steps = atoi(argv[++i]);
        else if (strcmp(argv[i], "--runs") == 0 && i + 1 < argc) cfg->num_runs = atoi(argv[++i]);
        else if (strcmp(argv[i], "--out") == 0 && i + 1 < argc) strcpy(cfg->output_file, argv[++i]);
    }

    if (strlen(cfg->mode) == 0 || cfg->boids <= 0 || cfg->threads <= 0) {
        fprintf(stderr, "ERRORE: Parametri --mode, --boids e --threads sono obbligatori.\n");
        exit(1);
    }
}

void run_benchmark(Config* cfg) {

    char layout[8], sync[16];
    #ifdef USE_SOA
        strcpy(layout, "SoA");
    #else
        strcpy(layout, "AoS");
    #endif

    #ifdef USE_ATOMIC
        strcpy(sync, "Atomic");
    #elif defined(_OPENMP)
        strcpy(sync, "Histo");
    #else
        strcpy(sync, "Seq");
    #endif

    char* env_sched = getenv("OMP_SCHEDULE");
    char schedule_name[32];
    if (env_sched) {
        strncpy(schedule_name, env_sched, 31);
        for(int i=0; schedule_name[i]; i++) if(schedule_name[i]==',') schedule_name[i]='-';
    } else {
        strcpy(schedule_name, "default");
    }

    // Warm-up
    boids_simulation(cfg->boids, cfg->steps, false, false);
    boids_simulation(cfg->boids, cfg->steps, false, false);

    double* wall_results = malloc(sizeof(double) * cfg->num_runs);
    double* cpu_results  = malloc(sizeof(double) * cfg->num_runs);

    // Runs
    for (int r = 0; r < cfg->num_runs; r++) {
        SimulationResult res = boids_simulation(cfg->boids, cfg->steps, false, false);
        wall_results[r] = res.wall_time;
        cpu_results[r]  = res.cpu_time;
        printf(" -> Run %d/%d completata | Wall: %.4fs | CPU: %.4fs\n",
           r + 1, cfg->num_runs, res.wall_time, res.cpu_time);
        fflush(stdout);
    }

    Stats w_s = compute_stats(wall_results, cfg->num_runs);
    Stats c_s = compute_stats(cpu_results, cfg->num_runs);
    double efficiency = (c_s.mean / (w_s.mean * cfg->threads)) * 100.0;
    double steps_per_sec = cfg->steps / w_s.mean;

    // Apertura file in APPEND
    FILE *check = fopen(cfg->output_file, "r");
    bool write_header = (check == NULL);
    if (check) fclose(check);

    FILE *f = fopen(cfg->output_file, "a");
    if (f) {
        if (write_header) fprintf(f, "Layout,Sync,Schedule,Boids,Threads,WallTime_Avg,WallTime_StdDev,CPUTime_Avg,Efficiency,StepsPerSec\n");
        fprintf(f, "%s,%s,%s,%d,%d,%.6f,%.6f,%.6f,%.2f,%.2f\n",
                layout, sync, schedule_name, cfg->boids, cfg->threads, w_s.mean, w_s.stdev, c_s.mean, efficiency, steps_per_sec);
        fclose(f);
    }

    printf("\nDONE: [B:%d T:%d] Wall: %.4fs (stdev: %.4f) (Eff: %.1f%%)\n", cfg->boids, cfg->threads, w_s.mean, w_s.stdev, efficiency);
    free(wall_results); free(cpu_results);
}

void run_validate(Config* cfg) {
    SimulationResult res = boids_simulation(cfg->boids, cfg->steps, false, true);
    FILE *f = fopen(cfg->output_file, "w");
    if (f) {
        fprintf(f, "x,y,vx,vy\n");
        for (int i = 0; i < cfg->boids; i++)
            fprintf(f, "%.6f,%.6f,%.6f,%.6f\n", res.final_dump[i].x, res.final_dump[i].y, res.final_dump[i].vx, res.final_dump[i].vy);
        fclose(f);
    }
    free(res.final_dump);
    printf("Validazione completata: %s\n", cfg->output_file);
}

void run_gui(Config* cfg) {
    boids_simulation(cfg->boids, -1, true, false);
}

int main(int argc, char* argv[]) {
    Config cfg;
    parse_args(argc, argv, &cfg);

    #ifdef _OPENMP
    omp_set_num_threads(cfg.threads);
    #endif

    print_info();

    if (strcmp(cfg.mode, "benchmark") == 0) run_benchmark(&cfg);
    else if (strcmp(cfg.mode, "validate") == 0) run_validate(&cfg);
    else if (strcmp(cfg.mode, "gui") == 0) run_gui(&cfg);
    else printf("Errore: Modalità non riconosciuta.\n");

    return 0;
}