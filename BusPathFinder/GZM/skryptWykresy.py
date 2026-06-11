import pandas as pd
import matplotlib.pyplot as plt
from scipy.stats import kruskal
import scikit_posthocs as sp

def safe_kruskal(gen, aco, pso):
    all_values = pd.concat([gen, aco, pso])

    if all_values.nunique() <= 1:
        return None, None, "wszystkie wartości identyczne"

    if len(gen) < 2 or len(aco) < 2 or len(pso) < 2:
        return None, None, "za mało danych"

    stat, p = kruskal(gen, aco, pso)
    return stat, p, "ok"

# =========================
# WCZYTANIE DANYCH
# =========================

hv = pd.read_csv("hv_per_uruchomienie.csv")
hv_summary = pd.read_csv("hv_podsumowanie.csv")
success_global = pd.read_csv("success_rate_globalnie.csv")
success_exp = pd.read_csv("success_rate_podsumowanie.csv")

alg_order = ["GEN", "ACO", "PSO"]


# =========================
# 1. BOXPLOT HV
# =========================

data = [hv[hv["algorytm"] == alg]["HV"] for alg in alg_order]

plt.figure(figsize=(7, 5))
plt.boxplot(data, labels=alg_order)
plt.ylabel("Hypervolume (HV)")
plt.xlabel("Algorytm")
plt.title("Rozkład wartości HV dla badanych algorytmów")
plt.grid(axis="y", alpha=0.3)
plt.tight_layout()
plt.savefig("wykres_boxplot_HV.png", dpi=300)
plt.close()


# =========================
# 2. ŚREDNIE HV GLOBALNIE
# =========================

hv_global = (
    hv
    .groupby("algorytm")
    .agg(
        srednie_HV=("HV", "mean"),
        odchylenie_HV=("HV", "std"),
        mediana_HV=("HV", "median"),
        min_HV=("HV", "min"),
        max_HV=("HV", "max")
    )
    .reindex(alg_order)
    .reset_index()
)

plt.figure(figsize=(7, 5))
plt.bar(
    hv_global["algorytm"],
    hv_global["srednie_HV"],
    yerr=hv_global["odchylenie_HV"],
    capsize=5
)
plt.ylabel("Średnie HV")
plt.xlabel("Algorytm")
plt.title("Średnia wartość HV z odchyleniem standardowym")
plt.grid(axis="y", alpha=0.3)
plt.tight_layout()
plt.savefig("wykres_srednie_HV.png", dpi=300)
plt.close()


# =========================
# 3. SUCCESS RATE GLOBALNIE
# =========================

success_global = success_global.set_index("algorytm").reindex(alg_order).reset_index()

plt.figure(figsize=(7, 5))
plt.bar(success_global["algorytm"], success_global["success_rate_%"])
plt.ylabel("Skuteczność [%]")
plt.xlabel("Algorytm")
plt.title("Skuteczność znajdowania poprawnego rozwiązania")
plt.ylim(0, 100)
plt.grid(axis="y", alpha=0.3)
plt.tight_layout()
plt.savefig("wykres_success_rate.png", dpi=300)
plt.close()


# =========================
# 4. TABELA HV PER EKSPERYMENT DO LATEX
# =========================

hv_pivot = hv_summary.pivot(
    index="nr_eksperymentu",
    columns="algorytm",
    values="srednie_HV"
)

hv_pivot = hv_pivot[alg_order]
hv_pivot = hv_pivot.round(4)

latex_hv = hv_pivot.to_latex(
    caption="Średnie wartości wskaźnika Hypervolume dla poszczególnych eksperymentów.",
    label="tab:hv_srednie",
    float_format="%.4f"
)

with open("tabela_hv_srednie.tex", "w", encoding="utf-8") as f:
    f.write(latex_hv)


# =========================
# 5. TABELA SUCCESS RATE PER EKSPERYMENT DO LATEX
# =========================

