import csv
import math
import random

INPUT_FILE = "stops.txt"
OUTPUT_FILE = "input.txt"

N = 5

START_HOUR = 6
END_HOUR = 21


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
        + math.cos(lat1) * math.cos(lat2) * math.sin(dlon / 2) ** 2
    )

    return R * 2 * math.atan2(math.sqrt(a), math.sqrt(1 - a))


def random_time():
    hour = random.randint(START_HOUR, END_HOUR)
    minute = random.randint(0, 59)
    return f"{hour:02}:{minute:02}"


# =========================
# Wczytanie przystanków z GTFS stops.txt
# =========================

stops = []

with open(INPUT_FILE, newline="", encoding="utf-8-sig") as f:
    reader = csv.DictReader(f)

    for row in reader:
        stops.append({
            "id": int(row["stop_id"]),
            "lat": float(row["stop_lat"]),
            "lon": float(row["stop_lon"]),
            "name": row["stop_name"]
        })

# =========================
# Generowanie par
# =========================

pairs = []

for i in range(len(stops)):
    for j in range(i + 1, len(stops)):
        a = stops[i]
        b = stops[j]

        dist = haversine(a["lat"], a["lon"], b["lat"], b["lon"])

        pairs.append({
            "start": a["id"],
            "end": b["id"],
            "distance": dist
        })

pairs.sort(key=lambda x: x["distance"])

total = len(pairs)

print(f"MIN odleglosc: {pairs[0]['distance']}")
print(f"Min odleglosc: {pairs[3 * total // 4]['distance']}")
print(f"Max odleglosc: {pairs[-1]['distance']}")

# close_pairs = pairs[: total // 3]
# medium_pairs = pairs[total // 3 : 2 * total // 3]
far_pairs = pairs[3 * total // 4 :]

selected = []

# selected += random.sample(close_pairs, min(N, len(close_pairs)))
# selected += random.sample(medium_pairs, min(N, len(medium_pairs)))
selected += random.sample(far_pairs, min(N, len(far_pairs)))

random.shuffle(selected)

# =========================
# Zapis input.txt
# =========================

with open(OUTPUT_FILE, "w", encoding="utf-8-sig") as f:
    for pair in selected:
        f.write(f"{pair['start']} {pair['end']} {random_time()}\n")

print(f"Zapisano {len(selected)} par do {OUTPUT_FILE}")