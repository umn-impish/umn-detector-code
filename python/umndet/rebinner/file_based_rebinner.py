import argparse
import gzip
import os

from umndet.common import helpers as hp
from umndet.common.impress_exact_structs import NominalHafx
from umndet.rebinner import rebinner_core as rebc

"""
A stripped-down rebinner that just accepts a list of files and
rebins them.
"""


def main():
    args = parse_args()
    for fn in args.files:
        _ = rebin_file(fn, args.compression_type)


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(
        description="rebin & flag HaFX + X123 science data in-flight (given a list of files)"
    )
    p.add_argument("compression_type", choices=["time", "energy", "time+energy"])
    p.add_argument(
        "files",
        nargs="+",
    )

    return p.parse_args()


def rebin_file(fn: str, compress: str) -> str:
    if compress == "none":
        return fn

    direc, just_fn = os.path.split(fn)
    rebinned_fn = os.path.join(direc, f"{compress}-{just_fn}")

    slices = hp.read_hafx_sci(fn)
    rebinned_slices = rebc.rebin_time_slices(
        slices,
        (rebc.read_energy_edges() if "energy" in compress else None),
        (rebc.read_times_combine() if "time" in compress else None),
    )
    write_rebinned(rebinned_fn, rebinned_slices)

    return rebinned_fn


def write_rebinned(fn: str, slices: list[NominalHafx]):
    with gzip.open(fn, "wb") as f:
        for sl in slices:
            f.write(sl)


if __name__ == "__main__":
    main()
