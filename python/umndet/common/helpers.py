import ctypes
import struct
import umndet.common.impress_exact_structs as ies
from typing import Any, Callable, IO, Iterable


def generic_read_binary(
    fn: str,
    open_func: Callable,
    function_body: Callable[[IO[bytes]], Any]
) -> list[Any]: 
    ret = []
    with open_func(fn, 'rb') as f:
        while True:
            try:
                new_data = function_body(f)
            except struct.error:
                break
            if not new_data: break
            ret.append(new_data)
    return ret


def read_binary(fn: str, type_: type, open_func: Callable) -> list:
    sz = ctypes.sizeof(type_)
    def read_elt(f: IO[bytes]):
        d = type_()
        eof = (f.readinto(d) != sz)
        if eof: return None
        return d

    return generic_read_binary(fn, open_func, read_elt)


def read_det_health(fn: str, open_func: Callable) -> list[ies.DetectorHealth]:
    return read_binary(fn, ies.DetectorHealth, open_func)


def read_hafx_sci(fn: str, open_func: Callable) -> list[ies.NominalHafx]:
    return read_binary(fn, ies.NominalHafx, open_func)


def read_x123_sci(fn: str, open_func: Callable) -> list[ies.X123NominalSpectrumStatus]:
    def read_elt(f: IO[bytes]):
        timestamp, = struct.unpack('<L', f.read(4))
        status_bytes = f.read(64)
        spectrum_size, = struct.unpack('<H', f.read(2))
        spectrum = list(struct.unpack('<' + ('L' * spectrum_size), f.read(4 * spectrum_size)))
        return ies.X123NominalSpectrumStatus(
            timestamp, spectrum, status_bytes
        )
    return generic_read_binary(fn, open_func, read_elt)


def read_x123_debug(fn: str, open_func: Callable) -> list[ies.X123Debug]:
    def read_elt(f: IO[bytes]):
        debug_type, = struct.unpack('<B', f.read(1))
        size, = struct.unpack('<L', f.read(4))
        data = f.read(size)
        return ies.X123Debug(
            debug_type,
            data
        )
    return generic_read_binary(fn, open_func, read_elt)


def read_hafx_debug(fn: str, open_func: Callable) -> list[ies.HafxDebug]:
    def read_elt(f: IO[bytes]):
        type_, = struct.unpack('<B', f.read(1))
        sz = struct.calcsize(ies.HafxDebug.TYPE_DECODE_MAP[type_][1])
        bytes_ = f.read(sz)
        return ies.HafxDebug(type_, bytes_)
    return generic_read_binary(fn, open_func, read_elt)


def reverse_bridgeport_mapping(adc_mapping: Iterable[int]) -> list[int]:
    '''
    Take the list of 2048 numbers which map 2048 "normal" ADC
    bins down to the IMPRESS 123 bins and undo that mapping.
    '''
    reversed_bins = list(
        2 * adc_mapping.index(i)
        for i in range(5, 128)
    )
    reversed_bins.append(4097)
    return reversed_bins