import json
import math
import random

INPUT_FILE = "gd_stops.json"
OUTPUT_FILE = "input.txt"

# liczba par dla każdej kategorii
N = 3

# zakres godzin
START_HOUR = 6
END_HOUR = 21

# =========================
# Wczytanie przystanków
# =========================

with open(INPUT_FILE, encoding="utf-8") as f:
    stops = json.load(f)

# =========================
# Distance helper
# =========================

def haversine(lat1, lon1, lat2, lon2):
    R = 6371.0

    lat1 = math.radians(lat1)
    lon1 = math.radians(lon1)
    lat2 = math.radians(lat2)
    lon2 = math.radians(lon2)

    dlat = lat2 - lat1
    dlon = lon2 - lon1

    a = (
        math.sin(dlat / 2) ** 2
        + math.cos(lat1)
        * math.cos(lat2)
        * math.sin(dlon / 2) ** 2
    )

    c = 2 * math.atan2(math.sqrt(a), math.sqrt(1 - a))

    return R * c  # km

# =========================
# Generowanie wszystkich par
# =========================

pairs = []

for i in range(len(stops)):
    for j in range(i + 1, len(stops)):
        a = stops[i]
        b = stops[j]

        dist = haversine(
            a["lat"],
            a["lon"],
            b["lat"],
            b["lon"]
        )

        pairs.append({
            "start": a["id"],
            "end": b["id"],
            "distance": dist
        })

# =========================
# Sortowanie po dystansie
# =========================

pairs.sort(key=lambda x: x["distance"])

total = len(pairs)

# podział na grupy
close_pairs = pairs[: total // 3]
medium_pairs = pairs[total // 3 : 2 * total // 3]
far_pairs = pairs[2 * total // 3 :]

# losowanie
selected = []

selected += random.sample(close_pairs, min(N, len(close_pairs)))
selected += random.sample(medium_pairs, min(N, len(medium_pairs)))
selected += random.sample(far_pairs, min(N, len(far_pairs)))

#random.shuffle(selected)

# =========================
# Losowa godzina
# =========================

def random_time():
    hour = random.randint(START_HOUR, END_HOUR)
    minute = random.randint(0, 59)

    return f"{hour:02}:{minute:02}"

# =========================
# Zapis
# =========================

with open(OUTPUT_FILE, "w", encoding="utf-8") as f:
    for pair in selected:
        line = (
            f"{pair['start']} "
            f"{pair['end']} "
            f"{random_time()}"
        )

        f.write(line + "\n")

print(f"Zapisano {len(selected)} par do {OUTPUT_FILE}")