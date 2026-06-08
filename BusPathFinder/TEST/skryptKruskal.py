import pandas as pd
from scipy.stats import kruskal

# wczytaj wyniki HV dla pojedynczych uruchomień
df = pd.read_csv("hv_per_uruchomienie.csv")

print("Liczba obserwacji:")
print(df.groupby("algorytm")["HV"].count())
print()

# =========================
# TEST GLOBALNY
# =========================

gen = df[df["algorytm"] == "GEN"]["HV"]
aco = df[df["algorytm"] == "ACO"]["HV"]
pso = df[df["algorytm"] == "PSO"]["HV"]

stat, p = kruskal(gen, aco, pso)

print("=== Test Kruskala-Wallisa globalnie ===")
print(f"H = {stat:.4f}")
print(f"p = {p:.6f}")

if p < 0.05:
    print("Wynik: różnice między algorytmami są statystycznie istotne.")
else:
    print("Wynik: brak podstaw do stwierdzenia istotnych różnic.")
print()


# =========================
# TEST OSOBNO DLA EKSPERYMENTÓW
# =========================

print("=== Test Kruskala-Wallisa osobno dla eksperymentów ===")

for exp_id, group in df.groupby("nr_eksperymentu"):

    gen = group[group["algorytm"] == "GEN"]["HV"]
    aco = group[group["algorytm"] == "ACO"]["HV"]
    pso = group[group["algorytm"] == "PSO"]["HV"]

    # test ma sens tylko jeśli każda grupa ma co najmniej 2 obserwacje
    if len(gen) < 2 or len(aco) < 2 or len(pso) < 2:
        print(f"Eksperyment {exp_id}: za mało danych do testu")
        continue

    stat, p = kruskal(gen, aco, pso)

    print(f"Eksperyment {exp_id}: H = {stat:.4f}, p = {p:.6f}")

    if p < 0.05:
        print("  -> różnice istotne statystycznie")
    else:
        print("  -> brak istotnych różnic")