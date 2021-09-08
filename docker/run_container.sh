#!/bin/bash

mkdir -p ~/.config/reddit_desktop

podman run --rm -v ~/.config/reddit_desktop:/root/.config/reddit_desktop \
        -v /tmp/.X11-unix:/tmp/.X11-unix --security-opt=label=type:container_runtime_t \
         -e DISPLAY -v /run/user/$(id -u)/:/run/user/0/   -e XDG_RUNTIME_DIR=/run/user/0 \
        -e PULSE_SERVER=/run/user/0/pulse/native \
        localhost/reddit_desktop_runtime
