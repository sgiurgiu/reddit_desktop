#!/usr/local/bin/bash

set -ex

if [ -z ${CI_PROJECT_DIR+x} ]; then
    root=$(git rev-parse --show-toplevel)
else
    root=${CI_PROJECT_DIR}
fi

if [ -z ${REDDITDESKTOP_VERSION_MAJOR+x} ]; then
    REDDITDESKTOP_VERSION_MAJOR="1"
    REDDITDESKTOP_VERSION_MINOR="0"
    REDDITDESKTOP_VERSION_PATCH="dev"
fi


if [ -z $CONTAINER_REGISTRY ]; then
    echo "FATAL: Please set the CONTAINER_REGISTRY environment variable to point to where the containers are located."
    exit 1
fi



CMAKE_ARGS="-GNinja -DPACKAGING=ON -DCMAKE_BUILD_TYPE=Release \
            -DENABLE_TESTS=OFF -DENABLE_M4DC=ON -DENABLE_CMARK=OFF"
distro=freebsd
CMAKE_ARGS="${CMAKE_ARGS} -DCPACK_DISTRIBUTION=freebsd -DCPACK_GENERATOR=FREEBSD"

rm -rf ${root}/build

cmake -B${root}/build -S${root} ${CMAKE_ARGS}

cmake --build ${root}/build -- package

rm -rf ${root}/packages
mkdir -p ${root}/packages
mv ${root}/build/reddit_desktop-*-${distro}.* ${root}/packages

