#!/bin/bash

set -x

export VCPKG_ROOT=/opt/vcpkg
CMAKE_PRESET=""
distro=""
cd /tmp/reddit_desktop

if [ -n "$1" ]
then
    distro=$1
    package_name="reddit-desktop-${REDDITDESKTOP_VERSION}-${distro}"
    if [[ "${distro}" == "ubuntu" || "${distro}" == "debian" ]]
    then
        echo "Building a deb package"
        CMAKE_PRESET="release-deb"
        export OS_FAMILY=${distro}
        export OS_VERSION=$( . /etc/os-release && echo "$VERSION_ID" )
        export OS_DIST_TAG="deb${OS_VERSION}"
        if [[ "${distro}" == "ubuntu" ]]
        then
            export OS_DIST_TAG="ubuntu${OS_VERSION}"
        fi
    fi
    if [[ "${distro}" == "fedora" ]]
    then
        echo "Building an rpm package"
        CMAKE_PRESET="release"
        export OS_FAMILY=fedora
        export OS_VERSION=$(rpm --eval '%{fedora}')
        export OS_DIST_TAG="fc${OS_VERSION}"
        echo "OS_VERSION=${OS_VERSION}" > build.env
        echo "OS_FAMILY=${OS_FAMILY}" >> build.env
        echo "OS_DIST_TAG=${OS_DIST_TAG}" >> build.env
    fi
    if [[ "${distro}" == "appimage" ]]
    then
        echo "Building an appimage package"
        package_name="simpleiptv-${REDDITDESKTOP_VERSION}.AppImage"
        CMAKE_PRESET="release-tgz"
    fi
fi

cmake_exe=$(find /opt/ -name cmake -type f -executable -print | sort | head -n 1)
if [[ -z ${cmake_exe} ]]
then
    echo "Cannot find cmake in /opt using whatever is installed"
    cmake_exe="cmake"
fi


rm -rf build-release
mkdir -p /tmp/reddit_desktop/packages

${cmake_exe}  --workflow --preset=${CMAKE_PRESET}

if [[ "${distro}" == "appimage" ]]
then
    export NO_STRIP=true
    rm -rf /tmp/reddit_desktop/AppDir
    mkdir -p /tmp/reddit_desktop/AppDir/usr
    tar -xvf /tmp/reddit_desktop/build-release/reddit_desktop-${REDDITDESKTOP_VERSION}-${distro}.tar.gz -C /tmp/reddit_desktop/AppDir/usr
    cp -f /tmp/reddit_desktop/redditdesktop_appimage.desktop /tmp/reddit_desktop/AppDir/usr/share/applications/reddit-desktop.desktop
    /opt/linuxdeployqt-continuous-x86_64.AppImage ./AppDir/usr/share/applications/reddit-desktop.desktop -appimage
    cp RedditDesktop*.AppImage /tmp/reddit_desktop/packages/  
else
    cp /tmp/reddit_desktop/build-release/reddit_desktop* /tmp/reddit_desktop/packages/
fi
