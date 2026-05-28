import csv
import json

STOP_TIMES_FILE = "stop_times.txt"
TRIPS_JSON_FILE = "trips.json"
OUTPUT_FILE = "stop_times.json"

# =========================
# Wczytanie dozwolonych trip_id z trips.json
# =========================

with open(TRIPS_JSON_FILE, encoding="utf-8") as f:
    trips_data = json.load(f)

valid_trip_ids = {
    trip["id"]
    for trip in trips_data["trips"]
}

# =========================
# Filtrowanie stop_times.txt
# =========================

stop_times = []

with open(STOP_TIMES_FILE, newline="", encoding="utf-8") as stop_times_file:
    reader = csv.DictReader(stop_times_file)

    for row in reader:
        trip_id = row["trip_id"]

        # zostaw tylko tripy istniejące w trips.json
        if trip_id not in valid_trip_ids:
            continue

        stop_times.append({
            "trip_id": trip_id,
            "time": row["arrival_time"],
            "stop_id": int(row["stop_id"]),
            "index": int(row["stop_sequence"])
        })

# =========================
# Zapis JSON
# =========================

output = {
    "stopTimes": stop_times
}

with open(OUTPUT_FILE, "w", encoding="utf-8") as f:
    json.dump(output, f, ensure_ascii=False, indent=2)

print(f"Zapisano {len(stop_times)} rekordów do {OUTPUT_FILE}")