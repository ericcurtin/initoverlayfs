#!/bin/bash

set -ex

REL="$(git tag | tail -1)"
uname_m="$(uname -m)"
if command -v distrobox-enter; then
  distrobox-enter -r centos-stream9 -- /bin/bash -c "sudo dnf install -y valgrind clang gcc erofs-utils dracut rpm-build git && cd $PWD && ./build-bin-only.sh && mkdir -p /home/ecurtin/rpmbuild/SOURCES/ && git archive -o /home/ecurtin/rpmbuild/SOURCES/initoverlayfs-$REL.tar.gz --prefix initoverlayfs-$REL/ HEAD && rpmbuild -ba *.spec && sudo mkdir -p /boot /initrofs /overlay /overlay/upper /overlay/work /initoverlayfs && sudo rpm --force -U ~/rpmbuild/RPMS/$uname_m/initoverlayfs-$REL-1.el9.$uname_m.rpm && sudo dracut -f --no-kernel && sudo initoverlayfs-install && sudo valgrind ./a.out"
else
  sudo podman run -it -v $PWD:$PWD centos:stream9 
fi

