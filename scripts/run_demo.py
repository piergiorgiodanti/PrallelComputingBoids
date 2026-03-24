import subprocess
import os
import sys

BUILD_DIR = "../cmake-build-release"
TARGET_NAME = "boids_par_soa_histo"

BOIDS = 20000
THREADS = 8
OMP_SCHEDULE = "dynamic,64"

def run_gui():
    target_path = os.path.join(BUILD_DIR, TARGET_NAME)

    if not os.path.isfile(target_path):
        print(f">>> [ERRORE] Binario non trovato in: {target_path}")
        print("    Assicurati di aver compilato il progetto in modalità Release.")
        sys.exit(1)

    env = os.environ.copy()
    env["OMP_SCHEDULE"] = OMP_SCHEDULE

    cmd = [
        target_path,
        "--mode", "gui",
        "--boids", str(BOIDS),
        "--threads", str(THREADS),
        "--steps", "-1"
    ]

    try:
        subprocess.run(cmd, env=env, check=True)
        print("\nDemo terminata con successo.")
    except subprocess.CalledProcessError as e:
        print(f"\n[ERRORE] La simulazione si è interrotta inaspettatamente (Exit Code: {e.returncode})")
    except KeyboardInterrupt:
        print("\nInterruzione da parte dell'utente.")

if __name__ == "__main__":
    run_gui()