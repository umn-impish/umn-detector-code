import base64
import ctypes
import struct

# Convert deg Celsius to Kelvin
c_to_k = lambda t: t + 273.15

NUM_HG_BINS = 123
HafxHistogramArray = NUM_HG_BINS * ctypes.c_uint32
class NominalHafx(ctypes.Structure):
    # do not pad the struct
    _pack_ = 1
    _fields_ = [
        ('ch', ctypes.c_uint8),
        ('buffer_number', ctypes.c_uint16),
        ('num_evts', ctypes.c_uint32),
        ('num_triggers', ctypes.c_uint32),
        ('dead_time', ctypes.c_uint32),
        ('anode_current', ctypes.c_uint32),
        ('histogram', HafxHistogramArray),
        ('time_anchor', ctypes.c_uint32),
        ('missed_pps', ctypes.c_bool)
    ]

    def to_json(self):
        units = {
            'dead_time': 'nanosecond',
            'anode_current': 'nanoampere',
        }
        # Explicitly convert time units
        # into ones which are nicer to use.
        # The units are defined in the firmware
        # specification
        converters = {
            'dead_time': lambda x: 800 * x,
            'anode_current': lambda x: 25 * x,
            'ch': lambda x: ['c1', 'm1', 'm5', 'x1'][x],
            'histogram': lambda x: list(x),
            'missed_pps': lambda x: bool(x)
        }
        return {
            k: {
                'value': converters.get(k, lambda x: x)(getattr(self, k)),
                'unit': units.get(k, 'N/A')
            }
            for k, _ in self._fields_
        }


class HafxHealth(ctypes.Structure):
    # no struct padding
    _pack_ = 1
    _fields_ = [
        # 0.01K / tick
        ('arm_temp', ctypes.c_uint16),
        # 0.01K / tick
        ('sipm_temp', ctypes.c_uint16),
        # 0.01V / tick
        ('sipm_operating_voltage', ctypes.c_uint16),
        ('sipm_target_voltage', ctypes.c_uint16),
        ('counts', ctypes.c_uint32),

        # clock cycles = 25ns / tick for a 40MHz clock
        ('dead_time', ctypes.c_uint32),
        # clock cycles = 25ns / tick for a 40MHz clockh
        ('real_time', ctypes.c_uint32),
    ]

    def to_json(self):
        units = {
            'arm_temp': 'Kelvin',
            'sipm_temp': 'Kelvin',
            'sipm_operating_voltage': 'volt',
            'sipm_target_voltage': 'volt',
            'counts': 'count',
            'dead_time': 'nanosecond',
            'real_time': 'nanosecond'
        }
        converters = {
            'arm_temp': lambda x: 0.01 * x,
            'sipm_temp': lambda x: 0.01 * x,
            'sipm_operating_voltage': lambda x: 0.01 * x,
            'sipm_target_voltage': lambda x: 0.01 * x,
            'dead_time': lambda x: x * 25,
            'real_time': lambda x: x * 25,
        }
        return {
            k: {
                'value': converters.get(k, lambda x: x)(getattr(self, k)),
                'unit': units[k]
            }
            for k, _ in self._fields_
        }


class X123Health(ctypes.Structure):
    _fields_ = [
        # 1 degC / tick
        ('board_temp', ctypes.c_int8),
        # 0.5V / tick
        ('det_high_voltage', ctypes.c_int16),
        # 0.1K / tick
        ('det_temp', ctypes.c_uint16),
        ('fast_counts', ctypes.c_uint32),
        ('slow_counts', ctypes.c_uint32),

        # 1ms / tick
        ('accumulation_time', ctypes.c_uint32),
        ('real_time', ctypes.c_uint32),
    ]
    # no struct padding
    _pack_ = 1

    def to_json(self):
        units = {
            'board_temp': 'Kelvin',
            'det_high_voltage': 'volt',
            'det_temp': 'Kelvin',
            'fast_counts': 'count',
            'slow_counts': 'count',
            'accumulation_time': 'millisecond',
            'real_time': 'millisecond'
        }
        converters = {
            'board_temp': c_to_k,
            'det_high_voltage': lambda x: 0.5*x
        }
        return {
            k: {
                'value': converters.get(k, lambda x: x)(getattr(self, k)),
                'unit': units[k]
            }
            for k, _ in self._fields_
        }


