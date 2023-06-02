#!/bin/bash

cd /opt
mkdir /opt/a
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh
./vcpkg install boost-asio boost-algorithm boost-beast boost-system boost-filesystem  boost-program-options  boost-process  boost-serialization boost-signals2 boost-spirit glew gumbo fmt glfw3 spdlog stb sqlite3 freetype opengl openssl uriparser zlib gtest --binarysource=files,/opt/a,readwrite


