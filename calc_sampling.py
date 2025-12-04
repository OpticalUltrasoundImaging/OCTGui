import math


fs = 180e6
print(f"OCT system DAQ {fs=:.3g}")

aline_size = 8192
time_to_acquire = aline_size/fs
max_aline_per_sec = 1/time_to_acquire
print("Old laser:")
print(f"{aline_size=}, {time_to_acquire=:.3g} s, {max_aline_per_sec=:.2f}")
# 45us to acquire an aline, 21.9k alines per second max
# Old laser 20kHz scan rate.

# New laser params
# sweep period: 8us
period = 8e-6
samples_per_period = period * fs
print("\nNew laser:")
print(f"Sweep {period=} s, freq={1/period} Hz")
print(f"With {fs=:.3g}, {samples_per_period=}")

new_aline_size = 2**math.floor(math.log2(samples_per_period))
print(f"{new_aline_size=}, which is {new_aline_size/samples_per_period:.1%} of period")

# one period corresponds to ~1440 samples. Acquire 1024 samples per aline
