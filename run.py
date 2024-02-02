# This program imports and runs the compiled data acuqisition C library for DT9837
import ctypes

dt_lib = ctypes.CDLL("C:\_Automation\Win32\SDK\Examples\DtConsole\dt_lib.so")
dt_lib.main()