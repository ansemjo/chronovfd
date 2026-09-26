import csv
from operator import itemgetter

def analyze_fm_scan(csv_file):
    # Read the CSV data
    with open(csv_file, 'r') as f:
        reader = csv.DictReader(f)
        data = [row for row in reader]

    # Convert string values to appropriate types
    for row in data:
        row['frequency'] = float(row['frequency'])
        row['rssi'] = float(row['rssi'])
        row['snr'] = float(row['snr'])

    # 1. Get top 10 by SNR
    top_snr = sorted(data, key=itemgetter('snr'), reverse=True)[:10]

    # 2. Create weighted score (50% RSSI, 50% SNR)
    # First normalize the values to a similar scale (simple min-max normalization)
    if data:
        min_rssi = min(row['rssi'] for row in data)
        max_rssi = max(row['rssi'] for row in data)
        min_snr = min(row['snr'] for row in data)
        max_snr = max(row['snr'] for row in data)

        for row in data:
            # Normalize to 0-1 range
            norm_rssi = (row['rssi'] - min_rssi) / (max_rssi - min_rssi) if (max_rssi - min_rssi) != 0 else 0
            norm_snr = (row['snr'] - min_snr) / (max_snr - min_snr) if (max_snr - min_snr) != 0 else 0
            # Weighted score (50/50)
            row['weighted_score'] = 0.5 * norm_rssi + 0.5 * norm_snr

        top_weighted = sorted(data, key=itemgetter('weighted_score'), reverse=True)[:10]

    # Print results
    print("Top 10 by SNR:")
    print("{:<12} {:<8} {:<8} {:<8}".format("Frequency", "RSSI", "SNR", "Score"))
    for row in top_snr:
        print("{:<12.3f} {:<8.2f} {:<8.2f} {:<8.2f}".format(
            row['frequency'], row['rssi'], row['snr'], row.get('weighted_score', 0)))

    print("\nTop 10 by weighted score (50% RSSI, 50% SNR):")
    print("{:<12} {:<8} {:<8} {:<8}".format("Frequency", "RSSI", "SNR", "Score"))
    for row in top_weighted:
        print("{:<12.3f} {:<8.2f} {:<8.2f} {:<8.2f}".format(
            row['frequency'], row['rssi'], row['snr'], row['weighted_score']))

if __name__ == "__main__":
    import sys
    if len(sys.argv) != 2:
        print("Usage: python fm_analysis.py <csv_file>")
        sys.exit(1)

    analyze_fm_scan(sys.argv[1])