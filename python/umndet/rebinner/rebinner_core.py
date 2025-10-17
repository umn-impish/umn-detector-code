from umndet.common.impress_exact_structs import HafxHistogramArray, NominalHafx
import umndet.common.constants as umncon

import copy


def read_energy_edges():
    return umncon.NEW_ENERGY_EDGES


def read_times_combine():
    return umncon.NUM_TIMES_REBIN


def rebin_energies(slices: list[NominalHafx], edges: list[int]) -> list[NominalHafx]:
    # assumes edges are in order, unique, and < # max elements
    for sl in slices:
        spec = list(sl.histogram)
        new = list()
        for i in range(len(edges) - 1):
            new.append(sum(spec[edges[i] : edges[i + 1]]))
        sl.histogram = HafxHistogramArray(*new)
    return slices


def rebin_times(slices: list[NominalHafx], num_combine: int) -> list[NominalHafx]:
    """NOTE: assumes first entry is synced up with PPS!"""
    if (num_combine <= 0) or (num_combine % 32 != 0):
        # If this wasn't the case, then we would have to encode
        # fractional seconds into the timestamp.
        # But the timestamp only holds an integer number of seconds.
        # So we are stuck with this.
        # If you want fractional second time bins then downlink the raw data.
        raise ValueError(
            "argument num_combine must be a multiple of 32 (only rebin full seconds)"
        )

    if slices[0].time_anchor == 0:
        raise ValueError("first slice must have valid time stamp")

    ret = list()
    for i in range(0, len(slices), num_combine):
        end = i + num_combine
        # need to make a copy otherwise we modify the original objects (bad)
        ref, *subset = copy.deepcopy(slices[i:end])
        cumulative_histogram = ref.histogram
        for sl in subset:
            # sum all the data across time
            for i in range(len(cumulative_histogram)):
                cumulative_histogram[i] += sl.histogram[i]
            ref.num_evts += sl.num_evts
            ref.num_triggers += sl.num_triggers
            ref.dead_time += sl.dead_time
            ref.anode_current += sl.anode_current

            # if any miss a PPS, then we say they all did in this chunk
            ref.missed_pps |= sl.missed_pps

        if any(x > 2**32 - 1 for x in cumulative_histogram):
            raise ValueError("overflowed!")

        ref.histogram = HafxHistogramArray(*cumulative_histogram)
        ret.append(ref)
    return ret


def rebin_time_slices(
    slices: list[NominalHafx], energy_edges: list[int], times_combine: int
) -> list[NominalHafx]:
    ret = slices
    if energy_edges is not None:
        ret = rebin_energies(ret, energy_edges)
    if times_combine is not None:
        ret = rebin_times(ret, times_combine)
    return ret
