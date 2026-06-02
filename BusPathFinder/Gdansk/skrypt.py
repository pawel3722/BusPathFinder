import csv

TRIPS_FILE = "trips.txt"
STOP_TIMES_FILE = "stop_times.txt"
OUTPUT_FILE = "stop_times_filtered.txt"

# Wczytaj wszystkie trip_id z trips.txt
trip_ids = set()

with open(TRIPS_FILE, "r", encoding="utf-8", newline="") as f:
    reader = csv.DictReader(f)

    for row in reader:
        trip_ids.add(row["trip_id"])

# Przefiltruj stop_times.txt
with open(STOP_TIMES_FILE, "r", encoding="utf-8", newline="") as infile, \
     open(OUTPUT_FILE, "w", encoding="utf-8", newline="") as outfile:

    reader = csv.DictReader(infile)
    writer = csv.DictWriter(outfile, fieldnames=reader.fieldnames)

    writer.writeheader()

    for row in reader:
        if row["trip_id"] in trip_ids:
            writer.writerow(row)

print(f"Zapisano wynik do: {OUTPUT_FILE}")
print(f"Liczba trip_id: {len(trip_ids)}")