import datetime
import numpy as np
import matplotlib.pyplot as plt

from umndet.common import impress_exact_structs as ies
from umndet.common import helpers
from umndet.common.constants import BRIDGEPORT_EDGES


def plot_raw_time_slice_spectrogram(
    data: list[ies.NominalHafx], fig=None, ax=None, adc_bins=BRIDGEPORT_EDGES
):
    counts_spectrogram = np.array([hd.histogram for hd in data])

    # Construct the timestamps from the given data points
    def from_timestamp(ts):
        return datetime.datetime.fromtimestamp(ts, tz=datetime.UTC)

    recent = from_timestamp(data[0].time_anchor)
    times = [recent]
    idx = 1
    for hd in data[1:]:
        if hd.time_anchor != 0:
            recent = from_timestamp(hd.time_anchor)
        times.append(recent + datetime.timedelta(seconds=((idx % 32) / 32)))
        idx += 1

    # "time bins" are 1 larger than the # of histograms we get
    times.append(times[-1] + datetime.timedelta(seconds=1 / 32))

    fig = fig or plt.gcf()
    ax = ax or plt.gca()

    # Convert bin map we send to the Bridgeport
    # into equivalent normal Bridgeport bins (4096 of em)
    reversed_bins = helpers.reverse_bridgeport_mapping(adc_bins)

    pcm = ax.pcolormesh(times, reversed_bins, counts_spectrogram.T, cmap="plasma")
    ax.set(
        xlabel="Time (UTC)",
        ylabel="Normal Bridgeport ADC bin",
        title="Counts spectrogram",
    )

    return fig, ax, pcm