success_exp_pivot = success_exp.pivot(
    index="nr_eksperymentu",
    columns="algorytm",
    values="success_rate_%"
)

success_exp_pivot = success_exp_pivot[alg_order]
success_exp_pivot = success_exp_pivot.round(2)

latex_success = success_exp_pivot.to_latex(
    caption="Skuteczność algorytmów dla poszczególnych eksperymentów [\\%].",
    label="tab:success_rate",
    float_format="%.2f"
)

with open("tabela_success_rate.tex", "w", encoding="utf-8") as f:
    f.write(latex_success)

# =========================
# 6. KRUSKAL-WALLIS GLOBALNIE
# =========================

gen = hv[hv["algorytm"] == "GEN"]["HV"]
aco = hv[hv["algorytm"] == "ACO"]["HV"]
pso = hv[hv["algorytm"] == "PSO"]["HV"]

stat, p, status = safe_kruskal(gen, aco, pso)

if status != "ok":
    print(f"Test pominięty: {status}")
else:
    print(f"H = {stat:.4f}, p = {p:.6f}")

kruskal_global = pd.DataFrame({
    "Test": ["Kruskal-Wallis globalnie"],
    "H": [stat],
    "p": [p]
})

latex_kruskal_global = kruskal_global.to_latex(
    index=False,
    caption="Wynik globalnego testu Kruskala-Wallisa dla wartości HV.",
    label="tab:kruskal_global",
    float_format="%.6f"
)

with open("tabela_kruskal_global.tex", "w", encoding="utf-8") as f:
    f.write(latex_kruskal_global)


# =========================
# 7. KRUSKAL-WALLIS PER EKSPERYMENT
# =========================

kruskal_rows = []

for exp_id, group in hv.groupby("nr_eksperymentu"):
    gen = group[group["algorytm"] == "GEN"]["HV"]
    aco = group[group["algorytm"] == "ACO"]["HV"]
    pso = group[group["algorytm"] == "PSO"]["HV"]

    stat, p, status = safe_kruskal(gen, aco, pso)

    if status != "ok":
        kruskal_rows.append({
            "Eksperyment": exp_id,
            "H": None,
            "p": None,
            "Wniosek": status
        })
    else:
        kruskal_rows.append({
            "Eksperyment": exp_id,
            "H": stat,
            "p": p,
            "Wniosek": "istotne" if p < 0.05 else "brak istotności"
        })

kruskal_exp = pd.DataFrame(kruskal_rows)

latex_kruskal_exp = kruskal_exp.to_latex(
    index=False,
    caption="Wyniki testu Kruskala-Wallisa dla poszczególnych eksperymentów.",
    label="tab:kruskal_exp",
    float_format="%.6f"
)

with open("tabela_kruskal_eksperymenty.tex", "w", encoding="utf-8") as f:
    f.write(latex_kruskal_exp)


# =========================
# 8. TEST DUNNA GLOBALNIE
# =========================

dunn = sp.posthoc_dunn(
    hv,
    val_col="HV",
    group_col="algorytm",
    p_adjust="bonferroni"
)

dunn = dunn.loc[alg_order, alg_order].round(6)

latex_dunn = dunn.to_latex(
    caption="Wyniki testu Dunna z korekcją Bonferroniego dla wartości HV.",
    label="tab:dunn_global",
    float_format="%.6f"
)

with open("tabela_dunn_global.tex", "w", encoding="utf-8") as f:
    f.write(latex_dunn)

# =========================
# 8B. TEST DUNNA GLOBALNIE - ŁADNA TABELA PAR
# =========================

