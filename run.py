# This program imports and runs the compiled data acuqisition C library for DT9837
import ctypes

# Config Params
NUM_CHANNELS = 4  # Max 4 channels for DT9837
ALL_CHANNEL_GAIN = 1  # 1 or 10
CLOCK_FREQUENCY = 1000.0  # Max: 46875Hz (output), 52700Hz (input)

CHANNEL_GAIN_0 = 1  # Z (1 or 10)
CHANNEL_GAIN_1 = 1  # Y (1 or 10)
CHANNEL_GAIN_2 = 1  # X (1 or 10)
CHANNEL_GAIN_3 = 1  # DAC (1 or 10)

WAVEFORM_AMPLITUDE = 3  # in volts (V) [0V-10V]
WAVEFORM_FREQUENCY = 10  # in hertz (Hz) [10Hz-400Hz]
WAVEFORM_DURATION = 10  # in seconds (s)

# Error Codes
ERR_BOARD_CONFIG = 1
ERR_CHANNEL_CONFIG = 2
ERR_DATA_CONFIG = 3
ERR_MEASUREMENT = 4
ERR_DEINIT_CONFIG = 5


class DT9837():
    def __init__(self):
        """Equipment class for DT9837 signal analyzer
        """
        self.dt_lib = ctypes.CDLL(
            "C:\_Automation\Win32\SDK\Examples\DtConsole\dt_lib.so")

    def measure_acceleration(self, use_default_vals=True):
        """This function measures the acceleration reading

        A 3 axis accelerometer must be connected to the equipment that 
        will be recorded indefinately until keyboard interrupt

        :param use_default_vals: use the set default values of the equipment, defaults to True
        :type use_default_vals: bool, optional
        """
        self.measure = self.dt_lib.measure
        self.measure.argtypes = [ctypes.c_bool, ctypes.c_int, ctypes.c_float,
                                 ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int]
        err_code = self.measure(use_default_vals, NUM_CHANNELS,
                                CLOCK_FREQUENCY, ALL_CHANNEL_GAIN, CHANNEL_GAIN_0, CHANNEL_GAIN_1, CHANNEL_GAIN_2, CHANNEL_GAIN_3)
        self._error_check(err_code)

    def generate_squarewave(self, use_default_vals=True, read_input=True):
        """This function generates a simple squarewave

        Generates a wave at the specified amplitude (V), frequency (Hz) and duration (in s)

        :param use_default_vals: use the set default values of the equipment, defaults to True
        :type use_default_vals: bool, optional
        :param read_input: record the input into a csv, defaults to True
        :type read_input: bool, optional
        """
        self.generate = self.dt_lib.generate
        self.generate.argtypes = [
            ctypes.c_bool,  ctypes.c_bool, ctypes.c_float, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int]
        self.generate.restype = ctypes.c_int
        err_code = self.generate(use_default_vals, read_input, 1000.0, ALL_CHANNEL_GAIN,
                                 WAVEFORM_AMPLITUDE, WAVEFORM_FREQUENCY, WAVEFORM_DURATION)
        self._error_check(err_code)

    def _error_check(self, err_code):
        if err_code == ERR_BOARD_CONFIG:
            err_str = "ERR_BOARD_CONFIG"
        if err_code == ERR_CHANNEL_CONFIG:
            err_str = "ERR_CHANNEL_CONFIG"
        if err_code == ERR_DATA_CONFIG:
            err_str = "ERR_DATA_CONFIG"
        if err_code == ERR_MEASUREMENT:
            err_str = "ERR_MEASUREMENT"
        if err_code == ERR_DEINIT_CONFIG:
            err_str = "ERR_DEINIT_CONFIG"
        print(f"Error Occured: {err_code}_{err_str}")


if __name__ == "__main__":
    signalanalyzer = DT9837()

    signalanalyzer.measure_acceleration()
    # signalanalyzer.measure_acceleration(use_default_vals=False)
    # signalanalyzer.generate_squarewave(use_default_vals=False, read_input=True)
    # signalanalyzer.test()
