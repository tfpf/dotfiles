#! /usr/bin/env sh

# The external diff tool relies on the built-in hash function in Python being
# deterministic across interpreter sessions. (Hashes of files are calculated in
# difference processes.) Hence, disable hash randomisation.
export PYTHONHASHSEED=0
