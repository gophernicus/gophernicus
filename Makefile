##
## Gophernicus server Makefile
##

#
# Variables and default configuration
#
NAME     = gophernicus
PACKAGE  = $(NAME)
BINARY   = $(NAME)
VERSION  = 3.1
CODENAME = Dungeon Edition

SOURCES = $(NAME).c file.c menu.c string.c platform.c session.c options.c
HEADERS = functions.h files.h
OBJECTS = $(SOURCES:.c=.o)
README  = README.md
DOCS    = LICENSE README.md INSTALL.md changelog README.Gophermap gophertag

INSTALL = PATH=$$PATH:/usr/sbin ./install-sh -o 0 -g 0
DESTDIR = /usr
OSXDEST = /usr/local
SBINDIR = $(DESTDIR)/sbin
DOCDIR  = $(DESTDIR)/share/doc/$(PACKAGE)
MANPAGE = gophernicus.1.man 
MANDEST = $(DESTDIR)/share/man/man1/gophernicus.1

ROOT    = /var/gopher
OSXROOT = /Library/GopherServer
WRTROOT = /gopher
MAP     = gophermap

INETD   = /etc/inetd.conf
XINETD  = /etc/xinetd.d
INETLIN = "gopher	stream	tcp	nowait	nobody	$(SBINDIR)/$(BINARY)	$(BINARY) -h `hostname`"
INETPID = /var/run/inetd.pid
LAUNCHD = /Library/LaunchDaemons
PLIST   = org.$(NAME).server.plist
NET_SRV = /boot/common/settings/network/services
SYSTEMD = /lib/systemd/system /usr/lib/systemd/system
HAS_STD = /run/systemd/system
SYSCONF = /etc/sysconfig
DEFAULT = /etc/default

CC      ?= gcc
CFLAGS  = -O2 -Wall
LDFLAGS =

IPCRM   = /usr/bin/ipcrm


#
# Platform support, compatible with both BSD and GNU make
#
all:
	case `uname` in \
		Darwin)	$(MAKE) ROOT="$(OSXROOT)" DESTDIR="$(OSXDEST)" $(BINARY); ;; \
		Haiku)	$(MAKE) EXTRA_LIBS="-lnetwork" $(BINARY); ;; \
		*)	if [ -f "/usr/include/tcpd.h" ]; then $(MAKE) withwrap; else $(MAKE) $(BINARY); fi; ;; \
	esac

withwrap:
	$(MAKE) EXTRA_CFLAGS="-DHAVE_LIBWRAP" EXTRA_LIBS="-lwrap" $(BINARY)

#
# Special targets
#
deb:
	dpkg-buildpackage -rfakeroot -uc -us

#
# Building
#

headers: $(HEADERS)

$(NAME).c: headers $(NAME).h

$(BINARY): $(OBJECTS)
	$(CC) $(LDFLAGS) $(EXTRA_LDFLAGS) $(OBJECTS) $(EXTRA_LIBS) -o $@

.c.o:
	$(CC) -c $(CFLAGS) $(EXTRA_CFLAGS) -DVERSION="\"$(VERSION)\"" -DCODENAME="\"$(CODENAME)\"" -DDEFAULT_ROOT="\"$(ROOT)\"" $< -o $@


functions.h:
	echo "/* Automatically generated function definitions */" > $@
	echo >> $@
	grep -h "^[a-z]" $(SOURCES) | \
		grep -v "int main" | \
		grep -v "strlc" | \
		grep -v "[a-z]:" | \
		sed -e "s/ =.*$$//" -e "s/ *$$/;/" >> $@
	sed -i '/shm_state/i #ifdef HAVE_SHMEM' functions.h
	sed -i '/shm_state/a #endif' functions.h
	@echo

bin2c: bin2c.c
	$(CC) bin2c.c -o $@
	@echo

README: $(README)
	cat $(README) > $@

files.h: bin2c README
	sed -e '/^(end of option list)/,$$d' README > README.options
	./bin2c -0 -n README README.options > $@
	./bin2c -0 LICENSE >> $@
	./bin2c -n ERROR_GIF error.gif >> $@
	@echo


#
# Clean targets
#
clean: clean-build clean-deb

clean-build:
	rm -f $(BINARY) $(OBJECTS) $(HEADERS) README.options README bin2c

clean-deb:
	if [ -d debian/$(PACKAGE) ]; then fakeroot debian/rules clean; fi

clean-shm:
	if [ -x $(IPCRM) ]; then $(IPCRM) -M `awk '/SHM_KEY/ { print $$3 }' $(NAME).h` || true; fi


#
# Install targets
#
install: clean-shm
	@case `uname` in \
		Darwin)  $(MAKE) ROOT="$(OSXROOT)" DESTDIR="$(OSXDEST)" install-files install-docs install-root install-osx install-done; ;; \
		Haiku)   $(MAKE) SBINDIR=/boot/common/bin DOCDIR=/boot/common/share/doc/$(PACKAGE) \
		                 install-files install-docs install-root install-haiku install-done; ;; \
		*)       $(MAKE) install-files install-docs install-root; ;; \
	esac
	@if [ -d "$(HAS_STD)" ]; then $(MAKE) install-systemd install-done; \
	elif [ -d "$(XINETD)" ]; then $(MAKE) install-xinetd install-done; \
	elif [ -f "$(INETD)"  ]; then $(MAKE) install-inetd install-done; fi

.PHONY: install

