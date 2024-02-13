# This program imports and runs the compiled data acuqisition C library for DT9837
import ctypes

# Config Params
NUM_CHANNELS = 4  # Max 4 for DT9837
ALL_CHANNEL_GAIN = 1  # 1 or 10
CLOCK_FREQUENCY = 1000.0

CHANNEL_GAIN_0 = 1  # Z
CHANNEL_GAIN_1 = 1  # Y
CHANNEL_GAIN_2 = 1  # X
CHANNEL_GAIN_3 = 1  # N/A

WAVEFORM_AMPLITUDE = 3  # in volts (V)
WAVEFORM_FREQUENCY = 10  # in hertz (Hz)
WAVEFORM_DURATION = 10  # in seconds (s)


class DT9837():
    def __init__(self):
        self.dt_lib = ctypes.CDLL(
            "C:\_Automation\Win32\SDK\Examples\DtConsole\dt_lib.so")

    def measure(self, use_default_vals=True):
        self.measure = self.dt_lib.measure
        self.measure.argtypes = [ctypes.c_bool, ctypes.c_int, ctypes.c_float,
                                 ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int]
        self.measure(use_default_vals, NUM_CHANNELS,
                     CLOCK_FREQUENCY, ALL_CHANNEL_GAIN, CHANNEL_GAIN_0, CHANNEL_GAIN_1, CHANNEL_GAIN_2, CHANNEL_GAIN_3)

    def generate(self, use_default_vals=True, read_input=True):
        self.generate = self.dt_lib.generate
        self.generate.argtypes = [
            ctypes.c_bool,  ctypes.c_bool, ctypes.c_float, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int]
        self.generate(use_default_vals, read_input, 1000.0, ALL_CHANNEL_GAIN,
                      WAVEFORM_AMPLITUDE, WAVEFORM_FREQUENCY, WAVEFORM_DURATION)


if __name__ == "__main__":
    signalanalyzer = DT9837()

    # signalanalyzer.measure()
    # signalanalyzer.measure(use_default_vals=False)
    signalanalyzer.generate(use_default_vals=False, read_input=True)
