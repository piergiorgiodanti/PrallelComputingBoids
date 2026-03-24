import subprocess
import os
import sys
import numpy as np
import pandas as pd

BUILD_DIR = "../cmake-build-release"
OUT_DIR = "../results/validation_dumps"

TARGETS = [
    "boids_seq_aos", "boids_seq_soa",
    "boids_par_aos_histo", "boids_par_soa_histo",
    "boids_par_aos_atomic", "boids_par_soa_atomic"
]

BOIDS = 1000
STEPS = 10
THREADS_VAL = 8
EPSILON = 1e-2

def main():
    print("=" * 60)
    print("   START: VALIDAZIONE CORRETTEZZA FISICA (Consistency Test)")
    print(f"   Configurazione: {BOIDS} Boids, {STEPS} Steps, {THREADS_VAL} Thread")
    print("=" * 60)

    os.makedirs(OUT_DIR, exist_ok=True)
    generated_files = {}

    for t in TARGETS:
        target_path = os.path.join(BUILD_DIR, t)

        if os.path.isfile(target_path):
            out_file = os.path.join(OUT_DIR, f"dump_{t}.csv")
            print(f">>> [VALIDATING] Target: {t}")

            cmd = [
                target_path,
                "--mode", "validate",
                "--boids", str(BOIDS),
                "--threads", str(THREADS_VAL),
                "--steps", str(STEPS),
                "--out", out_file
            ]

            try:
                subprocess.run(cmd, check=True)
                generated_files[t] = out_file
            except subprocess.CalledProcessError as e:
                print(f"    [ERRORE] Fallimento durante la validazione di {t} (Code: {e.returncode})")
        else:
            print(f">>> [SKIP] {t} non trovato in {BUILD_DIR}")

    print("\n" + "=" * 60)
    print("   FINE FASE 1: Dati generati. Avvio Fase 2...")
    print("=" * 60 + "\n")

    if len(generated_files) > 1:
        master_key = "boids_seq_aos" if "boids_seq_aos" in generated_files else list(generated_files.keys())[0]
        master_file = generated_files[master_key]

        print(f"Avvio Validazione Fisica (Master: {master_file})")
        print(f"Tolerance: {EPSILON}\n" + "-"*60)

        try:
            df_master = pd.read_csv(master_file)
            master_data = df_master.sort_values(by=['x', 'y']).to_numpy()
        except Exception as e:
            print(f"Errore nel caricamento del Master: {e}")
            return

        for t, f_name in generated_files.items():
            if f_name == master_file:
                continue

            try:
                df_test = pd.read_csv(f_name)
                test_data = df_test.sort_values(by=['x', 'y']).to_numpy()

                if len(master_data) != len(test_data):
                    print(f"{t:25} | ERRORE: Numero boids diverso! ({len(test_data)} vs {len(master_data)})")
                    continue

                diff = np.abs(master_data - test_data)
                max_diff = np.max(diff)

                if max_diff <= EPSILON:
                    print(f"{t:25} | PASS (Max Diff: {max_diff:.2e})")
                else:
                    failures = np.any(diff > EPSILON, axis=1).sum()
                    print(f"{t:25} | FAIL! Max Diff: {max_diff:.2e} | Boids sballati: {failures}")

            except Exception as e:
                print(f"Errore durante l'analisi di {t}: {e}")

if __name__ == "__main__":
    main()