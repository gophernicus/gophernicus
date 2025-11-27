#! /bin/sh
TEMPDIR="$(mktemp -d /tmp/gophernicusXXXXXX)" || exit
readonly TEMPDIR

if uname | grep -q 'Darwin' ; then
    # I do not have hardware to make this work on and am uninterested in
    # making another testsuite for travis Macs at this time
    exit 0
fi

mkdir -p "$TEMPDIR"
cp .travis/test.gophermap "$TEMPDIR/gophermap"
chmod 755 "$TEMPDIR"
chmod 644 "$TEMPDIR/gophermap"

#test 1, test a basic gophermap,
printf "\n" | src/gophernicus -h test.invalid -nf -nu -nv -nx -nf -nd -nr -r "$TEMPDIR" > "$TEMPDIR"/test1.output
if ! cmp .travis/test1.answer "$TEMPDIR/test1.output" ; then
    echo test1 round1 FAILED, test info saved at "$TEMPDIR", diff between expected output and actual output:
    diff .travis/test1.answer "$TEMPDIR/test1.output"
    exit 1
else
   echo test1 round1 passed
fi
printf "\r\n" | src/gophernicus -h test.invalid -nf -nu -nv -nx -nf -nd -nr -r "$TEMPDIR" > "$TEMPDIR"/test1.output
if ! cmp .travis/test1.answer "$TEMPDIR/test1.output" ; then
    echo test1 round2 FAILED, test info saved at "$TEMPDIR", diff between expected output and actual output:
    diff .travis/test1.answer "$TEMPDIR/test1.output"
    exit 1
else
   echo test1 round2 passed
fi
#test 1, test a basic gophermap
printf "/\n" | src/gophernicus -h test.invalid -nf -nu -nv -nx -nf -nd -nr -r "$TEMPDIR" > "$TEMPDIR"/test1.output
if ! cmp .travis/test1.answer "$TEMPDIR/test1.output" ; then
    echo test1 round3 FAILED, test info saved at "$TEMPDIR", diff between expected output and actual output:
    diff .travis/test1.answer "$TEMPDIR/test1.output"
    exit 1
else
   echo test1 round3 passed
fi
printf "/\r\n" | src/gophernicus -h test.invalid -nf -nu -nv -nx -nf -nd -nr -r "$TEMPDIR" > "$TEMPDIR"/test1.output
if ! cmp .travis/test1.answer "$TEMPDIR/test1.output" ; then
    echo test1 round4 FAILED, test info saved at "$TEMPDIR", diff between expected output and actual output:
    diff .travis/test1.answer "$TEMPDIR/test1.output"
    exit 1
else
   echo test1 round4 passed
fi
#test 1, test a basic gophermap
printf "GET /\n" | src/gophernicus -h test.invalid -nf -nu -nv -nx -nf -nd -nr -r "$TEMPDIR" > "$TEMPDIR"/test1.output
if ! cmp .travis/test1.answer "$TEMPDIR/test1.output" ; then
    echo test1 round5 FAILED, test info saved at "$TEMPDIR", diff between expected output and actual output:
    diff .travis/test1.answer "$TEMPDIR/test1.output"
    exit 1
else
   echo test1 round5 passed
fi
#test 1, test a basic gophermap
printf "GET /\r\n" | src/gophernicus -h test.invalid -nf -nu -nv -nx -nf -nd -nr -r "$TEMPDIR" > "$TEMPDIR"/test1.output
if ! cmp .travis/test1.answer "$TEMPDIR/test1.output" ; then
    echo test1 round6 FAILED, test info saved at "$TEMPDIR", diff between expected output and actual output:
    diff .travis/test1.answer "$TEMPDIR/test1.output"
    exit 1
else
   echo test1 round6 passed
fi

#test 2 test a non-existant file
printf "FAKEFILE\n" | src/gophernicus -h test.invalid -nf -nu -nv -nx -nf -nd -nr -r "$TEMPDIR" > "$TEMPDIR"/test2.output
if ! cmp .travis/test2.answer "$TEMPDIR/test2.output" ; then
    echo test2 round1 FAILED, test info saved at "$TEMPDIR", diff between expected output and actual output:
    diff .travis/test2.answer "$TEMPDIR/test2.output"
    exit 1
else
   echo test2 round1 passed
fi

#test 2 test a non-existant file
printf "FAKEFILE\r\n" | src/gophernicus -h test.invalid -nf -nu -nv -nx -nf -nd -nr -r "$TEMPDIR" > "$TEMPDIR"/test2.output
if ! cmp .travis/test2.answer "$TEMPDIR/test2.output" ; then
    echo test2 round2 FAILED, test info saved at "$TEMPDIR", diff between expected output and actual output:
    diff .travis/test2.answer "$TEMPDIR/test2.output"
    exit 1
else
   echo test2 round2 passed
fi

#test 2 test a non-existant file
printf "/FAKEFILE\n" | src/gophernicus -h test.invalid -nf -nu -nv -nx -nf -nd -nr -r "$TEMPDIR" > "$TEMPDIR"/test2.output
if ! cmp .travis/test2.answer "$TEMPDIR/test2.output" ; then
    echo test2 round3 FAILED, test info saved at "$TEMPDIR", diff between expected output and actual output:
    diff .travis/test2.answer "$TEMPDIR/test2.output"
    exit 1
else
   echo test2 round3 passed
fi

#test 2 test a non-existant file
printf "/FAKEFILE\r\n" | src/gophernicus -h test.invalid -nf -nu -nv -nx -nf -nd -nr -r "$TEMPDIR" > "$TEMPDIR"/test2.output
if ! cmp .travis/test2.answer "$TEMPDIR/test2.output" ; then
    echo test2 round4 FAILED, test info saved at "$TEMPDIR", diff between expected output and actual output:
    diff .travis/test2.answer "$TEMPDIR/test2.output"
    exit 1
else
   echo test2 round4 passed
fi

#test 3 test a non-existant GIF
printf "/FAKE.GIF\n" | src/gophernicus -h test.invalid -nf -nu -nv -nx -nf -nd -nr -r "$TEMPDIR" > "$TEMPDIR"/test3.output
if ! cmp .travis/test3.answer "$TEMPDIR/test3.output" ; then
    echo "test3 round1 FAILED, test info saved at $TEMPDIR, diff not displayed (binary files expected)"
    exit 1
else
   echo test3 round1 passed
fi

#test 3 test a non-existant GIF
printf "/FAKE.GIF\r\n" | src/gophernicus -h test.invalid -nf -nu -nv -nx -nf -nd -nr -r "$TEMPDIR" > "$TEMPDIR"/test3.output
if ! cmp .travis/test3.answer "$TEMPDIR/test3.output" ; then
    echo "test3 round2 FAILED, test info saved at $TEMPDIR, diff not displayed (binary files expected)"
    exit 1
else
   echo test3 round2 passed
fi

echo "All important tests passed, deleting the temporary directory at $TEMPDIR"
rm -rf "$TEMPDIR"
