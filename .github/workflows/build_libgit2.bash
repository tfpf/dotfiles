#! /usr/bin/env bash

git clone https://github.com/libgit2/libgit2.git
mkdir libgit2/build
cd libgit2/build
cmake .. -DBUILD_CLI=OFF -DBUILD_SHARED_LIBS=OFF -DBUILD_TESTS=OFF
cmake --build . --target install --parallel 4
