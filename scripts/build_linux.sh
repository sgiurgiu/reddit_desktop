#!/bin/bash

set -e

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


if [ -z $1 ]; then
    distros=("debian" "fedora" "ubuntu")
else
    distros=($1)
fi

for distro in "${distros[@]}"; do
    echo "Running podman to build for distribution ${distro}"
    podman pull registry.zergiu.com:5000/reddit_desktop_$distro:build
    podman run --rm --privileged=true --name rd \
            -v "${root}":/tmp/reddit_desktop/:Z \
            -e REDDITDESKTOP_VERSION_MAJOR="${REDDITDESKTOP_VERSION_MAJOR}" \
            -e REDDITDESKTOP_VERSION_MINOR="${REDDITDESKTOP_VERSION_MINOR}" \
            -e REDDITDESKTOP_VERSION_PATCH="${REDDITDESKTOP_VERSION_PATCH}" \
            registry.zergiu.com:5000/reddit_desktop_$distro:build
done

podman rmi -f reddit_desktop_runtime:latest || true
podman rmi -f registry.fedoraproject.org/fedora-minimal:latest || true

buildah bud --build-arg RDRPM=/tmp/reddit_desktop/packages/reddit_desktop-${REDDITDESKTOP_VERSION_MAJOR}.${REDDITDESKTOP_VERSION_MINOR}.${REDDITDESKTOP_VERSION_PATCH}-fedora.rpm \
            -v "${root}":/tmp/reddit_desktop/:Z \
            -f  ${root}/docker/Dockerfile.fedora.runtime -t reddit_desktop_runtime
podman save reddit_desktop_runtime:latest | gzip -9 -n > ${root}/packages/reddit_desktop_runtime.tar.gz

