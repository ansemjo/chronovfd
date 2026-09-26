#!/usr/bin/env -S uv run
# /// script
# dependencies = [
#   "matplotlib>=3.7.0",
# ]
# ///

import sys
import csv
import matplotlib.pyplot as plt

def plot_fm_data(csv_file):
    # Read data from CSV
    frequencies = []
    rssis = []
    snrs = []

    with open(csv_file, 'r') as f:
        reader = csv.reader(f)
        next(reader)  # Skip header if present
        for row in reader:
            if len(row) >= 3:
                try:
                    freq = float(row[0])
                    rssi = float(row[1])
                    snr = float(row[2])
                    frequencies.append(freq)
                    rssis.append(rssi)
                    snrs.append(snr)
                except ValueError:
                    continue

    if not frequencies:
        print("No valid data found in the CSV file.")
        return

    # Create figure and axis
    fig, ax1 = plt.subplots(figsize=(17, 4))
    limits = (10, 45)
    logscale = True

    # Plot RSSI on primary axis
    color = 'tab:red'
    ax1.set_xlabel('Frequency (MHz)')
    ax1.set_ylabel('RSSI (dBm)', color=color)
    if logscale: ax1.set_yscale('log')
    ax1.set_ylim(*limits)
    ax1.plot(frequencies, rssis, color=color, label='RSSI')
    ax1.tick_params(axis='y', labelcolor=color)

    # Create secondary axis for SNR
    ax2 = ax1.twinx()
    color = 'tab:blue'
    ax2.set_ylabel('SNR (dB)', color=color)
    if logscale: ax2.set_yscale('log')
    ax2.set_ylim(*limits)
    ax2.plot(frequencies, snrs, color=color, label='SNR')
    ax2.tick_params(axis='y', labelcolor=color)

    # Add title and legend
    plt.title('FM Frequency Analysis')
    fig.legend(loc="upper right", bbox_to_anchor=(1,1), bbox_transform=ax1.transAxes)

    # Adjust layout
    fig.tight_layout()

    # Show plot
    plt.show()

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python plot_fm_data.py <csv_file>")
        sys.exit(1)

    csv_file = sys.argv[1]
    plot_fm_data(csv_file)
