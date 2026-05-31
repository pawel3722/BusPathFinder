#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Analiza wyników algorytmów GEN / ACO / PSO z poprawnym porównaniem jakości tras.

CSV z programu C++ nie ma nagłówka i ma kolumny:
experiment, algorithm, iteration, arrival, travel, waiting, transfers, computation

Metodologia:
- arrival / travel / waiting / transfers NIE są uśredniane jako wartości bezwzględne
  między różnymi eksperymentami, bo eksperymenty mają różne relacje i godziny startu.
- Dla każdej instancji, czyli pary (experiment, iteration), wyznaczamy najlepszy
  wynik osiągnięty przez dowolny algorytm.
- Następnie dla każdego algorytmu liczymy stratę / odchylenie od tego najlepszego wyniku.
- Jeżeli algorytm zwraca wiele rozwiązań Pareto, dla każdej metryki wybieramy jego
  najlepszą wartość osobno.
- Czas obliczeń analizujemy bezpośrednio, bo jest niezależny od godziny odjazdu.

Przykład:
    python plot_results_polish.py output.csv --out wykresy

Wymagania:
    pip install pandas matplotlib
"""

from __future__ import annotations

import argparse
from pathlib import Path
import zipfile

import matplotlib.pyplot as plt
import pandas as pd


COLUMNS = [
    "experiment",
    "algorithm",
    "iteration",
    "arrival",
    "travel",
    "waiting",
    "transfers",
    "computation",
]

ALGORITHMS_ORDER = ["GEN", "ACO", "PSO"]
QUALITY_METRICS = ["arrival", "travel", "waiting", "transfers"]

METRIC_INFO = {
    "arrival": {
        "name": "czas przyjazdu",
        "loss_col": "strata_czasu_przyjazdu_min",
        "ylabel": "Strata względem najlepszego czasu przyjazdu [min]",
        "bar_title": "Średnia strata względem najlepszego czasu przyjazdu",
        "box_title": "Rozkład straty względem najlepszego czasu przyjazdu",
        "line_title": "Strata względem najlepszego czasu przyjazdu w eksperymentach",
        "file": "czas_przyjazdu",
    },
    "travel": {
        "name": "czas podróży",
        "loss_col": "strata_czasu_podrozy_min",
        "ylabel": "Strata względem najlepszego czasu podróży [min]",
        "bar_title": "Średnia strata względem najlepszego czasu podróży",
        "box_title": "Rozkład straty względem najlepszego czasu podróży",
        "line_title": "Strata względem najlepszego czasu podróży w eksperymentach",
        "file": "czas_podrozy",
    },
    "waiting": {
        "name": "czas oczekiwania",
        "loss_col": "strata_czasu_oczekiwania_min",
        "ylabel": "Strata względem najkrótszego czasu oczekiwania [min]",
        "bar_title": "Średnia strata względem najkrótszego czasu oczekiwania",
        "box_title": "Rozkład straty względem najkrótszego czasu oczekiwania",
        "line_title": "Strata względem najkrótszego czasu oczekiwania w eksperymentach",
        "file": "czas_oczekiwania",
    },
    "transfers": {
        "name": "liczba przesiadek",
        "loss_col": "nadmiar_przesiadek",
        "ylabel": "Nadmiar przesiadek względem najlepszego wyniku",
        "bar_title": "Średni nadmiar przesiadek względem najlepszego wyniku",
        "box_title": "Rozkład nadmiaru przesiadek względem najlepszego wyniku",
        "line_title": "Nadmiar przesiadek względem najlepszego wyniku w eksperymentach",
        "file": "przesiadki",
    },
}


def ordered_algorithms(values) -> list[str]:
    present = list(pd.Series(values).dropna().unique())
    ordered = [a for a in ALGORITHMS_ORDER if a in present]
    ordered.extend([a for a in present if a not in ordered])
    return ordered


def read_results(csv_path: Path) -> pd.DataFrame:
    df = pd.read_csv(csv_path, header=None, names=COLUMNS)
    df["algorithm"] = df["algorithm"].astype(str).str.strip().str.upper()

    for col in [c for c in COLUMNS if c != "algorithm"]:
        df[col] = pd.to_numeric(df[col], errors="coerce")

    df = df.dropna(subset=COLUMNS)

    for col in [c for c in COLUMNS if c != "algorithm"]:
        df[col] = df[col].astype(int)

    # Rekord 0,0,0,0 oznacza brak sensownej trasy, więc nie wchodzi do jakości trasy.
    # Czas obliczeń nadal może być użyty do analizy czasu działania algorytmu.
    df["poprawna_trasa"] = ~(
        (df["arrival"] == 0)
        & (df["travel"] == 0)
        & (df["waiting"] == 0)
        & (df["transfers"] == 0)
    )
    return df


def reduce_pareto_to_best_per_metric(df: pd.DataFrame) -> pd.DataFrame:
    """
    Dla wielu rozwiązań Pareto wybiera najlepszą wartość algorytmu osobno dla każdej metryki.
    Wynik: experiment, iteration, algorithm, metric, wartosc_algorytmu
    """
    valid = df[df["poprawna_trasa"]].copy()
    group_cols = ["experiment", "iteration", "algorithm"]
    pieces = []

    for metric in QUALITY_METRICS:
        part = (
            valid.groupby(group_cols, as_index=False)[metric]
            .min()
            .rename(columns={metric: "wartosc_algorytmu"})
        )
        part["metric"] = metric
        pieces.append(part)

    return pd.concat(pieces, ignore_index=True)


def compute_relative_losses(reduced: pd.DataFrame) -> pd.DataFrame:
    """
    Liczy stratę względem najlepszego wyniku w tej samej instancji:
    experiment + iteration + metric.
    """
    best = (
        reduced.groupby(["experiment", "iteration", "metric"], as_index=False)["wartosc_algorytmu"]
        .min()
        .rename(columns={"wartosc_algorytmu": "najlepszy_wynik_w_instancji"})
    )
    result = reduced.merge(best, on=["experiment", "iteration", "metric"], how="left")
    result["strata_wzgledem_najlepszego"] = (
        result["wartosc_algorytmu"] - result["najlepszy_wynik_w_instancji"]
    )

    # Dodatkowa kolumna z ładną nazwą metryki do tabel.
    result["metryka"] = result["metric"].map(lambda m: METRIC_INFO[m]["name"])
    return result


def computation_per_attempt(df: pd.DataFrame) -> pd.DataFrame:
    return df.groupby(["experiment", "iteration", "algorithm"], as_index=False)["computation"].first()


def compute_success_rate(df: pd.DataFrame) -> pd.DataFrame:
    attempts = (
        df.groupby(["experiment", "iteration", "algorithm"], as_index=False)["poprawna_trasa"]
        .any()
        .rename(columns={"poprawna_trasa": "czy_znaleziono_trase"})
    )
    success = (
        attempts.groupby("algorithm", as_index=False)["czy_znaleziono_trase"]
        .mean()
        .rename(columns={"czy_znaleziono_trase": "skutecznosc"})
    )
    success["skutecznosc_procent"] = success["skutecznosc"] * 100
    return success


def save_bar_chart(data: pd.DataFrame, x: str, y: str, title: str, ylabel: str, path: Path) -> None:
    algs = ordered_algorithms(data[x])
    data = data.set_index(x).loc[algs].reset_index()

    plt.figure(figsize=(10, 6))
    bars = plt.bar(data[x], data[y])
    plt.title(title)
    plt.xlabel("Algorytm")
    plt.ylabel(ylabel)
    plt.grid(axis="y", alpha=0.3)

    for bar in bars:
        height = bar.get_height()
        plt.text(
            bar.get_x() + bar.get_width() / 2,
            height,
            f"{height:.2f}",
            ha="center",
            va="bottom",
            fontsize=9,
        )

    plt.tight_layout()
    plt.savefig(path, dpi=240)
    plt.close()


def save_boxplot(data: pd.DataFrame, value_col: str, title: str, ylabel: str, path: Path) -> None:
    algs = ordered_algorithms(data["algorithm"])
    values = [data.loc[data["algorithm"] == alg, value_col].dropna().values for alg in algs]

    plt.figure(figsize=(10, 6))
    plt.boxplot(values, labels=algs, showmeans=True)
    plt.title(title)
    plt.xlabel("Algorytm")
    plt.ylabel(ylabel)
    plt.grid(axis="y", alpha=0.3)
    plt.tight_layout()
    plt.savefig(path, dpi=240)
    plt.close()


def save_line_chart(data: pd.DataFrame, title: str, ylabel: str, path: Path) -> None:
    algs = ordered_algorithms(data["algorithm"])

    plt.figure(figsize=(11, 6))
    for alg in algs:
        part = data[data["algorithm"] == alg].sort_values("experiment")
        plt.plot(part["experiment"], part["srednia_wartosc"], marker="o", label=alg)

    plt.title(title)
    plt.xlabel("Numer eksperymentu")
    plt.ylabel(ylabel)
    plt.legend(title="Algorytm")
    plt.grid(alpha=0.3)
    plt.tight_layout()
    plt.savefig(path, dpi=240)
    plt.close()


def create_summary(losses: pd.DataFrame, comp: pd.DataFrame, success: pd.DataFrame) -> pd.DataFrame:
    quality_summary = (
        losses.groupby(["metryka", "algorithm"], as_index=False)["strata_wzgledem_najlepszego"]
        .agg(
            srednia_strata="mean",
            mediana_straty="median",
            odchylenie_standardowe="std",
            minimum="min",
            maksimum="max",
        )
    )

    comp_summary = (
        comp.groupby("algorithm", as_index=False)["computation"]
        .agg(
            srednia_strata="mean",
            mediana_straty="median",
            odchylenie_standardowe="std",
            minimum="min",
            maksimum="max",
        )
    )
    comp_summary["metryka"] = "czas obliczeń [ms]"

    summary = pd.concat([quality_summary, comp_summary], ignore_index=True)
    summary = summary.merge(success[["algorithm", "skutecznosc_procent"]], on="algorithm", how="left")
    summary = summary.rename(columns={"algorithm": "algorytm"})
    return summary


def make_zip(out_dir: Path) -> Path:
    zip_path = out_dir.with_suffix(".zip")
    with zipfile.ZipFile(zip_path, "w", compression=zipfile.ZIP_DEFLATED) as zf:
        for file in sorted(out_dir.rglob("*")):
            if file.is_file():
                zf.write(file, arcname=file.relative_to(out_dir.parent))
    return zip_path


def generate(csv_path: Path, out_dir: Path) -> None:
    out_dir.mkdir(parents=True, exist_ok=True)

    df = read_results(csv_path)
    reduced = reduce_pareto_to_best_per_metric(df)
    losses = compute_relative_losses(reduced)
    comp = computation_per_attempt(df)
    success = compute_success_rate(df)
    summary = create_summary(losses, comp, success)

    # Tabele pomocnicze z polskimi nazwami.
    df.to_csv(out_dir / "01_dane_wejsciowe_z_flaga_poprawnosci.csv", index=False)
    reduced.to_csv(out_dir / "02_najlepsze_wyniki_algorytmow_per_metryka.csv", index=False)
    losses.to_csv(out_dir / "03_straty_wzgledem_najlepszego_wyniku.csv", index=False)
    comp.to_csv(out_dir / "04_czas_obliczen_per_proba.csv", index=False)
    success.to_csv(out_dir / "05_skutecznosc_algorytmow.csv", index=False)
    summary.to_csv(out_dir / "00_podsumowanie_do_prezentacji.csv", index=False)

    # Czas obliczeń — wartości bezwzględne.
    comp_mean = comp.groupby("algorithm", as_index=False)["computation"].mean()
    save_bar_chart(
        comp_mean,
        "algorithm",
        "computation",
        "Średni czas obliczeń algorytmów",
        "Czas obliczeń [ms]",
        out_dir / "01_sredni_czas_obliczen_algorytmow.png",
    )
    save_boxplot(
        comp,
        "computation",
        "Rozkład czasu obliczeń algorytmów",
        "Czas obliczeń [ms]",
        out_dir / "02_rozklad_czasu_obliczen_algorytmow.png",
    )
    comp_exp = (
        comp.groupby(["experiment", "algorithm"], as_index=False)["computation"]
        .mean()
        .rename(columns={"computation": "srednia_wartosc"})
    )
    save_line_chart(
        comp_exp,
        "Średni czas obliczeń w kolejnych eksperymentach",
        "Czas obliczeń [ms]",
        out_dir / "03_czas_obliczen_w_kolejnych_eksperymentach.png",
    )

    # Jakość tras — wyłącznie jako strata/odchylenie względem najlepszego wyniku instancji.
    for metric in QUALITY_METRICS:
        info = METRIC_INFO[metric]
        part = losses[losses["metric"] == metric].copy()
        part[info["loss_col"]] = part["strata_wzgledem_najlepszego"]

        mean_loss = (
            part.groupby("algorithm", as_index=False)[info["loss_col"]]
            .mean()
        )
        save_bar_chart(
            mean_loss,
            "algorithm",
            info["loss_col"],
            info["bar_title"],
            info["ylabel"],
            out_dir / f"04_srednia_strata_{info['file']}.png",
        )

        save_boxplot(
            part,
            info["loss_col"],
            info["box_title"],
            info["ylabel"],
            out_dir / f"05_rozklad_straty_{info['file']}.png",
        )

        exp = (
            part.groupby(["experiment", "algorithm"], as_index=False)[info["loss_col"]]
            .mean()
            .rename(columns={info["loss_col"]: "srednia_wartosc"})
        )
        save_line_chart(
            exp,
            info["line_title"],
            info["ylabel"],
            out_dir / f"06_strata_{info['file']}_w_eksperymentach.png",
        )

    save_bar_chart(
        success,
        "algorithm",
        "skutecznosc_procent",
        "Skuteczność algorytmów — odsetek prób z poprawną trasą",
        "Poprawne próby [%]",
        out_dir / "07_skutecznosc_algorytmow.png",
    )

    zip_path = make_zip(out_dir)
    print(f"Gotowe: {out_dir.resolve()}")
    print(f"Paczka ZIP: {zip_path.resolve()}")


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Generuje polsko opisane wykresy wyników GEN/ACO/PSO bez używania nazwy 'gap'."
    )
    parser.add_argument("csv", type=Path, help="Ścieżka do pliku CSV, np. output.csv")
    parser.add_argument("--out", type=Path, default=Path("wykresy_polskie"), help="Folder wyjściowy")
    args = parser.parse_args()
    generate(args.csv, args.out)


if __name__ == "__main__":
    main()
