# Python code for IMPRESS+EXACT+IMPISH

## INSTALL
### From `pypi` (normal use)
#### To install _all_ code, including ground scripts
`pip install umndet.[all]`
#### To install _just the structs_ used in flight
`pip install -e umndet`

### For development
First, `cd` into this directory. Then,
#### To install _all_ code, including ground scripts
`pip install -e .[all]`
#### To install _just the structs_ used in flight
`pip install -e .`

## USE
### Import what you want in Python, e.g.: `from umndet.common import impress_exact_structs as ies`

## Common code between flight/ground (`umn_detector/common`)
Code containing ways to parse binary data to Python (via `ctypes` and `struct`).

## Ground code
Code that runs on the ground once we get the data.
Used to decode packets into JSON for parsing later on.

## Tools
Useful tools--simulate data and other things.

## Rebinner (used in-flight)
Rebins the science data from IMPRESS into smaller files.

Scripts are defined in `pyproject.toml`. Example:
### Rebin data
```bash
impress-rebinner time+energy data/file-ident-*
```
Will rebin the files given along time and energy axes.

