#!/bin/sh


root=$(git rev-parse --show-toplevel)

podman rm -f rd
podman run --name rd -v "${root}":/tmp/reddit_desktop/:Z registry:5000/reddit_desktop_debian:build
podman rm -f rd
podman run --name rd -v "${root}":/tmp/reddit_desktop/:Z registry:5000/reddit_desktop_ubuntu:build
podman rm -f rd
podman run --name rd -v "${root}":/tmp/reddit_desktop/:Z registry:5000/reddit_desktop_fedora:build
podman rm -f rd

