import subprocess
import os
import sys

BUILD_DIR = "../cmake-build-release"
TARGET_NAME = "boids_par_soa_histo"
OUT_CSV = "../results/dumps/scheduling_comparison.csv"

BOIDS = 20000
THREADS = 8
STEPS = 1000
RUNS = 5

SCHEDULES = [
    "static", 
    "dynamic,16", 
    "dynamic,32", 
    "dynamic,64", 
    "dynamic,128", 
    "guided"
]

def run_scheduling_benchmarks():
    target_path = os.path.join(BUILD_DIR, TARGET_NAME)

    print("=" * 68)
    print(" START: ANALISI PERFORMANCE SCHEDULING (Load Balancing)")
    print(f" Target: {TARGET_NAME}")
    print(f" Configurazione: {BOIDS} Boids | {THREADS} Threads | {STEPS} Steps")
    print("=" * 68)

    os.makedirs(os.path.dirname(OUT_CSV), exist_ok=True)

    if not os.path.isfile(target_path):
        print(f">>> [ERRORE] Binario {TARGET_NAME} non trovato in {BUILD_DIR}!")
        sys.exit(1)

    if os.path.exists(OUT_CSV):
        os.remove(OUT_CSV)

    print("--- Testando clausole OMP_SCHEDULE ---")
    
    for s in SCHEDULES:
        print(f">>> [RUNNING] Schedule: {s}")
        
        env = os.environ.copy()
        env["OMP_SCHEDULE"] = s

        cmd = [
            target_path,
            "--mode", "benchmark",
            "--boids", str(BOIDS),
            "--threads", str(THREADS),
            "--steps", str(STEPS),
            "--runs", str(RUNS),
            "--out", OUT_CSV
        ]

        try:
            subprocess.run(cmd, env=env, check=True)
        except subprocess.CalledProcessError as e:
            print(f"    [ERRORE] Esecuzione fallita per schedule {s} (Exit code: {e.returncode})")
        except Exception as e:
            print(f"    [ERRORE INASPETTATO]: {e}")

    print("=" * 68)
    print(f" FINISH: Analisi completata. Dati in {OUT_CSV}")
    print("=" * 68)

if __name__ == "__main__":
    run_scheduling_benchmarks()