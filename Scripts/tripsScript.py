import csv
import json

TRIPS_FILE = "trips.txt"
ROUTES_FILE = "routes.txt"
OUTPUT_FILE = "trips.json"

TARGET_DATE = "20260512"

# =========================
# Wczytanie routes.txt
# route_id -> route_short_name
# =========================

routes_map = {}

with open(ROUTES_FILE, newline="", encoding="utf-8") as routes_file:
    reader = csv.DictReader(routes_file)

    for row in reader:
        routes_map[row["route_id"]] = row["route_short_name"]

# =========================
# Przetwarzanie trips.txt
# =========================

trips = []

with open(TRIPS_FILE, newline="", encoding="utf-8") as trips_file:
    reader = csv.DictReader(trips_file)

    for row in reader:
        service_id = row["service_id"]

        # zostaw tylko rekordy z service_id kończącym się na 20260512
        if not service_id.endswith(TARGET_DATE):
            continue

        route_id = row["route_id"]

        trips.append({
            "id": row["trip_id"],
            "line": routes_map.get(route_id, route_id),
            "direction": row["trip_headsign"],
            "shape_id": row["shape_id"]
        })

# =========================
# Zapis JSON
# =========================

output = {
    "trips": trips
}

with open(OUTPUT_FILE, "w", encoding="utf-8") as f:
    json.dump(output, f, ensure_ascii=False, indent=2)

print(f"Zapisano {len(trips)} tripów do {OUTPUT_FILE}")