#!/bin/bash

set -ex

REL="$(git tag | tail -1)"

if command -v distrobox-enter; then
  distrobox-enter -r centos-stream9 -- /bin/bash -c "sudo dnf install -y valgrind clang gcc erofs-utils dracut rpm-build git && cd $PWD && ./build-bin-only.sh && mkdir -p /home/ecurtin/rpmbuild/SOURCES/ && git archive -o /home/ecurtin/rpmbuild/SOURCES/initoverlayfs-$REL.tar.gz --prefix initoverlayfs-$REL/ HEAD && rpmbuild -ba *.spec && sudo rpm --force -U ~/rpmbuild/RPMS/aarch64/initoverlayfs-$REL-1.el9.aarch64.rpm && sudo dracut -f --no-kernel"
else
  sudo podman run -it -v $PWD:$PWD centos:stream9 
fi

