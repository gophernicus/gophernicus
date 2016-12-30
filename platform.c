/*
 * Gophernicus - Copyright (c) 2009-2017 Kim Holviala <kim@holviala.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include "gophernicus.h"


/*
 * Get OS name, version & architecture we're running on
 */
void platform(state *st)
{
#ifdef HAVE_UNAME
#if defined(_AIX) || defined(__linux) || defined(__APPLE__)
	FILE *fp;
#endif
#ifdef __linux
	struct stat file;
#endif
	struct utsname name;
	char buf[BUFSIZE];
	char sysname[64];
	char release[64];
	char machine[64];
	char *c;

	/* Fetch system information */
	uname(&name);

	strclear(sysname);
	strclear(release);
	strclear(machine);

	/* AIX-specific */
#ifdef _AIX

	/* Fix uname() results */
	sstrlcpy(machine, "powerpc");
	snprintf(release, sizeof(release), "%s.%s",
		name.version,
		name.release);

	/* Get CPU type */
	if ((fp = popen("/usr/sbin/getsystype -i", "r"))) {
		fgets(machine, sizeof(machine), fp);
		pclose(fp);

		strreplace(machine, ' ', '_');
	}

	/* Get hardware name using shell uname */
	if (!*st->server_description &&
	    (fp = popen("/usr/bin/uname -M", "r"))) {

		fgets(st->server_description,
			sizeof(st->server_description), fp);
		pclose(fp);

		strreplace(st->server_description, ',', ' ');
		chomp(st->server_description);
	}
#endif

	/* Mac OS X, just like Unix but totally different... */
#ifdef __APPLE__

	/* Hardcode OS name */
	sstrlcpy(sysname, "MacOSX");

	/* Get OS X version */
	if ((fp = popen("/usr/bin/sw_vers -productVersion", "r"))) {
		fgets(release, sizeof(release), fp);
		pclose(fp);
	}

	/* Get hardware name */
	if (!*st->server_description &&
	    (fp = popen("/usr/sbin/sysctl -n hw.model", "r"))) {

		/* Read hardware name */
		fgets(buf, sizeof(buf), fp);
		pclose(fp);

		/* Clones are gone now so we'll hardcode the manufacturer */
		sstrlcpy(st->server_description, "Apple ");
		sstrlcat(st->server_description, buf);

		/* Remove hardware revision */
		for (c = st->server_description; *c; c++)
			if (*c >= '0' && *c <= '9') { *c = '\0'; break; }
	}
#endif

	/* Linux uname() just says Linux/kernelversion - let's dig deeper... */
#ifdef __linux

	/* Most Linux ARM/MIPS boards have hardware name in /proc/cpuinfo */
#if defined(__arm__) || defined(__mips__)
	if (!*st->server_description && (fp = fopen("/proc/cpuinfo" , "r"))) {

		while (fgets(buf, sizeof(buf), fp)) {
#ifdef __arm__
			if ((c = strkey(buf, "Hardware"))) {
#else
			if ((c = strkey(buf, "machine"))) {
#endif
				sstrlcpy(st->server_description, c);
				chomp(st->server_description);
				break;
			}
		}
		fclose(fp);
	}
#endif

	/* Get hardware type from DMI data */
	if (!*st->server_description && (fp = fopen("/sys/class/dmi/id/board_vendor" , "r"))) {
		fgets(buf, sizeof(buf), fp);
		fclose(fp);

		sstrlcpy(st->server_description, buf);
		chomp(st->server_description);

		if ((fp = fopen("/sys/class/dmi/id/board_name" , "r"))) {
			fgets(buf, sizeof(buf), fp);
			fclose(fp);

			if (*st->server_description) sstrlcat(st->server_description, " ");
			sstrlcat(st->server_description, buf);
			chomp(st->server_description);
		}
	}

	/* No DMI? Get possible hypervisor name */
	if (!*st->server_description && (fp = fopen("/sys/hypervisor/type" , "r"))) {
		fgets(buf, sizeof(buf), fp);
		fclose(fp);

		chomp(buf);
		ucfirst(buf);

		if (*buf) snprintf(st->server_description, sizeof(st->server_description), "%s virtual machine", buf);
	}

	/* Identify Gentoo */
	if (!*sysname && (fp = fopen("/etc/gentoo-release", "r"))) {
		fgets(sysname, sizeof(sysname), fp);
		fclose(fp);

		if ((c = strstr(sysname, "release "))) sstrlcpy(release, c + 8);
		if ((c = strchr(release, ' '))) *c = '\0';
		if ((c = strchr(sysname, ' '))) *c = '\0';
	}

	/* Identify RedHat */
	if (!*sysname && (fp = fopen("/etc/redhat-release", "r"))) {
		fgets(sysname, sizeof(sysname), fp);
		fclose(fp);

		if ((c = strstr(sysname, "release "))) sstrlcpy(release, c + 8);
		if ((c = strchr(release, ' '))) *c = '\0';
		if ((c = strchr(sysname, ' '))) *c = '\0';

		if (strcmp(sysname, "Red") == MATCH) sstrlcpy(sysname, "RedHat");
	}

	/* Identify Slackware */
	if (!*sysname && (fp = fopen("/etc/slackware-version", "r"))) {
		fgets(sysname, sizeof(sysname), fp);
		fclose(fp);

		if ((c = strchr(sysname, ' '))) {
			sstrlcpy(release, c + 1);
			*c = '\0';
		}
	}

	/* Identify CRUX */
	if (!*sysname && stat("/usr/bin/crux", &file) == OK && (file.st_mode & S_IXOTH)) {

		sstrlcpy(sysname, "CRUX");

		if ((fp = popen("/usr/bin/crux", "r"))) {
			fgets(buf, sizeof(buf), fp);
			pclose(fp);

			if ((c = strchr(buf, ' ')) && (c = strchr(c + 1, ' ')))
				sstrlcpy(release, c + 1);
		}
	}

	/* Uh-oh.... how about a standard Linux with lsb_release? */
	if (stat("/usr/bin/lsb_release", &file) == OK && (file.st_mode & S_IXOTH)) {

		if (!*sysname && (fp = popen("/usr/bin/lsb_release -i -s", "r"))) {
			fgets(sysname, sizeof(sysname), fp);
			pclose(fp);
		}

		if (!*release && (fp = popen("/usr/bin/lsb_release -r -s", "r"))) {
			fgets(release, sizeof(release), fp);
			pclose(fp);
		}
	}

	/* OK, nothing worked - let's try /etc/issue for sysname */
	if (!*sysname && (fp = fopen("/etc/issue", "r"))) {
		fgets(sysname, sizeof(sysname), fp);
		fclose(fp);

		if ((c = strchr(sysname, ' '))) *c = '\0';
		if ((c = strchr(sysname, '\\'))) *c = '\0';
	}

	/* Debian version should be in /etc/debian_version */
	if (!*release && (fp = fopen("/etc/debian_version", "r"))) {
		fgets (release, sizeof(release), fp);
		fclose(fp);

		if ((c = strchr(release, '/'))) *c = '\0';
	}
#endif

	/* Haiku OS */
#ifdef __HAIKU__

	/* Fix release name */
	snprintf(release, sizeof(release), "R%s", name.release);
#endif

	/* Fill in the blanks using uname() data */
	if (!*sysname) sstrlcpy(sysname, name.sysname);
	if (!*release) sstrlcpy(release, name.release);
	if (!*machine) sstrlcpy(machine, name.machine);

	/* I always liked weird Perl-only functions */
	chomp(sysname);
	chomp(release);
	chomp(machine);

	/* We're only interested in major.minor version */
	if ((c = strchr(release, '.'))) if ((c = strchr(c + 1, '.'))) *c = '\0';
	if ((c = strchr(release, '-'))) *c = '\0';
	if ((c = strchr(release, '/'))) *c = '\0';

	/* Create a nicely formatted platform string */
	snprintf(st->server_platform, sizeof(st->server_platform), "%s/%s %s",
	         sysname,
#if defined(__OpenBSD__) || defined(__FreeBSD__) || defined(__NetBSD__)
	         machine,
	         release);
#else
	         release,
	         machine);
#endif 

	/* Debug */
	if (st->debug) {
		syslog(LOG_INFO, "generated platform string \"%s\"",
			st->server_platform);
	}

#else
	/* Fallback reply */
	sstrlcpy(st->server_platform, "Unknown computer-like system");
#endif
}


/*
 * Return current CPU load
 */
float loadavg(void)
{
	FILE *fp;
	char buf[BUFSIZE];

	/* Faster Linux version */
#ifdef __linux
	buf[0] = '\0';
	if ((fp = fopen("/proc/loadavg" , "r")) == NULL) return 0;
	fgets(buf, sizeof(buf), fp);
	fclose(fp);

	return (float) atof(buf);

	/* Generic slow version - parse the output of uptime */
#else
#ifdef HAVE_POPEN
	char *c;

	if ((fp = popen("/usr/bin/uptime", "r"))) {
		fgets(buf, sizeof(buf), fp);
		pclose(fp);

		if ((c = strstr(buf, "average: ")) || (c = strstr(buf, "averages: ")))
			return (float) atof(c + 10);
	}
#endif

	/* Fallback reply */
	return 0;
#endif
}


