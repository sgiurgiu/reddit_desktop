#!/bin/sh


root=$(git rev-parse --show-toplevel)

podman run --rm --name rd -v "${root}":/tmp/reddit_desktop/:Z registry:5000/reddit_desktop_debian:build
podman run --rm --name rd -v "${root}":/tmp/reddit_desktop/:Z registry:5000/reddit_desktop_ubuntu:build
podman run --rm --name rd -v "${root}":/tmp/reddit_desktop/:Z registry:5000/reddit_desktop_fedora:build


