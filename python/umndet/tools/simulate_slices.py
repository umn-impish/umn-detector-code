import argparse
import datetime as dt
import gzip
import os
import random
import time

import umndet.common.impress_exact_structs as ies
from umndet.common.constants import DATE_FMT


def main():
    p = argparse.ArgumentParser(
        description="simulate IMPRESS science data (time_slices)"
    )
    p.add_argument("data_dir", default="test-data", help="directory to save data to")
    p.add_argument(
        "num_files", type=int, default=30, help="number of data files to generate"
    )
    p.add_argument(
        "seconds_per_file",
        type=int,
        default=30,
        help="number of seconds per time_slice file",
    )
    args = p.parse_args()

    os.makedirs(args.data_dir, exist_ok=True)

    ts = int(time.time())
    for slice_num in range(args.num_files):
        time_str = dt.datetime.fromtimestamp(ts, dt.UTC).strftime(DATE_FMT)
        output_file = f"{args.data_dir}/sim-hafx-c1-hist_{time_str}_0.bin.gz"
        with gzip.open(output_file, "wb") as f:
            for sec in range(args.seconds_per_file):
                for i in range(32):
                    f.write(simulate_single_slice(i, ts if (i % 32 == 0) else 0))
                ts += 1


def simulate_single_slice(frame_num: int, time_anchor: int = 0) -> ies.NominalHafx:
    ret = ies.NominalHafx()

    # reasonable amt during a solar flare in 32ms
    ret.num_evts = (evts := random.randint(2500, 4000))

    # max ~90% more triggers (pileup)
    ret.num_triggers = (triggers := int(evts * (1 + random.uniform(0, 0.9))))

    # ~1.5us dead time
    ticks_per_evt = 4
    ret.dead_time = int(ticks_per_evt * triggers)

    # no clue what this should be
    ret.anode_current = int(random.uniform(1000, 2000))

    # for now make the science data random
    ret.histogram = ies.HafxHistogramArray(
        *[random.randint(0, 2**16 - 1) for _ in range(ies.NUM_HG_BINS)]
    )

    # might wanna change in the future (?)
    ret.missed_pps = False
    ret.buffer_number = frame_num
    ret.time_anchor = time_anchor

    return ret


if __name__ == "__main__":
    main()
