# This program imports and runs the compiled data acuqisition C library for DT9837
import ctypes

# Config Params
NUM_CHANNELS = 4  # Max 4 for DT9837
ALL_CHANNEL_GAIN = 10
CLOCK_FREQUENCY = 1000.0

CHANNEL_GAIN_0 = 10  # Z
CHANNEL_GAIN_1 = 10  # Y
CHANNEL_GAIN_2 = 10  # X
CHANNEL_GAIN_3 = 10  # N/A


class DT9837():
    def __init__(self):
        dt_lib = ctypes.CDLL(
            "C:\_Automation\Win32\SDK\Examples\DtConsole\dt_lib.so")
        self.measure = dt_lib.main

    def measure(self, use_default_vals=True):
        self.measure.argtypes = [ctypes.c_bool, ctypes.c_int, ctypes.c_float,
                                 ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int]
        self.measure(use_default_vals, NUM_CHANNELS,
                     CLOCK_FREQUENCY, ALL_CHANNEL_GAIN, CHANNEL_GAIN_0, CHANNEL_GAIN_1, CHANNEL_GAIN_2, CHANNEL_GAIN_3)


if __name__ == "__main__":
    signalanalyzer = DT9837()

    # signalanalyzer.measure()
    signalanalyzer.measure(use_default_vals=False)
