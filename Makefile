NAME     = gophernicus
PACKAGE  = $(NAME)
BINARY   = $(NAME)
VERSION  = 3.1
CODENAME = Dungeon Edition

SOURCES = src/$(NAME).c src/file.c src/menu.c src/string.c src/platform.c src/session.c src/options.c
HEADERS = src/files.h src/filetypes.h
OBJECTS = $(SOURCES:.c=.o)
README  = README.md

DESTDIR = /usr
OSXDIR  = /usr/local
SBINDIR = $(DESTDIR)/sbin
DOCDIR  = $(DESTDIR)/share/doc/$(PACKAGE)
MANPAGE = gophernicus.1
MANDEST = $(DESTDIR)/share/man/man1/gophernicus.1

INSTALL = PATH=$$PATH:/usr/sbin ./install-sh -o 0 -g 0
ROOT    = /var/gopher
OSXROOT = /Library/GopherServer
WRTROOT = /gopher
MAP     = gophermap

INETD   = /etc/inetd.conf
XINETD  = /etc/xinetd.d
# get OPTIONS line from gophernicus.env and use that also for inetd
INETOPT = $$(grep '^OPTIONS=' $(NAME).env | tail -n 1 | sed -e 's/OPTIONS="*//;s/"*$$//')
INETLIN = "gopher stream tcp nowait nobody $(SBINDIR)/$(BINARY) $(BINARY) $(INETOPT)"
INETPID = /var/run/inetd.pid
LAUNCHD = /Library/LaunchDaemons
PLIST   = org.$(NAME).server.plist
NET_SRV = /boot/common/settings/network/services
SYSTEMD = /lib/systemd/system /usr/lib/systemd/system
HAS_STD = /run/systemd/system
SYSCONF = /etc/sysconfig
DEFAULT = /etc/default

CC      ?= cc
CFLAGS  := -O2 -Wall $(CFLAGS)
LDFLAGS := $(LDFLAGS)

IPCRM   = /usr/bin/ipcrm

all:
	@case $$(uname) in \
		Darwin)	$(MAKE) ROOT="$(OSXROOT)" DESTDIR="$(OSXDIr)" src/$(BINARY); ;; \
		Haiku)	$(MAKE) LDFLAGS="$(LDFLAGS) -lnetwork" src/$(BINARY); ;; \
		*)	if [ -f "/usr/include/tcpd.h" ]; then $(MAKE) withwrap; else $(MAKE) src/$(BINARY); fi; ;; \
	esac

withwrap:
	$(MAKE) CFLAGS="$(CFLAGS) -DHAVE_LIBWRAP" LDFLAGS="$(LDFLAGS) -lwrap" src/$(BINARY)

deb:
	dpkg-buildpackage -rfakeroot -uc -us

headers: $(HEADERS)

src/$(NAME).c: headers src/$(NAME).h

src/$(BINARY): $(OBJECTS)
	$(CC) $(OBJECTS) $(CFLAGS) -o $@ $(LDFLAGS)

.c.o:
	$(CC) -c $(CFLAGS) -DVERSION="\"$(VERSION)\"" -DCODENAME="\"$(CODENAME)\"" -DDEFAULT_ROOT="\"$(ROOT)\"" $< -o $@

src/filetypes.h: src/filetypes.conf src/filetypes.awk
	awk -f src/filetypes.awk < src/filetypes.conf > $@

src/bin2c: src/bin2c.c
	$(CC) src/bin2c.c -o $@

README: $(README)
	cat $(README) > $@

src/files.h: src/bin2c README
	sed -e '/^(end of option list)/,$$d' README > README.options
	./src/bin2c -0 -n README README.options > $@
	./src/bin2c -0 LICENSE >> $@
	./src/bin2c -n ERROR_GIF error.gif >> $@
	@echo

# Clean cases

clean: clean-build clean-deb

clean-build:
	rm -rf src/$(BINARY) $(OBJECTS) $(HEADERS) README.options README src/bin2c

clean-deb:
	if [ -d debian/$(PACKAGE) ]; then fakeroot debian/rules clean; fi

clean-shm:
	if [ -x $(IPCRM) ]; then $(IPCRM) -M $$(awk '/SHM_KEY/ { print $$3 }' src/$(NAME).h) || true; fi

# Install cases

