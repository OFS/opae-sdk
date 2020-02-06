#!/usr/bin/env python3
import argparse
from array import array
import struct
import tempfile


def reverse_bits(x, n):
    result = 0
    for i in xrange(n):
        if (x >> i) & 1:
            result |= 1 << (n - 1 - i)
    return result


def reverse_bits_in_file(ifile, ofile):
    bit_rev = array('B')
    for i in range(0, 256):
        bit_rev.append(reverse_bits(i, 8))

    bytes_written = 0
    while True:
        ichunk = ifile.read(4096)
        if not ichunk:
            break

        if isinstance(ichunk, str):
            ochunk = ''
            for b in ichunk:
                ochunk += chr(bit_rev[ord(b)])
            bytes_written += ofile.write(ochunk)
        elif isinstance(ichunk, bytes):
            ochunk = []
            for b in ichunk:
                ochunk.append(bit_rev[b])
            bytes_written += ofile.write(bytes(ochunk))

    return bytes_written


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('ifile', type=argparse.FileType('rb'),
                        help='input file to be reversed')
    parser.add_argument('ofile', type=argparse.FileType('wb'),
                        help='output file')

    shlp = 'seek number of bytes into input file before reading'
    parser.add_argument('-s', '--seek', type=lambda x: int(x, 0), help=shlp)

    phlp = 'pad to multiple of bytes'
    parser.add_argument('-p', '--pad', type=lambda x: int(x, 0), help=phlp)

    pvhlp = 'value of byte used to pad [default to 0xff]'
    parser.add_argument('-pv', '--pad-value', type=lambda x: int(x, 0),
                        help=pvhlp, default=0xff)

    endhlp = 'swap 32 bin endian before swapping bytes [Max10 RPDs]'
    parser.add_argument('-e', '--endian', action='store_true', help=endhlp)

    args = parser.parse_args()

    if args.seek:
        args.ifile.seek(args.seek)

    if args.endian:
        sfile = tempfile.NamedTemporaryFile(mode='wb+', delete=False)
        a = array("I", args.ifile.read())
        for elem in a:
            sfile.write(struct.pack('>I', elem))
        sfile.close()
        args.ifile = open(sfile.name, 'rb')

    bytes_written = reverse_bits_in_file(args.ifile, args.ofile)

    if args.pad:
        abyte = struct.pack("=B", args.pad_value)
        while bytes_written % args.pad != 0:
            args.ofile.write(abyte)
            bytes_written += 1


if __name__ == '__main__':
    main()
