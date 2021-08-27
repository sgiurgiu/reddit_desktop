#!/bin/sh

buildah bud  -f Dockerfile.debian  -t reddit_desktop_debian:build
podman tag localhost/reddit_desktop_debian:build registry:5000/reddit_desktop_debian:build
podman push registry:5000/reddit_desktop_debian:build --tls-verify=false
podman rmi localhost/reddit_desktop_debian:build

buildah bud  -f Dockerfile.ubuntu  -t reddit_desktop_ubuntu:build
podman tag localhost/reddit_desktop_ubuntu:build registry:5000/reddit_desktop_ubuntu:build
podman push registry:5000/reddit_desktop_ubuntu:build --tls-verify=false
podman rmi localhost/reddit_desktop_ubuntu:build

buildah bud  -f Dockerfile.fedora  -t reddit_desktop_fedora:build
podman tag localhost/reddit_desktop_fedora:build registry:5000/reddit_desktop_fedora:build
podman push registry:5000/reddit_desktop_fedora:build --tls-verify=false
podman rmi localhost/reddit_desktop_fedora:build

