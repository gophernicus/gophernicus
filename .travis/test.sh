#!/bin/bash

echo -e "\n" | gophernicus -nf -nu -nv -nx -nf -nd > test.output
if ! cmp .travis/test.answer test.output ; then
    exit 1
fi

if ! gophernicus -v | grep -q 'Gophernicus/3.1 "Dungeon Edition"' ; then
    exit 1
fi
