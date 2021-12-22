#!/bin/bash

set -ex

if [ -z $CONTAINER_REGISTRY ]; then
    echo "FATAL: Please set the CONTAINER_REGISTRY environment variable to point to where the containers are located."
    exit 1
fi

buildah bud  -f Dockerfile.debian  -t localhost/reddit_desktop_debian:build
podman tag localhost/reddit_desktop_debian:build $CONTAINER_REGISTRY/reddit_desktop_debian:build
podman push $CONTAINER_REGISTRY/reddit_desktop_debian:build
podman rmi localhost/reddit_desktop_debian:build

buildah bud  -f Dockerfile.ubuntu  -t localhost/reddit_desktop_ubuntu:build
podman tag localhost/reddit_desktop_ubuntu:build $CONTAINER_REGISTRY/reddit_desktop_ubuntu:build
podman push $CONTAINER_REGISTRY/reddit_desktop_ubuntu:build
podman rmi localhost/reddit_desktop_ubuntu:build

buildah bud  -f Dockerfile.fedora  -t localhost/reddit_desktop_fedora:build
podman tag localhost/reddit_desktop_fedora:build $CONTAINER_REGISTRY/reddit_desktop_fedora:build
podman push $CONTAINER_REGISTRY/reddit_desktop_fedora:build
podman rmi localhost/reddit_desktop_fedora:build

buildah bud -f Dockerfile.fedora.base_runtime -t localhost/reddit_desktop_fedora:runtime
podman tag localhost/reddit_desktop_fedora:runtime $CONTAINER_REGISTRY/reddit_desktop_fedora:runtime
podman push $CONTAINER_REGISTRY/reddit_desktop_fedora:runtime
podman rmi localhost/reddit_desktop_fedora:runtime