class DetectorHealth(ctypes.Structure):
    _pack_ = 1
    _fields_ = [
        ('timestamp', ctypes.c_uint32),
        ('c1', HafxHealth),
        ('m1', HafxHealth),
        ('m5', HafxHealth),
        ('x1', HafxHealth),
        ('x123', X123Health)
    ]

    def to_json(self):
        return {'timestamp': self.timestamp} | {
            k: getattr(self, k).to_json() for (k, _) in self._fields_[1:]
        }


class X123NominalSpectrumStatus:
    def __init__(self, timestamp_seconds: int, count_histogram: list[int], status: bytes):
        self.timestamp = timestamp_seconds
        self.histogram = count_histogram
        # Encode to base64 for easy storage
        self.status_b64 = base64.b64encode(status).decode('utf-8')

    def to_json(self):
        return {
            'timestamp': self.timestamp,
            'histogram': list(self.histogram),
            'status_b64': self.status_b64,
        }


class X123Debug:
    def __init__(self, debug_type: int, debug_bytes: bytes):
        self.type = debug_type
        self.bytes = debug_bytes

    def decode(self) -> dict[str, object]:
        '''
        Decode the contained bytes into something more useful
        '''
        TYPE_MAP = [
            'histogram',
            'diagnostic',
            'ascii-settings'
        ]
        DECODE_MAP = [
            self._decode_histogram,
            self._decode_diagnostic,
            self._decode_ascii
        ]

        return {
            'type': TYPE_MAP[self.type],
            'data': DECODE_MAP[self.type]()
        }

    def _decode_histogram(self):
        # Last 64B are status data (spectrum + status packet)
        status_start = len(self.bytes) - 64
        data, status = self.bytes[:status_start], self.bytes[status_start:]

        # Each histogram entry is 3x uint32_t
        histogram = []
        for i in range(0, len(data), 3):
            histogram.append(
                data[i] |
                (data[i+1] << 8) |
                (data[i+2] << 16)
            )

        return {
            'status': base64.b64encode(status).decode('utf-8'),
            'histogram': histogram
        }

    def _decode_diagnostic(self):
        return base64.b64encode(self.bytes).decode('utf-8')

    def _decode_ascii(self):
        # the X-123 buffer comes out as padded with a bunch of zeros at the end
        first_null = self.bytes.index(0)
        # ASCII settings string is...ASCII already
        return self.bytes[:first_null].decode('utf-8')


class HafxDebug:
    # * Order matters here (we are decoding an enum)
    # * Data sizes are taken from MDS documentation
    #   https://www.bridgeportinstruments.com/products/software/wxMCA_doc/documentation/english/mds/mca3k/introduction.html
    TYPE_DECODE_MAP = [
        ('arm_ctrl', '<12f'),
        ('arm_cal', '<64f'),
        ('arm_status', '<7f'),
        ('fpga_ctrl', '<16H'),
        ('fpga_oscilloscope_trace', '<1024H'),
        ('fpga_statistics', '<16L'),
        ('fpga_weights', '<1024H'),
        ('histogram', '<4096L'),
        ('listmode', '<1024H'),
    ]

    def __init__(self, debug_type: int, debug_bytes: bytes):
        self.type = debug_type
        self.bytes = debug_bytes

    def decode(self) -> dict[str, object]:
        type_, unpack_str = HafxDebug.TYPE_DECODE_MAP[self.type]
        return {
            'type': type_,
            'registers': list(
                struct.unpack(unpack_str, self.bytes)
            )
        }


class StrippedNrlDataPoint(ctypes.LittleEndianStructure):
    _pack_ = 1
    _fields_ = (
        ('relative_timestamp', ctypes.c_uint32, 25),
        ('energy', ctypes.c_uint32, 4),
        ('was_pps', ctypes.c_uint32, 1),
        ('piled_up', ctypes.c_uint32, 1),
        ('out_of_range', ctypes.c_uint32, 1),
    )

    def to_json(self):
        return {field[0]: getattr(self, field[0]) for field in self._fields_}
