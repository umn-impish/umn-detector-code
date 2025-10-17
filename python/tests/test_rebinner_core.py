import numpy as np
import pytest
import random
import time

from umndet.rebinner import rebinner_core as rebc
from umndet.tools import simulate_slices as ssl
from umndet.common.impress_exact_structs import NominalHafx


def simulate_chunk(num: int = 1024) -> list[NominalHafx]:
    """
    Simulate some time slices for testing with random data.
    """
    ts = int(time.time())
    ret = list()
    for i in range(num):
        frame = i % 32
        given_time = 0
        if frame == 0:
            given_time = ts
            ts += 1
        ret.append(ssl.simulate_single_slice(frame, given_time))
    return ret


def test_time_rebin():
    """
    Tests if rebinner_core.rebin_times function properly works.
    """
    # Argument validation tests
    with pytest.raises(ValueError):
        rebc.rebin_times([], 31)
    with pytest.raises(ValueError):
        rebc.rebin_times([], 0)
    with pytest.raises(ValueError):
        rebc.rebin_times([], -32)
    with pytest.raises(ValueError):
        test_slices = simulate_chunk(num=1)
        test_slices[0].time_anchor = 0
        rebc.rebin_times(test_slices, 32)

    # Test if actual data got rebinned right
    data = simulate_chunk(num=2048)
    # memoize . . . speed up testing
    memo = dict()

    # these should remain unchanged after rebinning as long as we sum everything
    SAME_TOTAL_ATTRS = (
        "num_evts",
        "num_triggers",
        "dead_time",
        "anode_current",
    )

    num_sum_iter = (32 * x for x in range(1, 10 + 1))
    for num_sum in num_sum_iter:
        rebinned = rebc.rebin_times(data, num_sum)
        for k in SAME_TOTAL_ATTRS:
            orig = memo.get(k, np.array([getattr(d, k) for d in data]))
            if k not in memo:
                memo[k] = orig

            reb = np.array([getattr(r, k) for r in rebinned])
            assert orig.sum() == reb.sum()


def test_energy_rebin():
    """
    Tests if rebinner_core.rebin_energies function properly works.
    """
    data = simulate_chunk(num=2048)
    adc_edges = list(range(125))
    max_edges = 124
    attempts = 10

    # these should remain unchanged after rebinning
    SAME_ATTRS = (
        "ch",
        "buffer_number",
        "num_evts",
        "num_triggers",
        "dead_time",
        "anode_current",
        "time_anchor",
        "missed_pps",
    )

    for _ in range(attempts):
        num_edges = random.randint(0, max_edges)
        rebin_edges = random.sample(adc_edges, num_edges)
        rebin_edges.sort()
        rebinned = rebc.rebin_energies(data, rebin_edges)

        for reb, orig in zip(rebinned, data):
            # Counts should not change after rebinning
            assert sum(reb.histogram) == sum(orig.histogram)
            for a in SAME_ATTRS:
                assert getattr(reb, a) == getattr(orig, a)
