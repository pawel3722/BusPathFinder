import csv

TRIPS_FILE = "trips.txt"
STOP_TIMES_FILE = "stop_times.txt"
OUTPUT_FILE = "stop_times_filtered.txt"

trip_ids = set()

with open(TRIPS_FILE, "r", encoding="utf-8-sig", newline="") as f:
    reader = csv.DictReader(f)
    reader.fieldnames = [name.strip() for name in reader.fieldnames]

    for row in reader:
        trip_ids.add(row["trip_id"].strip())

with open(STOP_TIMES_FILE, "r", encoding="utf-8-sig", newline="") as infile, \
     open(OUTPUT_FILE, "w", encoding="utf-8", newline="") as outfile:

    reader = csv.DictReader(infile)
    reader.fieldnames = [name.strip() for name in reader.fieldnames]

    writer = csv.DictWriter(outfile, fieldnames=reader.fieldnames)
    writer.writeheader()

    for row in reader:
        if row["trip_id"].strip() in trip_ids:
            writer.writerow(row)

print(f"Zapisano wynik do: {OUTPUT_FILE}")
print(f"Liczba trip_id: {len(trip_ids)}")