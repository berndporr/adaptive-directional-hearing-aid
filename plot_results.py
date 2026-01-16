import numpy as np
import matplotlib.pyplot as plt
from scipy.io import wavfile

# Hardcoded file paths
wav_path1 = "result.wav"            # filtered
wav_path2 = "result_unfiltered.wav" # unfiltered

# Read first WAV
sr1, data1 = wavfile.read(wav_path1)
if data1.ndim == 2:
    data1 = data1[:, 0]  # left channel
if data1.dtype != np.float32 and data1.dtype != np.float64:
    data1 = data1.astype(np.float32) / np.iinfo(data1.dtype).max

# Read second WAV
sr2, data2 = wavfile.read(wav_path2)
if data2.ndim == 2:
    data2 = data2[:, 0]  # left channel
if data2.dtype != np.float32 and data2.dtype != np.float64:
    data2 = data2.astype(np.float32) / np.iinfo(data2.dtype).max

# Make time axes
time1 = np.arange(len(data1)) / sr1
time2 = np.arange(len(data2)) / sr2

# Make difference signal (truncate to shortest length)
min_len = min(len(data1), len(data2))
abs1 = np.abs(data1[:min_len])
abs2 = np.abs(data2[:min_len])
diff = abs2 - abs1
time_diff = np.arange(len(diff)) / sr1  # same sample rate
diff = np.clip(diff, -1.0, 1.0)

max_amp = max(
    np.max(np.abs(data1)),
    np.max(np.abs(data2)),
    np.max(np.abs(diff))
)


# Plot
plt.figure(figsize=(14, 9))

# First subplot: filtered
plt.subplot(3, 1, 1)
plt.plot(time1, data1, color='blue')
plt.ylim(-max_amp, max_amp)
plt.title(f"Filtered Waveform: {wav_path1}")
plt.xlabel("Time (s)")
plt.ylabel("Amplitude")
plt.grid(True)

# Second subplot: unfiltered
plt.subplot(3, 1, 2)
plt.plot(time2, data2, color='green')
plt.ylim(-max_amp, max_amp)
plt.title(f"Unfiltered Waveform: {wav_path2}")
plt.xlabel("Time (s)")
plt.ylabel("Amplitude")
plt.grid(True)

# Third subplot: difference
plt.subplot(3, 1, 3)
plt.plot(time_diff, diff, color='red')
plt.title("Difference (Unfiltered - Filtered)(absoluted)")
plt.xlabel("Time (s)")
plt.ylabel("Amplitude")
plt.grid(True)

plt.tight_layout()
plt.show()