dunn_pairs = pd.DataFrame([
    {
        "Porównanie": "GEN vs ACO",
        "p": dunn.loc["GEN", "ACO"],
        "Wniosek": "istotna różnica" if dunn.loc["GEN", "ACO"] < 0.05 else "brak istotności"
    },
    {
        "Porównanie": "GEN vs PSO",
        "p": dunn.loc["GEN", "PSO"],
        "Wniosek": "istotna różnica" if dunn.loc["GEN", "PSO"] < 0.05 else "brak istotności"
    },
    {
        "Porównanie": "ACO vs PSO",
        "p": dunn.loc["ACO", "PSO"],
        "Wniosek": "istotna różnica" if dunn.loc["ACO", "PSO"] < 0.05 else "brak istotności"
    }
])

latex_dunn_pairs = dunn_pairs.to_latex(
    index=False,
    caption="Wyniki testu Dunna z korekcją Bonferroniego dla par algorytmów.",
    label="tab:dunn_pairs",
    float_format="%.6f"
)

with open("tabela_dunn_pary.tex", "w", encoding="utf-8") as f:
    f.write(latex_dunn_pairs)

dunn_pairs.to_csv("tabela_dunn_pary.csv", index=False)

# =========================
# 9. ZAPIS TABEL CSV POMOCNICZO
# =========================

hv_global.to_csv("tabela_hv_globalnie.csv", index=False)
kruskal_global.to_csv("tabela_kruskal_global.csv", index=False)
kruskal_exp.to_csv("tabela_kruskal_eksperymenty.csv", index=False)
dunn.to_csv("tabela_dunn_global.csv")

# =========================
# CZAS WYKONANIA ALGORYTMÓW
# =========================

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

raw = pd.read_csv("output.csv", header=None, names=columns)

# bierzemy po jednym czasie na każde uruchomienie,
# bo jedno uruchomienie może mieć kilka tras Pareto z tym samym czasem wykonania
exec_runs = (
    raw
    .groupby(["nr_eksperymentu", "algorytm", "nr_iteracji"])
    .agg(
        czas_wykonania=("czas_wykonania", "max"),
        czy_poprawny=("czy_poprawny", "max")
    )
    .reset_index()
)

# opcja A: wszystkie uruchomienia, także niepoprawne
exec_summary = (
    exec_runs
    .groupby("algorytm")
    .agg(
        sredni_czas_ms=("czas_wykonania", "mean"),
        mediana_czasu_ms=("czas_wykonania", "median"),
        odchylenie_czasu_ms=("czas_wykonania", "std"),
        min_czas_ms=("czas_wykonania", "min"),
        max_czas_ms=("czas_wykonania", "max")
    )
    .reindex(alg_order)
    .reset_index()
)

exec_summary.to_csv("czas_wykonania_podsumowanie.csv", index=False)

print("\n=== Czas wykonania globalnie ===")
print(exec_summary)


# =========================
# WYKRES CZASU WYKONANIA
# =========================

plt.figure(figsize=(7, 5))
plt.bar(
    exec_summary["algorytm"],
    exec_summary["sredni_czas_ms"],
    yerr=exec_summary["odchylenie_czasu_ms"],
    capsize=5
)
plt.ylabel("Średni czas wykonania [ms]")
plt.xlabel("Algorytm")
plt.title("Średni czas wykonania algorytmów")
plt.grid(axis="y", alpha=0.3)
plt.tight_layout()
plt.savefig("wykres_czas_wykonania.png", dpi=300)
plt.close()


# =========================
# TABELA LATEX CZASU WYKONANIA
# =========================

exec_latex = exec_summary.round(2).to_latex(
    index=False,
    caption="Porównanie czasu wykonania algorytmów.",
    label="tab:czas_wykonania",
    float_format="%.2f"
)

with open("tabela_czas_wykonania.tex", "w", encoding="utf-8") as f:
    f.write(exec_latex)


print("Gotowe. Wygenerowano:")
print("- wykres_boxplot_HV.png")
print("- wykres_srednie_HV.png")
print("- wykres_success_rate.png")
print("- tabela_hv_srednie.tex")
print("- tabela_success_rate.tex")
print("- tabela_kruskal_global.tex")
print("- tabela_kruskal_eksperymenty.tex")
print("- tabela_dunn_global.tex")