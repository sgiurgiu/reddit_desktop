#!/bin/bash

set -e

root=$(git rev-parse --show-toplevel)

build_version_file="${root}/build_version"
if [ ! -f "${build_version_file}" ]; then
    echo "REDDITDESKTOP_VERSION_MAJOR=1" >  "${build_version_file}"
    echo "REDDITDESKTOP_VERSION_MINOR=0" >> "${build_version_file}"
    echo "REDDITDESKTOP_VERSION_PATCH=0" >> "${build_version_file}"
fi

source "${build_version_file}"
old_patch_line="REDDITDESKTOP_VERSION_PATCH=${REDDITDESKTOP_VERSION_PATCH}"
REDDITDESKTOP_VERSION_PATCH=$((REDDITDESKTOP_VERSION_PATCH+1))
echo "Building version ${REDDITDESKTOP_VERSION_MAJOR}.${REDDITDESKTOP_VERSION_MINOR}.${REDDITDESKTOP_VERSION_PATCH}"
new_patch_line="REDDITDESKTOP_VERSION_PATCH=${REDDITDESKTOP_VERSION_PATCH}"
sed -i "s/${old_patch_line}/${new_patch_line}/" "${build_version_file}"

if [ -z $1 ]; then
    distros=("debian" "ubuntu" "fedora")
else
    distros=($1)
fi

for distro in "${distros[@]}"; do
    podman run --rm --name rd \
            -v "${root}":/tmp/reddit_desktop/:Z \
            -e REDDITDESKTOP_VERSION_MAJOR="${REDDITDESKTOP_VERSION_MAJOR}" \
            -e REDDITDESKTOP_VERSION_MINOR="${REDDITDESKTOP_VERSION_MINOR}" \
            -e REDDITDESKTOP_VERSION_PATCH="${REDDITDESKTOP_VERSION_PATCH}" \
            registry.zergiu.com:5000/reddit_desktop_$distro:build
done

