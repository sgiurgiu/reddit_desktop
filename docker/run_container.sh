podman run --rm -v /tmp/.X11-unix:/tmp/.X11-unix -v /dev/dri:/dev/dri --security-opt=label=type:container_runtime_t -e DISPLAY localhost/reddit_desktop_runtime
