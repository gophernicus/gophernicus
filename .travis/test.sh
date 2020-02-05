#!/bin/bash

if uname | grep -q 'Darwin' ; then
    # I do not have hardware to make this work on and am uninterested in
    # making another testsuite for travis Macs at this time
    exit 0
fi

sudo cp .travis/test.gophermap /var/gopher/gophermap
sudo chmod 644 /var/gopher/gophermap

echo -e "\n" | gophernicus -nf -nu -nv -nx -nf -nd > test.output
if ! cmp .travis/test.answer test.output ; then
    exit 1
fi

if ! gophernicus -v | grep -q 'Gophernicus/3.1 "Dungeon Edition"' ; then
    exit 1
fi
