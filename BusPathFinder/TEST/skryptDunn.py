import pandas as pd
import scikit_posthocs as sp

# wczytaj wyniki HV
df = pd.read_csv("hv_per_uruchomienie.csv")

print("=== TEST DUNNA (globalnie) ===")

dunn = sp.posthoc_dunn(
    df,
    val_col="HV",
    group_col="algorytm",
    p_adjust="bonferroni"
)

print(dunn)

print("\n")

print("=== TEST DUNNA DLA POSZCZEGÓLNYCH EKSPERYMENTÓW ===")

for exp_id, group in df.groupby("nr_eksperymentu"):

    alg_count = group["algorytm"].nunique()

    if alg_count < 3:
        print(f"\nEksperyment {exp_id}: pominięty (za mało danych)")
        continue

    print(f"\nEksperyment {exp_id}")

    dunn_exp = sp.posthoc_dunn(
        group,
        val_col="HV",
        group_col="algorytm",
        p_adjust="bonferroni"
    )

    print(dunn_exp)