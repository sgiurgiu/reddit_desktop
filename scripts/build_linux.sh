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

if [ -z $CONTAINER_REGISTRY ]; then
    echo "FATAL: Please set the CONTAINER_REGISTRY environment variable to point to where the containers are located."
    exit 1
fi

function build()
{
    local distro=$1
    echo "Running podman to build for distribution ${distro}"
    podman pull $CONTAINER_REGISTRY/reddit_desktop_$distro:build
    podman run --rm --privileged=true --name rd \
            -v "${root}":/tmp/reddit_desktop/:Z \
            -e REDDITDESKTOP_VERSION_MAJOR="${REDDITDESKTOP_VERSION_MAJOR}" \
            -e REDDITDESKTOP_VERSION_MINOR="${REDDITDESKTOP_VERSION_MINOR}" \
            -e REDDITDESKTOP_VERSION_PATCH="${REDDITDESKTOP_VERSION_PATCH}" \
            $CONTAINER_REGISTRY/reddit_desktop_$distro:build
}
procs=()  # bash array
for distro in "${distros[@]}"; do
    procs+=("build $distro")
done
num_procs=${#procs[@]}  # number of processes
pids=()  # bash array
for (( i=0; i<"$num_procs"; i++ )); do
    echo "cmd = ${procs[$i]}"
    ${procs[$i]} &  # run the cmd as a subprocess
    # store pid of last subprocess started; see:
    # https://unix.stackexchange.com/a/30371/114401
    pids+=("$!")
    echo "    pid = ${pids[$i]}"
done

for pid in "${pids[@]}"; do
    wait "$pid"
    return_code="$?"
    echo "PID = $pid; return_code = $return_code"
done


podman rmi -f reddit_desktop_runtime:latest || true
FEDORA_PACKAGE=/tmp/reddit_desktop/packages/reddit_desktop-${REDDITDESKTOP_VERSION_MAJOR}.${REDDITDESKTOP_VERSION_MINOR}.${REDDITDESKTOP_VERSION_PATCH}-fedora.rpm
buildah bud --build-arg RDRPM=${FEDORA_PACKAGE} \
            --build-arg BASE_CONTAINER=$CONTAINER_REGISTRY/reddit_desktop_fedora:runtime \
            -v "${root}":/tmp/reddit_desktop/:Z \
            -f  ${root}/docker/Dockerfile.fedora.runtime -t reddit_desktop_runtime
podman save reddit_desktop_runtime:latest | gzip -9 -n > ${root}/packages/reddit_desktop_runtime.tar.gz

