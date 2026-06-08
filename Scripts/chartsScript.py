import pandas as pd
import matplotlib.pyplot as plt

# wczytanie pliku (bez nagłówka)
df = pd.read_csv("results.csv", header=None)

df.columns = [
    "run",
    "algo",
    "iteration",
    "arrival",
    "travel",
    "waiting",
    "transfers",
    "time_ms"
]

# -----------------------------
# 1. ŚREDNI CZAS OBLICZEŃ
# -----------------------------
plt.figure()
df.groupby("algo")["time_ms"].mean().plot(kind="bar")
plt.title("Średni czas obliczeń")
plt.ylabel("ms")
plt.xlabel("Algorytm")
plt.tight_layout()
plt.show()

# -----------------------------
# 2. ROZKŁAD CZASU (boxplot)
# -----------------------------
plt.figure()
df.boxplot(column="time_ms", by="algo")
plt.title("Rozkład czasu obliczeń")
plt.suptitle("")
plt.ylabel("ms")
plt.tight_layout()
plt.show()

# -----------------------------
# 3. ARRIVAL TIME (średnie)
# -----------------------------
plt.figure()
df.groupby("algo")["arrival"].mean().plot(kind="bar")
plt.title("Średni arrival time")
plt.ylabel("minutes (encoded)")
plt.tight_layout()
plt.show()

# -----------------------------
# 4. TRANSFERS
# -----------------------------
plt.figure()
df.groupby("algo")["transfers"].mean().plot(kind="bar")
plt.title("Średnia liczba przesiadek")
plt.tight_layout()
plt.show()

# -----------------------------
# 5. TRAJEKTORIE (per run)
# -----------------------------
plt.figure()
for algo in df["algo"].unique():
    subset = df[df["algo"] == algo]
    plt.plot(subset["iteration"], subset["time_ms"], label=algo)

plt.title("Czas obliczeń w kolejnych iteracjach")
plt.xlabel("Iteracja")
plt.ylabel("ms")
plt.legend()
plt.tight_layout()
plt.show()