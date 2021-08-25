#!/bin/bash

mkdir /tmp/build
cd /tmp/build


CMAKE_ARGS="-GNinja -DFONTS_DIRECTORY=/usr/share/reddit_desktop -DCMAKE_BUILD_TYPE=Release -DENABLE_TESTS=OFF -DENABLE_M4DC=ON -DENABLE_CMARK=OFF"

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

cmake ${CMAKE_ARGS} /tmp/reddit_desktop

ninja package

mkdir -p /tmp/reddit_desktop/packages
cp *.deb /tmp/reddit_desktop/packages/
