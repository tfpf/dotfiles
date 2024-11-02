#! /usr/bin/env bash

echo "------------------------------" $CMAKE_INSTALL_PREFIX
(
    git clone https://github.com/libgit2/libgit2.git
    mkdir libgit2/build
    cd libgit2/build
    cmake .. -DBUILD_CLI=OFF -DBUILD_SHARED_LIBS=OFF -DBUILD_TESTS=OFF
    cmake --build . --target install --parallel 4
)

echo "------------------------------" $PKG_CONFIG_PATH
CPPFLAGS="$(pkg-config --cflags --static libgit2)"
LDFLAGS="-static"
LDLIBS="-lstdc++ $(pkg-config --libs --static libgit2)"
(
    cd custom-prompt
    make LDFLAGS="$LDFLAGS" LDLIBS="$LDLIBS" -j release
)
