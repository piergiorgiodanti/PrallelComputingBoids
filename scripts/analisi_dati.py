import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import os

try:
    df = pd.read_csv('../results/dumps/all_implementations_benchmarks.csv')
    df_sched_raw = pd.read_csv('../results/dumps/scheduling_comparison.csv')
    df_o0 = pd.read_csv('../results/dumps/baseline_results.csv')
    print("File CSV caricati con successo.")
except Exception as e:
    print(f"Errore caricamento: {e}")
    exit()

if not os.path.exists('../results/plots'):
    os.makedirs('../results/plots')

# Creazione della colonna Impl per entrambi i dataframe
df['Impl'] = df['Layout'] + " " + df['Sync']
df_sched_raw['Impl'] = df_sched_raw['Layout'] + " " + df_sched_raw['Sync']

df = df.drop_duplicates(subset=['Impl', 'Schedule', 'Boids', 'Threads'])

PARALLEL_IMPLS = ['AoS Atomic', 'AoS Histo', 'SoA Atomic', 'SoA Histo']
COLORS = {'AoS Atomic': '#e74c3c', 'AoS Histo': '#c0392b', 'SoA Atomic': '#3498db', 'SoA Histo': '#2980b9'}
BOIDS_TARGET = 20000
df_base = df[df['Schedule'] == 'dynamic-64']

# Grafico 1: Confronto Layout di Memoria e Sincronizzazione
plt.figure(figsize=(10, 6))
df_comp = df_base[(df_base['Boids'] == BOIDS_TARGET) & (df_base['Threads'] == 8)].copy()
df_comp = df_comp[df_comp['Impl'].isin(PARALLEL_IMPLS)]

plt.bar(df_comp['Impl'], df_comp['WallTime_Avg'], yerr=df_comp['WallTime_StdDev'],
        capsize=5, color=[COLORS[i] for i in df_comp['Impl']], alpha=0.8, edgecolor='black')
plt.ylabel('Wall Time (s)')
plt.title(f'Confronto Architetturale ({BOIDS_TARGET} Boids, 8 Thread)')
plt.grid(axis='y', linestyle='--', alpha=0.4)
plt.tight_layout()
plt.savefig('../results/plots/1_arch_comparison.png')
plt.close()

# Grafico 2: Strong Scaling (tempo vs threads)
plt.figure(figsize=(10, 6))
df_20k = df_base[df_base['Boids'] == BOIDS_TARGET]
threads_list = [1, 2, 4, 8, 16]

for impl in PARALLEL_IMPLS:
    data = df_20k[df_20k['Impl'] == impl].sort_values('Threads')
    plt.errorbar(data['Threads'], data['WallTime_Avg'], yerr=data['WallTime_StdDev'],
                 marker='o', color=COLORS[impl], linewidth=2, label=impl, capsize=4)

seq_time = df_20k[df_20k['Impl'] == 'SoA Seq']['WallTime_Avg'].values[0]
plt.axhline(y=seq_time, color='black', linestyle='--', label='SoA Seq (Baseline)')

plt.xlabel('Numero di Thread')
plt.ylabel('Wall Time (s)')
plt.title(f'Strong Scaling: Tempo di Esecuzione ({BOIDS_TARGET} Boids)')
plt.xticks(threads_list)
plt.legend()
plt.grid(True, alpha=0.4)
plt.tight_layout()
plt.savefig('../results/plots/2_strong_scaling.png')
plt.close()

# Grafico 3: Speedup ed Efficienza
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(15, 6))

for impl in PARALLEL_IMPLS:
    data = df_20k[df_20k['Impl'] == impl].sort_values('Threads')
    speedup = seq_time / data['WallTime_Avg'].values
    efficiency = speedup / data['Threads'].values * 100

    ax1.plot(data['Threads'], speedup, marker='o', color=COLORS[impl], linewidth=2, label=impl)
    ax2.plot(data['Threads'], efficiency, marker='s', color=COLORS[impl], linewidth=2, label=impl)

ax1.plot(threads_list, threads_list, 'k--', alpha=0.6, label='Ideale')
ax1.set_xlabel('Thread')
ax1.set_ylabel('Speedup')
ax1.set_title('Speedup Parallelo')
ax1.set_xticks(threads_list)
ax1.grid(True, alpha=0.4)

ax2.axhline(y=100, color='k', linestyle='--', alpha=0.6)
ax2.set_xlabel('Thread')
ax2.set_ylabel('Efficienza (%)')
ax2.set_title('Efficienza di Parallelizzazione')
ax2.set_xticks(threads_list)
ax2.grid(True, alpha=0.4)
ax2.legend()
plt.tight_layout()
plt.savefig('../results/plots/3_speedup_efficiency.png')
plt.close()

