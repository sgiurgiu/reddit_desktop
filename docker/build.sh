#!/bin/sh

root=$(git rev-parse --show-toplevel)

if [ -z $1 ]; then
    distros=("debian" "ubuntu" "fedora")
else
    distros=($1)
fi

for distro in "${distros[@]}"; do
    podman run --rm --name rd -v "${root}":/tmp/reddit_desktop/:Z registry:5000/reddit_desktop_$distro:build
done

