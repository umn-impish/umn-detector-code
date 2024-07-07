'''
Imports the current bin mapping given in the
`umndet.common.constants` file and generates a
.txt file which can be read and sent to the detector controller code
to command the Bridgeports to use a new bin mapping.

Takes no arguments. Writes to `bin-map.txt`.
'''

from umndet.common.constants import BRIDGEPORT_EDGES as adc_edges


def main():
    with open('bin-map.txt', 'w') as f:
        print(' '.join(str(x) for x in adc_edges), file=f)


if __name__ == '__main__':
    main()
