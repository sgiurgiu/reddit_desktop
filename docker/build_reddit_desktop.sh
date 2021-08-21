#!/bin/sh

mkdir /tmp/build
cd /tmp/build

cmake -GNinja -DFONTS_DIRECTORY=/usr/share/reddit_desktop -DCPACK_GENERATOR=DEB -DCMAKE_BUILD_TYPE=Release -DENABLE_TESTS=OFF -DENABLE_M4DC=ON -DENABLE_CMARK=OFF /tmp/reddit_desktop

ninja package

mkdir /tmp/reddit_desktop/packages
cp *.deb /tmp/reddit_desktop/packages/
