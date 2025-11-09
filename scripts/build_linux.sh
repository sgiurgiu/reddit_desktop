#!/bin/bash

set -ex

if [ -z ${CI_PROJECT_DIR+x} ]; then
    root=$(git rev-parse --show-toplevel)
else
    root=${CI_PROJECT_DIR}
fi

if [ -z ${REDDITDESKTOP_VERSION+x} ]; then
    REDDITDESKTOP_VERSION="1.0.dev"
fi


if [ -z $1 ]; then
    distros=("debian" "fedora" "ubuntu")
else
    distros=($1)
fi

if [ -z $CONTAINER_REGISTRY ]; then
    echo "FATAL: Please set the CONTAINER_REGISTRY environment variable to point to where the containers are located."
    exit 1
fi

for distro in "${distros[@]}"; do
    echo "Running podman to build for distribution ${distro}"
    container=$CONTAINER_REGISTRY/vcpkg_mpv_apps_$distro:build
    podman pull $container
    podman run --rm --privileged=true --name rd \
            -v "${root}":/tmp/reddit_desktop/:Z \
            -e REDDITDESKTOP_VERSION="${REDDITDESKTOP_VERSION}" \
            --entrypoint "/tmp/reddit_desktop/docker/build_reddit_desktop.sh" \
            $container "${distro}"
done


podman rmi -f reddit_desktop_runtime:latest || true

buildah bud --build-arg RDRPM=/tmp/reddit_desktop/packages/reddit_desktop-${REDDITDESKTOP_VERSION}-fedora.rpm \
            -v "${root}":/tmp/reddit_desktop/:Z \
            -f  ${root}/docker/Dockerfile.fedora.runtime -t reddit_desktop_runtime
podman save reddit_desktop_runtime:latest | gzip -9 -n > ${root}/packages/reddit_desktop_runtime.tar.gz

