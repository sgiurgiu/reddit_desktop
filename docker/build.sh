#!/bin/sh


root=$(git rev-parse --show-toplevel)

podman rm -f rd

#buildah bud  -f Dockerfile.debian  -t reddit_desktop_debian:build

podman run --name rd --entrypoint=/tmp/workdir/build_reddit_desktop.sh  -v "${root}":/tmp/reddit_desktop/:Z localhost/reddit_desktop_debian:build

podman rm -f rd

