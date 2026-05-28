import csv
import json
import re
from collections import defaultdict

INPUT_FILE = "stops.txt"
OUTPUT_FILE = "stops.json"


def split_stop_name(name):
    """
    'Wrzeszcz PKP 03' -> ('Wrzeszcz PKP', '03')
    """
    match = re.match(r"^(.*)\s+(\d+)$", name.strip())

    if match:
        base_name = match.group(1).strip()
        platform = match.group(2)
        return base_name, platform

    return name.strip(), None


grouped = defaultdict(list)

with open(INPUT_FILE, newline="", encoding="utf-8") as csvfile:
    reader = csv.DictReader(csvfile)

    for row in reader:
        base_name, _ = split_stop_name(row["stop_name"])

        grouped[base_name].append({
            "platform_id": int(row["stop_id"]),
            "lat": float(row["stop_lat"]),
            "lon": float(row["stop_lon"]),
        })

result = []

for stop_name, entries in grouped.items():
    # ID przystanku = najmniejsze ID platformy
    stop_id = min(e["platform_id"] for e in entries)

    # średnia lokalizacja
    avg_lat = sum(e["lat"] for e in entries) / len(entries)
    avg_lon = sum(e["lon"] for e in entries) / len(entries)

    # lista ID platform
    platforms = sorted(e["platform_id"] for e in entries)

    result.append({
        "id": stop_id,
        "name": stop_name,
        "lat": avg_lat,
        "lon": avg_lon,
        "platforms": platforms
    })

# sortowanie po ID
result.sort(key=lambda x: x["id"])

with open(OUTPUT_FILE, "w", encoding="utf-8") as f:
    json.dump(result, f, ensure_ascii=False, indent=2)

print(f"Zapisano {len(result)} przystanków do {OUTPUT_FILE}")