#!/bin/bash

set -ex

if command -v distrobox-enter; then
  distrobox-enter -r centos-stream9 -- /bin/bash -c "sudo dnf install -y valgrind clang gcc; cd $PWD; ./build-bin-only.sh"
else
  sudo podman run -it -v $PWD:$PWD centos:stream9 
fi

