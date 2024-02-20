# This program imports and runs the compiled data acuqisition C library for DT9837
from ctypes import *

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
ERR_CFG_FAILURE = -1
ERR_INIT_CONFIG = 1
ERR_BOARD_CONFIG = 2
ERR_CHANNEL_CONFIG = 3
ERR_DATA_CONFIG = 4
ERR_MEASUREMENT = 5
ERR_OUTPUT = 6
ERR_DEINIT_CONFIG = 7

# [TODO]:


class ChannelData(Structure):
    _fields_ = [
        ("channel", (POINTER(c_double)) * NUM_CHANNELS),
        ("time_ms", POINTER(c_double)),
        ("num_readings", c_int),
        ("max_readings", c_int)
    ]


class DT9837():
    def __init__(self):
        """Equipment class for DT9837 signal analyzer
        """
        self.dt_lib = CDLL(
            "C:\_Automation\Win32\SDK\Examples\DtConsole\dt_lib.so")

    def connect(self):
        """Connect to the device."""
        err_str = ""
        self.init = self.dt_lib.initialize_board
        err_code = self.init()
        if err_code != 0:
            err_str = "ERROR_INIT_CONFIG_FAILURE"
            print(f"Error Occured: {err_code}_{err_str}")

    def disconnect(self):
        """Disconnect the device"""
        err_str = ""
        self.deinit = self.dt_lib.deinit_board
        err_code = self.deinit()
        if err_code != 0:
            err_str = "ERROR_DEINIT_CONFIG_FAILURE"
            print(f"Error Occured: {err_code}_{err_str}")

    def measure_acceleration(self, duration, timer_enabled=True, use_default_vals=True):
        """This function measures the acceleration reading

        A 3 axis accelerometer must be connected to the equipment that 
        will be recorded indefinately until keyboard interrupt

        :param use_default_vals: use the set default values of the equipment, defaults to True
        :type use_default_vals: bool, optional
        """
        self.measure = self.dt_lib.measure
        self.get_data = self.dt_lib.get_channel_data
        self.cleanup_data = self.dt_lib.cleanup_data
        self.measure.argtypes = [c_bool, c_int, c_float,
                                 c_int, c_int, c_int, c_int, c_int, c_bool, c_int]

        self.measure.restype = c_int
        self.get_data.restype = ChannelData

        err_code = self.measure(use_default_vals, NUM_CHANNELS,
                                CLOCK_FREQUENCY, ALL_CHANNEL_GAIN, CHANNEL_GAIN_0, CHANNEL_GAIN_1, CHANNEL_GAIN_2, CHANNEL_GAIN_3, timer_enabled, duration)
        self._error_check(err_code)

        data_struct = self.get_data()
        channels = [[round(data_struct.channel[i][j], 3)
                     for j in range(data_struct.num_readings)] for i in range(4)]
        time_ms = [round(data_struct.time_ms[i], 3)
                   for i in range(data_struct.num_readings)]

        # Print the data
        print("Number of readings:", data_struct.num_readings)
        print("Max number of readings:", data_struct.max_readings)
        print("Time(ms),accel_x(g),accel_y(g),accel_z(g)\n")
        print(
            f"{time_ms[0]},{channels[0][0]},{channels[1][0]},{channels[2][0]},{channels[3][0]}\n")
        print(
            f"{time_ms[1]},{channels[0][1]},{channels[1][1]},{channels[2][1]},{channels[3][1]}")
        print(
            f"{time_ms[2]},{channels[0][2]},{channels[1][2]},{channels[2][2]},{channels[3][2]}")
        print(
            f"{time_ms[3]},{channels[0][3]},{channels[1][3]},{channels[2][3]},{channels[3][3]}")
        print(
            f"{time_ms[4]},{channels[0][4]},{channels[1][4]},{channels[2][4]},{channels[3][4]}")

        self.cleanup_data()

    def generate_squarewave(self, duration, timer_enabled=True, use_default_vals=True, read_input=True):
        """This function generates a simple squarewave

        Generates a wave at the specified amplitude (V), frequency (Hz) and duration (in s)

        :param use_default_vals: use the set default values of the equipment, defaults to True
        :type use_default_vals: bool, optional
        :param read_input: record the input into a csv, defaults to True
        :type read_input: bool, optional
        """
        self.generate = self.dt_lib.generate
        self.generate.argtypes = [
            c_bool,  c_bool, c_float, c_int, c_int, c_int, c_bool, c_int]
        self.generate.restype = c_int
        err_code = self.generate(use_default_vals, read_input, CLOCK_FREQUENCY, ALL_CHANNEL_GAIN,
                                 WAVEFORM_AMPLITUDE, WAVEFORM_FREQUENCY, timer_enabled, duration)
        self._error_check(err_code)

    def _error_check(self, err_code):
        err_str = ""
        if err_code != 0:
            if err_code == ERR_CFG_FAILURE:
                err_str = "ERROR_MISC_FAILURE"
            if err_code == ERR_INIT_CONFIG:
                err_str = "ERROR_INIT_CONFIG_FAILURE"
            if err_code == ERR_BOARD_CONFIG:
                err_str = "ERROR_BOARD_CONFIG_FAILURE"
            if err_code == ERR_CHANNEL_CONFIG:
                err_str = "ERROR_CHANNEL_CONFIG_FAILURE"
            if err_code == ERR_DATA_CONFIG:
                err_str = "ERROR_DATA_CONFIG_FAILURE"
            if err_code == ERR_MEASUREMENT:
                err_str = "ERROR_MEASUREMENT_FAILURE"
            if err_code == ERR_OUTPUT:
                err_str = "ERROR_OUTPUT_FAILURE"
            if err_code == ERR_DEINIT_CONFIG:
                err_str = "ERROR_DEINIT_CONFIG_FAILURE"
            print(f"Error Occured: {err_code}_{err_str}")


if __name__ == "__main__":
    signalanalyzer = DT9837()

    signalanalyzer.connect()
    signalanalyzer.measure_acceleration(1, timer_enabled=True)
    # signalanalyzer.measure_acceleration(10, timer_enabled=False)
    # signalanalyzer.generate_squarewave(
    #     WAVEFORM_DURATION, use_default_vals=False, read_input=False)
    # signalanalyzer.generate_squarewave(
    #     0, timer_enabled=False, use_default_vals=False)
    signalanalyzer.disconnect()
