##
## Gophernicus server Makefile
##

#
# Variables and default configuration
#
NAME    = gophernicus
PACKAGE = $(NAME)
BINARY  = in.$(NAME)
VERSION = 1.5-rc1

SOURCES = $(NAME).c file.c menu.c string.c platform.c session.c options.c
HEADERS = functions.h files.h
OBJECTS = $(SOURCES:.c=.o)
DOCS    = LICENSE README INSTALL TODO ChangeLog README.Gophermap gophertag

INSTALL = PATH=$$PATH:/usr/sbin ./install-sh -o 0 -g 0
DESTDIR = /usr
SBINDIR = $(DESTDIR)/sbin
DOCDIR  = $(DESTDIR)/share/doc/$(PACKAGE)

ROOT    = /var/gopher
OSXROOT = /Library/GopherServer
WRTROOT = /gopher
MAP     = gophermap

INETD   = /etc/inetd.conf
XINETD  = /etc/xinetd.d
LAUNCHD = /Library/LaunchDaemons
PLIST   = org.gophernicus.server.plist
NET_SRV = /boot/common/settings/network/services

DIST    = $(PACKAGE)-$(VERSION)
TGZ     = $(DIST).tar.gz
RELDIR  = /var/gopher/gophernicus.org/software/gophernicus/server/

CC      = gcc
HOSTCC	= $(CC)
CFLAGS  = -O2 -Wall
LDFLAGS = 


#
# Platform support, compatible with both BSD and GNU make
#
all:
	@case `uname` in \
		Darwin)  $(MAKE) ROOT="$(OSXROOT)" $(BINARY); ;; \
		Haiku)   $(MAKE) EXTRA_LDFLAGS="-lnetwork" $(BINARY); ;; \
		*)       $(MAKE) $(BINARY); ;; \
	esac

generic: $(BINARY)


#
# Special targets
#
deb:
	dpkg-buildpackage -rfakeroot -uc -us


#
# Building
#
$(NAME).c: $(NAME).h $(HEADERS)
	
$(BINARY): $(OBJECTS)
	$(CC) $(LDFLAGS) $(EXTRA_LDFLAGS) $(OBJECTS) -o $@

.c.o:
	$(CC) -c $(CFLAGS) $(EXTRA_CFLAGS) -DVERSION="\"$(VERSION)\"" -DDEFAULT_ROOT="\"$(ROOT)\"" $< -o $@


headers: $(HEADERS)
	@echo

functions.h:
	echo "/* Automatically generated function definitions */" > $@
	echo >> $@
	grep -h "^[a-z]" $(SOURCES) | grep -v "int main" | sed -e "s/ =.*$$//" -e "s/ *$$/;/" >> $@
	@echo

bin2c: bin2c.c
	$(HOSTCC) bin2c.c -o $@
	@echo

files.h: bin2c
	sed -n -e "1,/^ $$/p" README > README.options
	./bin2c -0 -n README README.options > $@
	./bin2c -0 LICENSE >> $@
	./bin2c -n ERROR_GIF error.gif >> $@
	@echo


#
# Cleanup after building
#
clean: clean-build clean-deb

clean-build:
	rm -f $(BINARY) $(OBJECTS) $(TGZ) $(HEADERS) README.options bin2c

clean-deb:
	if [ -d debian/$(PACKAGE) ]; then fakeroot debian/rules clean; fi


#
# Install targets
#
install:
	@case `uname` in \
		Darwin)  $(MAKE) ROOT="$(OSXROOT)" install-files install-docs install-root install-osx install-done; ;; \
		Haiku)   $(MAKE) SBINDIR=/boot/common/bin DOCDIR=/boot/common/share/doc/$(PACKAGE) \
		                 install-files install-docs install-root install-haiku install-done; ;; \
		*)       $(MAKE) install-files install-docs install-root; ;; \
	esac
	@if [ -d "$(XINETD)" ]; then $(MAKE) install-xinetd install-done; fi
	@if [ -f "$(INETD)" ]; then $(MAKE) install-inetd; fi

.PHONY: install

install-done:
	@echo
	@echo "======================================================================"
	@echo
	@echo "Gophernicus has now been succesfully installed. To try it out, launch"
	@echo "your favorite gopher browser and navigate to this URL:"
	@echo
	@echo "              gopher://`hostname`/"
	@echo
	@echo "======================================================================"
	@echo

