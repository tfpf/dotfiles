#! /usr/bin/env python

import os
import sys

print(sys.argv)
print([*os.walk(sys.argv[1])])
print([*os.walk(sys.argv[2])])
