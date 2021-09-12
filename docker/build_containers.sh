#!/bin/bash

set -ex

buildah bud  -f Dockerfile.debian  -t localhost/reddit_desktop_debian:build
podman tag localhost/reddit_desktop_debian:build registry.zergiu.com:5000/reddit_desktop_debian:build
podman push registry.zergiu.com:5000/reddit_desktop_debian:build
podman rmi localhost/reddit_desktop_debian:build

buildah bud  -f Dockerfile.ubuntu  -t localhost/reddit_desktop_ubuntu:build
podman tag localhost/reddit_desktop_ubuntu:build registry.zergiu.com:5000/reddit_desktop_ubuntu:build
podman push registry.zergiu.com:5000/reddit_desktop_ubuntu:build
podman rmi localhost/reddit_desktop_ubuntu:build

buildah bud  -f Dockerfile.fedora  -t localhost/reddit_desktop_fedora:build
podman tag localhost/reddit_desktop_fedora:build registry.zergiu.com:5000/reddit_desktop_fedora:build
podman push registry.zergiu.com:5000/reddit_desktop_fedora:build
podman rmi localhost/reddit_desktop_fedora:build