install-files:
	mkdir -p $(SBINDIR)
	$(INSTALL) -s -m 755 $(BINARY) $(SBINDIR)
	@echo

install-docs:
	mkdir -p $(DOCDIR)
	$(INSTALL) -m 644 $(DOCS) $(DOCDIR)
	@echo

install-root:
	if [ ! -d "$(ROOT)" ]; then \
		mkdir -p $(ROOT); \
		$(INSTALL) -m 644 $(MAP) $(ROOT); \
		ln -s $(DOCDIR) $(ROOT)/docs; \
	fi
	@echo

install-inetd:
	@echo
	@echo "======================================================================"
	@echo
	@echo "Looks like your system has the traditional internet superserver inetd."
	@echo "Automatic installations aren't supported, so please add the following"
	@echo "line to the end of your /etc/inetd.conf and restart or kill -HUP the"
	@echo "inetd process."
	@echo
	@echo "gopher  stream  tcp  nowait  nobody  $(SBINDIR)/$(BINARY)  $(BINARY) -h `hostname`"
	@echo
	@echo "======================================================================"
	@echo

install-xinetd:
	if [ -d "$(XINETD)" -a ! -f "$(XINETD)/$(NAME)" ]; then \
		sed -e "s/@HOSTNAME@/`hostname`/g" $(NAME).xinetd > $(XINETD)/$(NAME); \
		[ -x /sbin/service ] && /sbin/service xinetd reload; \
	fi
	@echo

install-osx:
	if [ -d "$(LAUNCHD)" -a ! -f "$(LAUNCHD)/$(PLIST)" ]; then \
		sed -e "s/@HOSTNAME@/`hostname`/g" org.gophernicus.server.plist > \
			$(LAUNCHD)/$(PLIST); \
		launchctl load $(LAUNCHD)/$(PLIST); \
	fi
	@echo
	chown -h root:admin $(ROOT) $(ROOT)/*
	chmod -h 0775 $(ROOT) $(ROOT)/docs
	@echo

install-haiku:
	if [ -f "$(NET_SRV)" -a ! "`grep -m1 gopher $(NET_SRV)`" ]; then \
		(echo ""; \
		echo "service gopher {"; \
		echo "	family inet"; \
		echo "	protocol tcp"; \
		echo "	port 70"; \
		echo "	launch in.gophernicus -h `hostname`"; \
		echo "}") >> $(NET_SRV); \
	fi
	@echo
	chown user:root $(DOCDIR)/* $(SBINDIR)/$(BINARY) $(ROOT)/$(MAP)
	@echo
	ps | grep net_server | grep -v grep | awk '{ print $$2 }' | xargs kill
	nohup /boot/system/servers/net_server >/dev/null 2>/dev/null &
	@echo

#
# Uninstall targets
#
uninstall: uninstall-xinetd uninstall-launchd
	rm -f $(SBINDIR)/$(BINARY)
	for DOC in $(DOCS); do rm -f $(DOCDIR)/$$DOC; done
	rmdir -p $(SBINDIR) $(DOCDIR) 2>/dev/null || true
	@echo

uninstall-xinetd:
	if grep -q $(BINARY) "$(XINETD)/gopher" 2>/dev/null; then \
		rm -f $(XINETD)/gopher; \
		[ -x /sbin/service ] && service xinetd reload; \
	fi
	@echo

uninstall-launchd:
	if [ -f $(LAUNCHD)/$(PLIST) ]; then \
		launchctl unload $(LAUNCHD)/$(PLIST); \
		rm -f $(LAUNCHD)/$(PLIST); \
	fi
	if [ -L $(ROOT) ]; then \
		rm -f $(ROOT); \
	fi
	@echo


#
# Release targets
#
dist: clean functions.h
	mkdir -p /tmp/$(DIST)
	tar -cf - ./ | (cd /tmp/$(DIST) && tar -xf -)
	(cd /tmp/ && tar -cvf - $(DIST)) | gzip > $(TGZ)
	rm -rf /tmp/$(DIST)

release: dist
	cp $(TGZ) $(RELDIR)


#
# List all C defines
#
defines:
	$(CC) -dM -E $(NAME).c


#
# LOC
#
loc:
	@wc -l *.c


