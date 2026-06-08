import csv

INPUT_FILE = "trips.txt"
OUTPUT_FILE = "trips_filtered.txt"

TARGET_SERVICE_ID = "7069_RO"

rows = []

with open(INPUT_FILE, newline="", encoding="utf-8") as f:
    reader = csv.DictReader(f)

    fieldnames = reader.fieldnames

    for row in reader:
        if row["service_id"] == TARGET_SERVICE_ID:
            rows.append(row)

with open(OUTPUT_FILE, "w", newline="", encoding="utf-8") as f:
    writer = csv.DictWriter(
        f,
        fieldnames=fieldnames,
        quoting=csv.QUOTE_MINIMAL
    )

    writer.writeheader()
    writer.writerows(rows)

print(f"Zapisano {len(rows)} rekordów do {OUTPUT_FILE}")