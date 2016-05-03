#!/usr/bin/env python

import sys
import os

class Differ(object):
    def main(self):
        fn1 = sys.argv[1]
        fn2 = sys.argv[2]

        with open(fn1, "r") as f1:
            with open(fn2, "r") as f2:
                while True:
                    line1 = f1.readline()
                    line2 = f2.readline()
                    if not line1 and not line2:
                        return 0
                    elif not line1:
                        return 1
                    elif not line2:
                        return 2
                    else:
                        if line1.rstrip().upper() != line2.rstrip().upper():
                            return 3

if __name__ == "__main__":
    sys.exit(Differ().main())
