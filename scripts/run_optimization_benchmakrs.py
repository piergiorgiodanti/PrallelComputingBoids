import subprocess
import os
import sys

BUILD_DIR = "../cmake-build-release"
TARGET_NAME = "boids_O0"
OUT_CSV = "../results/dumps/baseline_results.csv"

BOIDS = 20000
THREADS = 8
STEPS = 1000
RUNS = 5
OMP_SCHEDULE = "dynamic,64"

def run_baseline():
    target_path = os.path.join(BUILD_DIR, TARGET_NAME)

    print("=" * 68)
    print(f" RUNNING BASELINE TEST: {TARGET_NAME} (No compiler optimizations)")
    print(f" Configurazione: {BOIDS} Boids, {STEPS} Steps, {RUNS} Runs")
    print("=" * 68)

    os.makedirs(os.path.dirname(OUT_CSV), exist_ok=True)

    if not os.path.isfile(target_path):
        print(f">>> [ERRORE] Il file {TARGET_NAME} non esiste in {BUILD_DIR}.")
        print("    Controlla il tuo CMakeLists.txt per il target con flag -O0.")
        sys.exit(1)

    if os.path.exists(OUT_CSV):
        os.remove(OUT_CSV)

    env = os.environ.copy()
    env["OMP_SCHEDULE"] = OMP_SCHEDULE

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
        print(">>> [OK] Baseline target trovato. Inizio benchmark...")
        subprocess.run(cmd, env=env, check=True)
        
        print("-" * 60)
        print(">>> FINISH: Baseline completato con successo.")
        print(f">>> Dati salvati in: {OUT_CSV}")
        print("=" * 68)

    except subprocess.CalledProcessError as e:
        print(f"\n>>> [ERRORE] Il benchmark è fallito con codice {e.returncode}.")
        sys.exit(1)
    except Exception as e:
        print(f"\n>>> [ERRORE INASPETTATO]: {e}")
        sys.exit(1)

if __name__ == "__main__":
    run_baseline()