install: clean-shm
	@case $$(uname) in \
		Darwin)  $(MAKE) ROOT="$(OSXROOT)" DESTDIR="$(OSXDIR)" install-files install-docs install-root install-osx install-done; ;; \
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
	@echo "Gopher URL...: gopher://$$(hostname)/"
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

install-files: src/$(BINARY)
	mkdir -p $(SBINDIR)
	$(INSTALL) -s -m 755 src/$(BINARY) $(SBINDIR)
	@echo

install-docs:
	mkdir -p $(DOCDIR)
	$(INSTALL) -m 644 $(MANPAGE) $(MANDEST)
	@echo

install-root:
	if [ ! -d "$(ROOT)" -o ! -f "$(ROOT)/$(MAP)" ]; then \
		mkdir -p $(ROOT); \
		$(INSTALL) -m 644 $(MAP).sample $(ROOT); \
		ln -fs $(DOCDIR) $(ROOT)/docs; \
	fi
	@echo

install-inetd: install-files install-docs install-root
	@if update-inetd --add $(INETLIN); then \
		echo update-inetd install worked ; \
	else if grep '^gopher' $(INETD) >/dev/null 2>&1 ; then \
		echo "::::: Gopher entry in $(INETD) already present -- please check! :::::"; \
		else echo "Trying to add gopher entry to $(INETD)" ; \
			echo "$(INETLIN)" >> $(INETD) ; \
			if [ -r $(INETPID) ] ; then kill -HUP $$(cat $(INETPID)) ; \
				else echo "::::: No PID for inetd found, not restarted -- please check! :::::" ; fi ; \
		fi ; \
	fi
	@echo

install-xinetd: install-files install-docs install-root
	if [ -d "$(XINETD)" -a ! -f "$(XINETD)/$(NAME)" ]; then \
		sed -e "s/@HOSTNAME@/$$(hostname)/g" $(NAME).xinetd > $(XINETD)/$(NAME); \
		[ -x /sbin/service ] && /sbin/service xinetd reload; \
	fi
	@echo

install-osx:
	if [ -d "$(LAUNCHD)" -a ! -f "$(LAUNCHD)/$(PLIST)" ]; then \
		sed -e "s/@HOSTNAME@/$$(hostname)/g" src/$(PLIST) > $(LAUNCHD)/$(PLIST); \
		launchctl load $(LAUNCHD)/$(PLIST); \
	fi
	@echo
	chown -h root:admin $(ROOT) $(ROOT)/*
	chmod -h 0775 $(ROOT) $(ROOT)/docs
	@echo

install-haiku:
	if [ -f "$(NET_SRV)" -a ! "$$(grep -m1 gopher $(NET_SRV))" ]; then \
		(echo ""; \
		echo "service gopher {"; \
		echo "	family inet"; \
		echo "	protocol tcp"; \
		echo "	port 70"; \
		echo "	launch $(BINARY) -h $$(hostname)"; \
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
			$(INSTALL) -m 644 init/$(NAME).env $(SYSCONF)/$(NAME); \
		fi; \
		if [ ! -d "$(SYSCONF)" -a -d "$(DEFAULT)" -a ! -f $(DEFAULT)/$(NAME) ]; then \
			$(INSTALL) -m 644 init/$(NAME).env $(DEFAULT)/$(NAME); \
		fi; \
		for DIR in $(SYSTEMD); do \
			if [ -d "$$DIR" ]; then \
				$(INSTALL) -m 644 init/$(NAME).socket init/$(NAME)\@.service $$DIR; \
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

# Uninstall cases
uninstall: uninstall-xinetd uninstall-launchd uninstall-systemd uninstall-inetd
	rm -f $(SBINDIR)/$(BINARY)
	for DOC in $(DOCS); do rm -f $(DOCDIR)/$$DOC; done
	rmdir -p $(SBINDIR) $(DOCDIR) 2>/dev/null || true
	rm -rf $(MANDEST)
	@echo

uninstall-inetd:
	@if [ -f "$(INETD)" ] && update-inetd --remove "^gopher.*gophernicus" ; then \
		echo update-inetd remove worked ; \
	else if grep '^gopher' $(INETD) >/dev/null 2>&1 && \
		sed -i .bak -e 's/^gopher/#gopher/' $(INETD) ; then \
			echo "commented out gopher entry in $(INETD), reloading inetd" ; \
			[ -r $(INETPID) ] && kill -HUP $$(cat $(INETPID)) ; \
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
