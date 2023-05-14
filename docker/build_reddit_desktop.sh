#!/bin/bash

set -x

export VCPKG_BINARY_SOURCES=files,/opt/a,readwrite

CMAKE_ARGS="-GNinja -DVCPKG_INSTALLED_DIR=/opt/vcpkg/installed \
            -DCMAKE_TOOLCHAIN_FILE=/opt/vcpkg/scripts/buildsystems/vcpkg.cmake \
            -DPACKAGING=ON -DCMAKE_BUILD_TYPE=Release \
            -DENABLE_TESTS=OFF -DENABLE_M4DC=ON -DENABLE_CMARK=OFF -DCMAKE_INSTALL_PREFIX=/usr"
distro=""
if [ -n "$1" ]
then
    distro=$1
    CMAKE_ARGS="${CMAKE_ARGS} -DCPACK_DISTRIBUTION=${distro}"
    if [[ "${distro}" == "ubuntu" || "${distro}" == "debian" ]]
    then
        echo "Building a deb package"
        CMAKE_ARGS="${CMAKE_ARGS} -DCPACK_GENERATOR=DEB"
    fi
    if [[ "${distro}" == "fedora" || "${distro}" == "redhat" || "${distro}" == "centos" || "${distro}" == "suse" ]]
    then
        echo "Building an rpm package"
        CMAKE_ARGS="${CMAKE_ARGS} -DCPACK_GENERATOR=RPM"
    fi
fi

cmake -B/tmp/build -S/tmp/reddit_desktop ${CMAKE_ARGS}
cmake --build /tmp/build -- package

mkdir -p /tmp/reddit_desktop/packages
cp /tmp/build/reddit_desktop-*-${distro}.* /tmp/reddit_desktop/packages/
