# Set up Python for working with IMPRESS/EXACT/IMPISH data

The Python tools in this repository can decode binary data from the flight computer into JSON,
    and also allow you to interact with the data without decoding it into JSON,
    if so desired.

This document describes how to properly set up a Python environment with all of the required
    packages and tools, as well as guarantee that the correct Python version is used.

### Prerequesites
First of all: you need to know how to use basic command line functions: `ls`, `mv`, `cp`, `mkdir`, etc.

To get the latest code, you need `git` installed.
On Windows,
    you can install `git` with the [official installer](https://git-scm.com/downloads/win),
    or use a package manager like [chocolatey](https://chocolatey.org/).
On Mac you can install `git` via [Homebrew](https://brew.sh/).
On Debian linux,
    you can use `apt`.
On other tyeps of Linux, well... you can figure it out.

## Step 1: install a version manager
Several python version managers exist.
We will only describe one way to set up Python environments: using `uv` from Astral.
Installation instructions for Windows, Mac, or Linux
    can be found [here](https://docs.astral.sh/uv/getting-started/installation/).

**If you have questions about installation, environment configuration, etc.,
    we will not help answer them if you do not use the `uv` tool.**

## Step 2: set up a Python environment
For the rest of the guide I'll assume that you are working in
    Linux or Mac,
    but most commands should translate to Windows without too
    much trouble.

Make a directory you want to work in.
For example,
```bash
cd ~
mkdir smallsat-analysis
```

Now, set up a python virtual environment with `uv`
```bash
cd smallsat-analysis

# You only need to create the virtual environment once.
uv venv

# Each time you want to do analysis, 
# you need to source this "activate" script.
# In Windows the command is slightly different, but `uv` should tell
# you how to run that command when you create the virtual environment.
source .venv/bin/activate
```

Now,
    clone the `umn-detector-code` repository,
    and install its Python package.
```bash
git clone https://github.com/umn-impish/umn-detector-code.git
cd umn-detector-code/python
uv pip install -e .[all]
```

Finally,
    install some extra packages so that you can
    use Jupyter notebooks and other things
```bash
uv pip install jupyter ipynb pyqt6 scipy astropy
```

## Analysis after initial setup
You need to activate your Python virtual environment when
you want to do smallsat analysis.
```bash
# For simplicity say we are working in the folder from before
cd ~/smallsat-analysis
source .venv/bin/activate

# Now, you can open up Jupyter notebooks and Python scripts
# in vs code if you want, or you can just run the jupyter
# server directly
jupyter notebook
```

### Scripts to decode data into JSON
There are a few scripts which are installed from
    `umn-detector-code/python`.
You can see their names at the bottom of
    `umn-detector-code/python/pyproject.toml`.
The scripts call code functions within the Python package.

To decode, for example, health data:
```bash
decode-impress-health health_file1.bin.gz health_file2.bin.gz impress_health.json
```

This will decode the health data into a JSON file,
    which you can read into Jupyter.

Similar scripts exist for other data types.
The script names are in `pyproject.toml`,
    at the bottom.

### Working with `.bin.gz` files directly
You can also open the `.bin.gz` files in Python directly.
Here is an example snippet of doing just that.
```py
from umndet.common import helpers
import gzip
import pprint

# Example file name of health packets from the flight computer
fn = 'health.bin.gz'
# The file is gzipped,
# so we need to specify to open with the gzip.open function
health_packets = helpers.read_det_health(fn, open_func=gzip.open)

# Directly access variables you want
print(health_packets[0].c1.arm_temp)
# Output:
# 30370

# Or, turn it into JSON on the fly
pprint.pprint(health_packets[0].c1.to_json())
# Output:
# {'arm_temp': {'unit': 'Kelvin', 'value': 303.7},
#  'counts': {'unit': 'count', 'value': 1469204445},
#  'dead_time': {'unit': 'nanosecond', 'value': 42593800},
#  'real_time': {'unit': 'nanosecond', 'value': 2901237375},
#  'sipm_operating_voltage': {'unit': 'volt', 'value': 35.37},
#  'sipm_target_voltage': {'unit': 'volt', 'value': 35.38},
#  'sipm_temp': {'unit': 'Kelvin', 'value': 295.18}}
```