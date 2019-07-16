# Versioning

## Latest version

3.0

## Changelog

<!--- this should be mirrored from Changelog -->

### 3.0.1

 * update documentation to `git checkout` before install
 * fix typo in docs for debian packaging

### 3.0 (from 101):

**N.B. this version has two important changes that may make it backwards-incompatible:**

 * binary changed from in.gophernicus to gophernicus
 * virtual hosting NEVER WORKED and does not work in the way previously described

#### Other changes:

 * prevent leak of executable gophermap contents
 * make sure {x,}inetd works when systemd is on the system
 * allow -j flag to work
 * add h9bnks (yargo) and fosslinux into developer roles
 * add -nx flag, blocks executable gophermaps
 * add -nu flag, disable ~/public_gopher
 * modify various documentation to markdown
 * fix various formattings and typos
 * allow inetd targets to work without update-inetd
 * correct handling of inetd.conf
 * remove list of supported platforms
 * remove example gophermaps
 * add dependencies for various distros to INSTALL.md
 * fix query urls
 * add travis ci
 * add documentation about CI

#### Upgrade guide:

If you are running gophernicus on a **production** system, **do not** upgrade to 3.0.
Wait for 3.1.

As a general guide:

If you are running 101 and haven't upgraded to newer versions **because** of
instability worries, **wait for 3.1**.

If you were running other rolling-release versions, **upgrade now**.

## History

Gophernicus has had a rough versioning history.

Versions progressed through to 2.6. Then it changed to a rolling-release scheme.
This dosen't work very well, hence the decision was made to revert to a numbered
versioning scheme. In some places, it was referred to 101 (the git commit
number) or 2.99.101 (2.99.gitcommitnumber).

These days (June 2019), the vast majority of gophernicus servers are on 101.
