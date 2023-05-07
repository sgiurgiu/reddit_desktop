#!/bin/bash

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


mkdir /tmp/build
cd /tmp/build

CMAKE_ARGS="-GNinja \
            -DFONTS_DIRECTORY=/usr/share/reddit_desktop -DCMAKE_BUILD_TYPE=Release \
            -DENABLE_TESTS=OFF -DENABLE_M4DC=ON -DENABLE_CMARK=OFF"
distro=freebsd
CMAKE_ARGS="${CMAKE_ARGS} -DCPACK_DISTRIBUTION=freebsd"
CMAKE_ARGS="${CMAKE_ARGS} -DCPACK_GENERATOR=FREEBSD"

cmake ${CMAKE_ARGS} /tmp/reddit_desktop

sudo mkdir -p /usr/share/reddit_desktop/fonts
sudo ninja package

mkdir -p /tmp/reddit_desktop/packages
cp reddit_desktop-*-${distro}.* /tmp/reddit_desktop/packages/