install-done:
	@echo
	@echo "======================================================================"
	@echo
	@echo "If there were no errors shown above,"
	@echo "Gophernicus has now been succesfully installed. To try it out, launch"
	@echo "your favorite gopher browser and navigate to your gopher root."
	@echo
	@echo "Gopher URL...: gopher://`hostname`/"
	@for CONFFILE in /etc/sysconfig/gophernicus \
		/etc/default/gophernicus \
		/Library/LaunchDaemons/org.gophernicus.server.plist \
		/boot/common/settings/network/services \
		/lib/systemd/system/gophernicus\@.service \
		/etc/xinetd.d/gophernicus \
		/etc/inetd.conf; do \
			if [ -f $$CONFFILE ]; then echo "Configuration: $$CONFFILE"; break; fi; done;
	@echo
	@echo "======================================================================"
	@echo

install-files: $(BINARY)
	mkdir -p $(SBINDIR)
	$(INSTALL) -s -m 755 $(BINARY) $(SBINDIR)
	@echo

install-docs:
	mkdir -p $(DOCDIR)
	$(INSTALL) -m 644 $(DOCS) $(DOCDIR)
	$(INSTALL) -m 644 $(MANPAGE) $(MANDEST)
	@echo

install-root:
	if [ ! -d "$(ROOT)" -o ! -f "$(ROOT)/$(MAP)" ]; then \
		mkdir -p $(ROOT); \
		$(INSTALL) -m 644 $(MAP).sample $(ROOT); \
		ln -s $(DOCDIR) $(ROOT)/docs; \
	fi
	@echo

install-inetd: install-files install-docs install-root
	@if update-inetd --add $(INETLIN); then \
		echo update-inetd install worked ; \
	else if grep '^gopher' $(INETD) >/dev/null 2>&1 ; then \
		echo "::::: Gopher entry in $(INETD) already present -- please check! :::::"; \
		else echo "Trying to add gopher entry to $(INETD)" ; \
			echo "$(INETLIN)" >> $(INETD) ; \
			if [ -r $(INETPID) ] ; then kill -HUP `cat $(INETPID)` ; \
				else echo "::::: No PID for inetd found, not restarted -- please check! :::::" ; fi ; \
		fi ; \
	fi
	@echo

install-xinetd: install-files install-docs install-root
	if [ -d "$(XINETD)" -a ! -f "$(XINETD)/$(NAME)" ]; then \
		sed -e "s/@HOSTNAME@/`hostname`/g" $(NAME).xinetd > $(XINETD)/$(NAME); \
		[ -x /sbin/service ] && /sbin/service xinetd reload; \
	fi
	@echo

install-osx:
	if [ -d "$(LAUNCHD)" -a ! -f "$(LAUNCHD)/$(PLIST)" ]; then \
		sed -e "s/@HOSTNAME@/`hostname`/g" $(PLIST) > $(LAUNCHD)/$(PLIST); \
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
		echo "	launch $(BINARY) -h `hostname`"; \
		echo "}") >> $(NET_SRV); \
	fi
	@echo
	chown user:root $(DOCDIR)/* $(SBINDIR)/$(BINARY) $(ROOT)/$(MAP)
	@echo
	ps | grep net_server | grep -v grep | awk '{ print $$2 }' | xargs kill
	nohup /boot/system/servers/net_server >/dev/null 2>/dev/null &
	@echo

install-systemd: install-files install-docs install-root
	if [ -d "$(HAS_STD)" ]; then \
		if [ -d "$(SYSCONF)" -a ! -f "$(SYSCONF)/$(NAME)" ]; then \
			$(INSTALL) -m 644 $(NAME).env $(SYSCONF)/$(NAME); \
		fi; \
		if [ ! -d "$(SYSCONF)" -a -d "$(DEFAULT)" -a ! -f $(DEFAULT)/$(NAME) ]; then \
			$(INSTALL) -m 644 $(NAME).env $(DEFAULT)/$(NAME); \
		fi; \
		for DIR in $(SYSTEMD); do \
			if [ -d "$$DIR" ]; then \
				$(INSTALL) -m 644 $(NAME).socket $(NAME)\@.service $$DIR; \
				break; \
			fi; \
		done; \
		if pidof systemd ; then \
			systemctl daemon-reload; \
			systemctl enable $(NAME).socket; \
			systemctl start $(NAME).socket; \
		fi; \
	fi
	@echo

#
# Uninstall targets
#
uninstall: uninstall-xinetd uninstall-launchd uninstall-systemd uninstall-inetd
	rm -f $(SBINDIR)/$(BINARY)
	for DOC in $(DOCS); do rm -f $(DOCDIR)/$$DOC; done
	rmdir -p $(SBINDIR) $(DOCDIR) 2>/dev/null || true
	@echo

uninstall-inetd:
	@if [ -f "$(INETD)" ] && update-inetd --remove "^gopher.*gophernicus" ; then \
		echo update-inetd remove worked ; \
	else if grep '^gopher' $(INETD) >/dev/null 2>&1 && \
		sed -i .bak -e 's/^gopher/#gopher/' $(INETD) ; then \
			echo "commented out gopher entry in $(INETD), reloading inetd" ; \
			[ -r $(INETPID) ] && kill -HUP `cat $(INETPID)` ; \
		else echo "::::: could not find gopher entry in $(INETD) :::::" ; \
		fi ; \
	fi
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

uninstall-systemd:
	if [ -d "$(HAS_STD)" ]; then \
		for DIR in $(SYSTEMD); do \
			if [ -f "$$DIR/$(NAME).socket" ]; then \
				systemctl stop $(NAME).socket; \
				systemctl disable $(NAME).socket; \
				rm -f $$DIR/$(NAME).socket $$DIR/$(NAME)\@.service $(SYSCONF)/$(NAME) $(DEFAULT)/$(NAME); \
				systemctl daemon-reload; \
				break; \
			fi; \
		done; \
	fi
	@echo
