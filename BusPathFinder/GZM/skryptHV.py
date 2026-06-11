import pandas as pd
import numpy as np
from pymoo.indicators.hv import HV

plik = "output.csv"

columns = [
    "nr_eksperymentu",
    "algorytm",
    "nr_iteracji",
    "czy_poprawny",
    "godzina_przyjazdu",
    "czas_jazdy",
    "czas_oczekiwania",
    "liczba_przesiadek",
    "czas_wykonania"
]

objectives = [
    "godzina_przyjazdu",
    "czas_jazdy",
    "czas_oczekiwania",
    "liczba_przesiadek"
]


def remove_dominated(points):
    """
    Usuwa rozwiązania zdominowane.
    Wszystkie kryteria są minimalizowane.
    """
    points = np.asarray(points)
    keep = []

    for i, p in enumerate(points):
        dominated = False
        for j, q in enumerate(points):
            if i != j and np.all(q <= p) and np.any(q < p):
                dominated = True
                break
        if not dominated:
            keep.append(i)

    return points[keep]


df = pd.read_csv(plik, header=None, names=columns)

print(df.head())

# =========================
# SUCCESS RATE
# =========================

runs = (
    df
    .groupby(["nr_eksperymentu", "algorytm", "nr_iteracji"])
    .agg(czy_znaleziono_rozwiazanie=("czy_poprawny", "max"))
    .reset_index()
)

success_summary = (
    runs
    .groupby(["nr_eksperymentu", "algorytm"])
    .agg(
        liczba_uruchomien=("czy_znaleziono_rozwiazanie", "count"),
        liczba_poprawnych=("czy_znaleziono_rozwiazanie", "sum"),
        success_rate=("czy_znaleziono_rozwiazanie", "mean")
    )
    .reset_index()
)

success_summary["success_rate_%"] = success_summary["success_rate"] * 100

success_global = (
    runs
    .groupby("algorytm")
    .agg(
        liczba_uruchomien=("czy_znaleziono_rozwiazanie", "count"),
        liczba_poprawnych=("czy_znaleziono_rozwiazanie", "sum"),
        success_rate=("czy_znaleziono_rozwiazanie", "mean")
    )
    .reset_index()
)

success_global["success_rate_%"] = success_global["success_rate"] * 100

success_summary.to_csv("success_rate_podsumowanie.csv", index=False)
success_global.to_csv("success_rate_globalnie.csv", index=False)

print("\n=== Success rate per eksperyment ===")
print(success_summary)

print("\n=== Success rate globalnie ===")
print(success_global)


# =========================
# HYPERVOLUME
# normalizacja osobno dla każdego eksperymentu
# =========================

results = []

ref_point = np.array([1.1] * len(objectives))

for exp, exp_df in df.groupby("nr_eksperymentu"):

    exp_valid = exp_df[exp_df["czy_poprawny"] == 1].copy()

    if exp_valid.empty:
        # jeśli w całym eksperymencie nie ma żadnego poprawnego rozwiązania
        for (alg, run), group in exp_df.groupby(["algorytm", "nr_iteracji"]):
            results.append({
                "nr_eksperymentu": exp,
                "algorytm": alg,
                "nr_iteracji": run,
                "HV": 0.0,
                "liczba_rozwiazan_pareto": 0
            })
        continue

    # normalizacja min-max tylko w ramach danego eksperymentu
    mins = exp_valid[objectives].min()
    maxs = exp_valid[objectives].max()
    ranges = (maxs - mins).replace(0, 1)

    hv_indicator = HV(ref_point=ref_point)

    # iterujemy po wszystkich uruchomieniach w danym eksperymencie,
    # także po tych, gdzie nie znaleziono rozwiązania
    for (alg, run), group in exp_df.groupby(["algorytm", "nr_iteracji"]):

        group_valid = group[group["czy_poprawny"] == 1].copy()

        if group_valid.empty:
            hv_value = 0.0
            pareto_size = 0
        else:
            group_valid[objectives] = (group_valid[objectives] - mins) / ranges
            points = group_valid[objectives].to_numpy()

            pareto_points = remove_dominated(points)
            hv_value = hv_indicator(pareto_points)
            pareto_size = len(pareto_points)

        results.append({
            "nr_eksperymentu": exp,
            "algorytm": alg,
            "nr_iteracji": run,
            "HV": hv_value,
            "liczba_rozwiazan_pareto": pareto_size
        })

hv_df = pd.DataFrame(results)

summary = (
    hv_df
    .groupby(["nr_eksperymentu", "algorytm"])
    .agg(
        srednie_HV=("HV", "mean"),
        mediana_HV=("HV", "median"),
        odchylenie_HV=("HV", "std"),
        min_HV=("HV", "min"),
        max_HV=("HV", "max"),
        srednia_liczba_rozwiazan_pareto=("liczba_rozwiazan_pareto", "mean")
    )
    .reset_index()
)

print("\n=== Liczba obserwacji HV ===")
print(hv_df.groupby("algorytm")["HV"].count())

print("\n=== HV dla każdego uruchomienia ===")
print(hv_df)

print("\n=== Podsumowanie HV ===")
print(summary)

hv_df.to_csv("hv_per_uruchomienie.csv", index=False)
summary.to_csv("hv_podsumowanie.csv", index=False)