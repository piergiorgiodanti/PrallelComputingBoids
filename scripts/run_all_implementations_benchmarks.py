import subprocess
import os
import sys

BUILD_DIR = "../cmake-build-release"
OUT_CSV = "../results/dumps/all_implementations_benchmarks.csv"

TARGETS = [
    "boids_seq_aos", "boids_seq_soa", 
    "boids_par_aos_histo", "boids_par_soa_histo", 
    "boids_par_aos_atomic", "boids_par_soa_atomic"
]

BOIDS_LIST = [1000, 5000, 10000, 20000]
THREADS_LIST = [1, 2, 4, 8, 16]
STEPS = 1000
RUNS = 5

OMP_ENV = os.environ.copy()
OMP_ENV["OMP_SCHEDULE"] = "dynamic,64"

def main():
    print("="*68)
    print(f" START: ALL IMPLEMENTATIONS BENCHMARKS ")
    print(f" Configurazione: {STEPS} steps, {RUNS} runs per config")
    print("="*68)

    os.makedirs(os.path.dirname(OUT_CSV), exist_ok=True)
    if os.path.exists(OUT_CSV):
        os.remove(OUT_CSV)

    for target in TARGETS:
        target_path = os.path.join(BUILD_DIR, target)

        if not os.path.isfile(target_path):
            print(f">>> [SKIP] {target} non trovato in {BUILD_DIR}")
            continue

        print("-" * 60)
        print(f">>> [TESTING TARGET]: {target}")

        for boids in BOIDS_LIST:
            for threads in THREADS_LIST:
                
                if "seq" in target and threads > 1:
                    continue

                print(f"\n>>> [START] Config: Boids={boids}, Threads={threads}")

                cmd = [
                    target_path,
                    "--mode", "benchmark",
                    "--boids", str(boids),
                    "--threads", str(threads),
                    "--steps", str(STEPS),
                    "--runs", str(RUNS),
                    "--out", OUT_CSV
                ]

                try:
                    subprocess.run(cmd, env=OMP_ENV, check=True)
                except subprocess.CalledProcessError as e:
                    print(f"    [ERRORE] Il target {target} è crashato. Config: B:{boids} T:{threads}")
                except Exception as e:
                    print(f"    [ERRORE INASPETTATO]: {e}")

        print(f">>> [DONE] {target} completato.")

    print("="*68)
    print(f" FINISH: Dati salvati in {OUT_CSV}")
    print("="*68)

if __name__ == "__main__":
    main()