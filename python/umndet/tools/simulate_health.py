import argparse
import gzip
import time

import umndet.common.impress_exact_structs as ies
import numpy as np

rng = np.random.default_rng()


def main():
    p = argparse.ArgumentParser(
        description="simulate IMPRESS health data (time_slices)"
    )
    p.add_argument(
        "num_packets",
        help="number of health packets to simulate",
        type=int,
        default=1000,
    )
    p.add_argument(
        "output_filename",
        help="filename to put the output data in; .bin.gz gets appended",
        default="health-example.bin.gz",
    )
    args = p.parse_args()

    print("simulating", args.num_packets, "health packets")
    ts = int(time.time())
    with gzip.open(args.output_filename + ".bin.gz", "wb") as f:
        for _ in range(args.num_packets):
            hd = simulate_health(ts)
            f.write(hd)
            ts += 1
    print("done")


def simulate_health(time_stamp: int) -> ies.DetectorHealth:
    ret = ies.DetectorHealth()

    ret.c1 = simulate_hafx_health()
    ret.m1 = simulate_hafx_health()
    ret.m5 = simulate_hafx_health()
    ret.x1 = simulate_hafx_health()
    ret.x123 = simulate_x123_health()

    ret.timestamp = time_stamp

    return ret


def simulate_x123_health() -> ies.X123Health:
    """
    Put realistic but very random values into
    simulated X-123 detector health packet.
    """
    ret = ies.X123Health()

    ret.board_temp = int(rng.uniform(low=-50, high=50))
    ret.det_high_voltage = int(rng.uniform(low=-800, high=-200))
    ret.det_temp = int(rng.uniform(1500, 2500))
    ret.fast_counts = (fc := int(rng.uniform(1e4, 1e6)))
    ret.slow_counts = int(rng.uniform(0.2 * fc, fc))

    ret.accumulation_time = (at := int(rng.uniform(0, 2e6)))
    ret.real_time = int(rng.uniform(at, at * 2))
    return ret


def simulate_hafx_health() -> ies.HafxHealth:
    """
    Put realistic but very random values into
    simulated HaFX detector health packet.
    """
    ret = ies.HafxHealth()
    ret.arm_temp = int(rng.uniform(26000, 31000))
    ret.sipm_temp = int(rng.uniform(24000, 33000))
    ret.sipm_operating_voltage = int(rng.uniform(2800, 3900))
    ret.counts = int(rng.uniform(10, 1e5))
    ret.dead_time = (dt := int(rng.uniform(1000, 1e9)))
    ret.real_time = int(rng.uniform(dt, dt * 1000))
    return ret


if __name__ == "__main__":
    main()
