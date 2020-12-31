#!/bin/sh
set -ex

RELEASE=3.1

# Create release branch
git branch "v${RELEASE}"

# Create tmpdir
TMPDIR=$(mktemp -d)
cp -r . "${TMPDIR}/gophernicus-${RELEASE}"
cd "${TMPDIR}/gophernicus-${RELEASE}"

# Remove build leftovers etc
rm -rf src/*.o src/files.h src/filetypes.h src/bin2c src/gophernicus Makefile README README.options
# Remove things
rm -rf .git .gitignore .travis .travis.yml release.sh

# Change readme for devel
sed -i '/The master branch is rolling Development/d' README.md
sed -i "s/ DEVEL$/ ${RELEASE}/" README.md

# Create tarball(s)
cd "${TMPDIR}"
tar -czvf "gophernicus-${RELEASE}.tar.gz" "gophernicus-${RELEASE}"
tar -cjvf "gophernicus-${RELEASE}.tar.bz2" "gophernicus-${RELEASE}"
tar -cJvf "gophernicus-${RELEASE}.tar.xz" "gophernicus-${RELEASE}"

# Create binaries
cd "${TMPDIR}/gophernicus-${RELEASE}"

# x86_64-glibc
./configure --listener=none --os=linux --hostname=HOSTNAME
make
strip src/gophernicus
cp src/gophernicus "../gophernicus-${RELEASE}-x86_64-glibc"
make clean

# x86-64-static
export CFLAGS="-static"
export LDFLAGS="-static"
./configure --listener=none --os=linux --hostname=HOSTNAME
make
strip src/gophernicus
cp src/gophernicus "../gophernicus-${RELEASE}-x86_64-static"
make clean
