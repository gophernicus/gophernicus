# Compiling and installing Gophernicus

Gophernicus requires a C compiler but no extra libraries aside
from standard LIBC ones. Care has been taken to use only
standard POSIX syscalls so that it should work pretty much on
any \*nix system.

To compile and install run:

```
$ git clone https://github.com/gophernicus/gophernicus.git
$ cd gophernicus
$ make
$ sudo make install
```

after having set the correct public hostname in the `gophernicus.env`
file. If this is wrong, selectors ("gopher links") won't work!

On \*nix systems, `hostname` might give you an idea, but please
note this might be completely wrong, especially on your personal
machine at home or on some cheap virtual server. If you know you
have a fixed numerical IP, you can also directly use that.
For testing, just keep the default value of `localhost` which will
result in selectors working only when you're connecting locally.

That's it - Gophernicus should now be installed, preconfigured
and running under gopher://<HOSTNAME>/. And more often than not,
It Just Works(tm).

By default Gophernicus serves gopher documents from `/var/gopher`
although that can be changed by using the `-r <root>` parameter.
To enable virtual hosting create hostname directories under
the gopher root and make sure you have at least the primary
hostname (the one set with `-h <hostname>`) directory available
(`mkdir /var/gopher/$HOSTNAME`).


## Dependencies

These were obtained from a base docker installation, what we
(will) be using on Travis.

### Ubuntu 18.04, 16.04, Debian Sid, Buster, Stretch, Jessie
- build-essential
- git
- libwrap0-dev for tcp

### Centos 6, 7
- the group 'Development Tools'. less is probably required, but
  I know this works and couldn't be bothered to find out what was
  actually required.

### Fedora 29, 30, rawhide
- the group 'Development Tools'. less is probably required, but
  I know this works and couldn't be bothered to find out what was
  actually required.
- net-tools

### OpenSuse Leap, Tumbleweed
- the pattern devel_C_C++
- the pattern devel_basis
- git

### archlinux
- base-devel
- git

### Gentoo
- git

### Alpine Linux
- alpine-sdk. once again, less is probably required.. blah blah.

### Other installation targets

Suppose your server runs systemd, but you'd rather have Gophernicus
started with inetd or xinetd.  To do that, do `make install-inetd`
or `make install-xinetd`.  Likewise use `make uninstall-inetd` or
`make uninstall-xinetd` to uninstall Gophernicus.


## Compiling with TCP wrappers

Gophernicus uses no extra libraries... well... except libwrap
(TCP wrappers) if it is installed with headers in default Unix
directories at the time of compiling. If you have the headers
installed and don't want wrapper support, run 'make generic'
instead of just 'make', and if you have wrappers installed in
non-standard place and want to force compile with wrappers
just run 'make withwrap'.

For configuring IP access lists with TCP wrappers, take a look
at the files `/etc/hosts.allow` and `/etc/hosts.deny` (because the
manual pages suck). Use the daemon name "gophernicus" to
make your access lists.


## Running with traditional inetd superserver

If you want to run Gophernicus under the traditional Unix inetd, the
below line should be added to your `/etc/inetd.conf` and the inetd
process restarted.

```
gopher  stream  tcp  nowait  nobody  /usr/sbin/gophernicus  gophernicus -h <hostname>
```

The Makefile will automatically do this for you and remove it when
uninstalling.


## Compiling on Debian Linux (and Ubuntu)

The above commands work on Debian just fine, but if you prefer
having everything installed as packages run `make deb` instead
of plain `make`. If all the dependencies were in place you'll
end up with an offical-looking deb package in the parent
directory (don't ask - that's just how it works). And instead
of `sudo make install` you should just install the deb with
`dpkg -i ../gophernicus_*.deb` after which It Should Just
Work(tm).

If you need TCP wrappers support on Debian/Ubuntu, please
install libwrap0-dev before compiling.


## Cross-compiling

Cross-compiling to a different target architecture can be done
by defining HOSTCC and CC to be different compilers. HOSTCC
must point to a local arch compiler, and CC to the target
arch one.

```
$ make HOSTCC=gcc CC=target-arch-gcc
```

## Shared memory issues

Gophernicus uses SYSV shared memory for session tracking and
statistics. It creates the shared memory block using mode 600
and a predefined key which means that a shared memory block
created with one user cannot be used by another user. Simply
said, running gophernicus under various different user
accounts may create a situation where the memory block is locked
to the wrong user.

If that happens you can simply delete the memory block and
let Gophernicus recreate it - no harm done:

```
$ sudo make clean-shm
```


## Porting to different platforms

If you need to port Gophernicus to a new platform, please take a look at
gophernicus.h which has a bunch of `#define HAVE_...` Fiddling with those
usually makes it possible to compile a working server.
If you succeed in compiling Gophernicus to a new platform please send
the patches to <gophernicus at gophernicus dot org> so we can include
them into the next release -- or even better, commit them to your fork
on Github and make a pull request!

## Supported Platforms

| Platform     | Versions                     |
| ------------ | ---------------------------- |
| Ubuntu       | 18.04, 16.04                 |
| Debian       | Sid, Buster, Stretch, Jessie |
| Centos       | 7, 6                         |
| Fedora       | 29, 30, Rawhide              |
| Opensuse     | Leap, Tumbleweed             |
| Arch Linux   | up to date                   |
| Gentoo       | up to date                   |
| Alpine Linux | Edge, 3.9                    |
| FreeBSD      | 12.0                         |
