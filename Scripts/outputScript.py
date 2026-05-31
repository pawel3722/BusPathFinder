import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv(
    "output.csv",
    header=None,
    names=[
        "experiment",
        "algorithm",
        "run",
        "arrival",
        "travel",
        "waiting",
        "transfers",
        "computation"
    ]
)

# ============================================================
# AGREGACJA:
# dla każdego (experiment, run, algorithm)
# bierzemy najlepszą znalezioną ścieżkę
# ============================================================

best = (
    df.groupby(["experiment", "run", "algorithm"])
      .agg(
          best_travel=("travel", "min"),
          best_arrival=("arrival", "min"),
          best_waiting=("waiting", "min"),
          best_transfers=("transfers", "min"),
          pareto_size=("travel", "size"),
          computation=("computation", "first")
      )
      .reset_index()
)

# ============================================================
# 1. CZAS OBLICZEŃ
# ============================================================

plt.figure(figsize=(8,5))

for alg in ["GEN", "ACO", "PSO"]:
    vals = best[best.algorithm == alg]["computation"]
    plt.boxplot(
        vals,
        positions=[["GEN","ACO","PSO"].index(alg)+1],
        labels=[alg]
    )

plt.title("Computation time")
plt.ylabel("ms")
plt.grid(True, alpha=0.3)

plt.tight_layout()
plt.savefig("01_computation_time.png")
plt.close()

# ============================================================
# 2. JAKOŚĆ ROZWIĄZANIA
# ============================================================

plt.figure(figsize=(8,5))

for alg in ["GEN", "ACO", "PSO"]:
    vals = best[best.algorithm == alg]["best_travel"]
    plt.boxplot(
        vals,
        positions=[["GEN","ACO","PSO"].index(alg)+1],
        labels=[alg]
    )

plt.title("Best travel time")
plt.ylabel("minutes")
plt.grid(True, alpha=0.3)

plt.tight_layout()
plt.savefig("02_travel_time.png")
plt.close()

# ============================================================
# 3. ROZMIAR FRONTU PARETO
# ============================================================

plt.figure(figsize=(8,5))

for alg in ["GEN", "ACO", "PSO"]:
    vals = best[best.algorithm == alg]["pareto_size"]
    plt.boxplot(
        vals,
        positions=[["GEN","ACO","PSO"].index(alg)+1],
        labels=[alg]
    )

plt.title("Pareto front size")
plt.ylabel("number of paths")
plt.grid(True, alpha=0.3)

plt.tight_layout()
plt.savefig("03_pareto_size.png")
plt.close()

# ============================================================
# 4. ŚREDNIE + ODCHYLENIE
# ============================================================

summary = (
    best.groupby("algorithm")
        .agg(
            mean_time=("computation","mean"),
            std_time=("computation","std")
        )
)

plt.figure(figsize=(8,5))

plt.bar(
    summary.index,
    summary["mean_time"],
    yerr=summary["std_time"],
    capsize=5
)

plt.title("Average computation time")
plt.ylabel("ms")

plt.tight_layout()
plt.savefig("04_average_time.png")
plt.close()

print(summary)
print("\nDone.")