# Grafico 4: CPU time vs wall time
plt.figure(figsize=(10, 6))
soa_h_data = df_20k[df_20k['Impl'] == 'SoA Histo'].sort_values('Threads')

width = 0.35
x = np.arange(len(threads_list))
ideal_cpu = soa_h_data['WallTime_Avg'].values * soa_h_data['Threads'].values

plt.bar(x - width/2, ideal_cpu, width, label='CPU Time Ideale (WallTime * Thread)', color='#95a5a6')
plt.bar(x + width/2, soa_h_data['CPUTime_Avg'], width, label='CPU Time Reale (Dai dati osservati)', color='#e67e22')

plt.ylabel('Tempo Cumulativo (s)')
plt.xlabel('Thread')
plt.title('Analisi Overhead: CPU Time Ideale vs Reale (SoA Histo)')
plt.xticks(x, threads_list)
plt.legend()
plt.grid(axis='y', alpha=0.4)
plt.tight_layout()
plt.savefig('../results/plots/4_cpu_vs_wall_time.png')
plt.close()

# Grafico 5: Tempo vs Numero di Boids
plt.figure(figsize=(10, 6))
df_8t = df_base[df_base['Threads'] == 8]
boids_list = [1000, 5000, 10000, 20000]

for impl in PARALLEL_IMPLS:
    data = df_8t[df_8t['Impl'] == impl].sort_values('Boids')
    plt.plot(data['Boids'], data['WallTime_Avg'], marker='o', color=COLORS[impl], linewidth=2, label=impl)

plt.xlabel('Numero di Boids')
plt.ylabel('Wall Time (s)')
plt.title('Complessità Computazionale: Tempo vs Carico (8 Thread)')
plt.xticks(boids_list)
plt.legend()
plt.grid(True, alpha=0.4)
plt.tight_layout()
plt.savefig('../results/plots/5_time_vs_boids.png')
plt.close()

# Grafico 6: Scheduling
schedules = ['static', 'dynamic-16', 'dynamic-32', 'dynamic-64', 'dynamic-128', 'guided']
df_sched = df_sched_raw[(df_sched_raw['Boids'] == BOIDS_TARGET) & (df_sched_raw['Threads'] == 8) & (df_sched_raw['Impl'] == 'SoA Histo')].copy()
df_sched = df_sched[df_sched['Schedule'].isin(schedules)]

df_sched['Schedule'] = pd.Categorical(df_sched['Schedule'], categories=schedules, ordered=True)
df_sched = df_sched.sort_values('Schedule')

plt.figure(figsize=(10, 6))
bars = plt.bar(df_sched['Schedule'], df_sched['WallTime_Avg'], yerr=df_sched['WallTime_StdDev'],
               capsize=5, color='#16a085', edgecolor='black', alpha=0.8)
plt.xlabel('Strategia di Scheduling OpenMP')
plt.ylabel('Wall Time (s)')
plt.title('Impatto dello Scheduling (SoA Histo, 20k Boids, 8 Thread)')
plt.xticks(rotation=15)
plt.grid(axis='y', alpha=0.4)

for bar in bars:
    yval = bar.get_height()
    plt.text(bar.get_x() + bar.get_width()/2, yval + 0.15, f'{yval:.2f}s', ha='center', va='bottom', fontweight='bold')

plt.ylim(0, max(df_sched['WallTime_Avg']) * 1.15)
plt.tight_layout()
plt.savefig('../results/plots/6_scheduling_impact.png')
plt.close()

# Grafico 7: Ottimizzazioni Compilatore
t_o3_row = df_20k[(df_20k['Impl'] == 'SoA Histo') & (df_20k['Threads'] == 8)].iloc[0]
t_o3 = t_o3_row['WallTime_Avg']
t_o3_std = t_o3_row['WallTime_StdDev']

t_o0 = df_o0['WallTime_Avg'].values[0]
t_o0_std = df_o0['WallTime_StdDev'].values[0]

plt.figure(figsize=(8, 6))
bars = plt.bar(['-O0 (Baseline)', '-O3 (Turbo + LTO)'], [t_o0, t_o3], yerr=[t_o0_std, t_o3_std],
               capsize=5, color=['#7f8c8d', '#f1c40f'], edgecolor='black', alpha=0.8)
plt.ylabel('Wall Time (s)')
plt.title('Impatto Ottimizzazioni Compilatore (SoA Histo, 20k Boids, 8T)')
plt.grid(axis='y', alpha=0.4)

for bar in bars:
    yval = bar.get_height()

plt.tight_layout()
plt.savefig('../results/plots/7_compiler_impact.png')
plt.close()

print("Generati grafici nella cartella '../results/ plots/'.")