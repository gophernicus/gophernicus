/*
 * Gophernicus
 *
 * Copyright (c) 2009-2018 Kim Holviala <kimholviala@fastmail.com>
 * Copyright (c) 2019 Gophernicus Developers <gophernicus@gophernicus.org>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *	 * Redistributions of source code must retain the above copyright
 *	   notice, this list of conditions and the following disclaimer.
 *	 * Redistributions in binary form must reproduce the above copyright
 *	   notice, this list of conditions and the following disclaimer in the
 *	   documentation and/or other materials provided with the distribution.
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
 * Libwrap needs these defined
 */
#ifdef HAVE_LIBWRAP
#include <syslog.h>

int allow_severity = LOG_DEBUG;
int deny_severity = LOG_ERR;
#endif

/*
 * Print gopher menu line
 */
void info(state *st, char *str, char type)
{
	char buf[BUFSIZE];
	char selector[16];

	/* Convert string to output charset */
	if (st->opt_iconv) sstrniconv(st->out_charset, buf, str);
	else sstrlcpy(buf, str);

	/* Handle gopher title resources */
	strclear(selector);
	if (type == TYPE_TITLE) {
		sstrlcpy(selector, "TITLE");
		type = TYPE_INFO;
	}

	/* Output info line */
	strcut(buf, st->out_width);
	printf("%c%s\t%s\t%s" CRLF,
		type, buf, selector, DUMMY_HOST);
}


/*
 * Print footer
 */
void footer(state *st)
{
	char line[BUFSIZE];
	char buf[BUFSIZE];
	char msg[BUFSIZE];

	if (!st->opt_footer) {
#ifndef ENABLE_STRICT_RFC1436
		if (st->req_filetype == TYPE_MENU || st->req_filetype == TYPE_QUERY)
#endif
			printf("." CRLF);
		return;
	}

	/* Create horizontal line */
	strrepeat(line, '_', st->out_width);

	/* Create right-aligned footer message */
	snprintf(buf, sizeof(buf), FOOTER_FORMAT, st->server_platform);
	snprintf(msg, sizeof(msg), "%*s", st->out_width - 1, buf);

	/* Menu footer? */
	if (st->req_filetype == TYPE_MENU || st->req_filetype == TYPE_QUERY) {
		info(st, line, TYPE_INFO);
		info(st, msg, TYPE_INFO);
		printf("." CRLF);
	}

	/* Plain text footer */
	else {
		printf("%s" CRLF, line);
		printf("%s" CRLF, msg);
#ifdef ENABLE_STRICT_RFC1436
		printf("." CRLF);
#endif
	}
}

void html_encode(const char *unsafe, char *dest, int bufsize)
{
	char literals[] = "!#$&'()*+,/:;=?@[]-_.~";
	int i = 0, j = 0;
	while (unsafe[i] != '\0') {
		if (j >= bufsize - 5) {
			break;
		}
		if (strchr(literals, unsafe[i]) ||
			(unsafe[i] >= 'a' && unsafe[i] <= 'z') ||
			(unsafe[i] >= 'A' && unsafe[i] <= 'Z') ||
			(unsafe[i] >= '0' && unsafe[i] <= '9')) {
			dest[j] = unsafe[i];
			i += 1;
			j += 1;
		} else {
			j += snprintf(&dest[j], BUFSIZE - j, "%%%02x", unsafe[i]);
			i += 1;
		}
	}
	dest[j] = '\0';
}

/*
 * Print error message & exit
 */
void die(state *st, const char *message, const char *description)
{
	static const char error_gif[] = ERROR_GIF;

	log_fatal("error \"%s %s\" for request \"%s\" from %s",
	          message, description ? description : "",
	          st->req_selector, st->req_remote_addr);

	log_combined(st, HTTP_404);

	/* Handle menu errors */
	if (st->req_filetype == TYPE_MENU || st->req_filetype == TYPE_QUERY) {
		printf("3" ERROR_PREFIX "%s %s\tTITLE\t" DUMMY_HOST CRLF, message, description);
		footer(st);
	}

	/* Handle image errors */
	else if (st->req_filetype == TYPE_GIF || st->req_filetype == TYPE_IMAGE) {
		fwrite(error_gif, sizeof(error_gif), 1, stdout);
	}

	/* Handle HTML errors */
	else if (st->req_filetype == TYPE_HTML) {
		char safe_message[BUFSIZE];
		html_encode(message, safe_message, BUFSIZE);
		char safe_description[BUFSIZE];
		html_encode(description, safe_description, BUFSIZE);
		printf("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">\n"
			"<HTML>\n<HEAD>\n"
			"  <META HTTP-EQUIV=\"Content-Type\" CONTENT=\"text/html;charset=iso-8859-1\">\n"
			"  <TITLE>" ERROR_PREFIX "%1$s %2$s</TITLE>\n"
			"</HEAD>\n<BODY>\n"
			"<STRONG>" ERROR_PREFIX "%1$s %2$s</STRONG>\n"
			"<PRE>", safe_message, safe_description);
		footer(st);
		printf("</PRE>\n</BODY>\n</HTML>\n");
	}

	/* Use plain text error for other filetypes */
	else {
		printf(ERROR_PREFIX "%s %s" CRLF, message, description);
		footer(st);
	}

	/* Quit */
	exit(EXIT_FAILURE);
}


/*
 * Apache-compatible combined logging
 */
void log_combined(state *st, int status)
{
	FILE *fp;
	struct tm *ltime;
	char timestr[64];
	time_t now;

	/* Try to open the logfile for appending */
	if (!*st->log_file) return;
	if ((fp = fopen(st->log_file , "a")) == NULL) return;

	/* Format time */
	now = time(NULL);
	ltime = localtime(&now);
	strftime(timestr, sizeof(timestr), HTTP_DATE, ltime);

	/* Generate log entry */
	fprintf(fp, "%s %s:%i - [%s] \"GET %c%s HTTP/1.0\" %i %li \"%s\" \"" HTTP_USERAGENT "\"\n",
		st->req_remote_addr,
		st->server_host,
		st->server_port,
		timestr,
		st->req_filetype,
		st->req_selector,
		status,
		(long) st->req_filesize,
		st->req_referrer);
	fclose(fp);
}


/*
 * Convert gopher selector to an absolute path
 */
static void selector_to_path(state *st)
{
	DIR *dp;
	struct dirent *dir;
	struct stat file;
#ifdef HAVE_PASSWD
	struct passwd *pwd;
	char *path = EMPTY;
	char *c;
#endif
	char buf[BUFSIZE];
	int i;

	/* Handle selector rewriting */
	for (i = 0; i < st->rewrite_count; i++) {

		/* Match found? */
		if (strstr(st->req_selector, st->rewrite[i].match) == st->req_selector) {

			/* Replace match with a new string */
			snprintf(buf, sizeof(buf), "%s%s",
				st->rewrite[i].replace,
				st->req_selector + strlen(st->rewrite[i].match));

			log_debug("rewriting selector \"%s\" -> \"%s\"",
			          st->req_selector, buf);

			sstrlcpy(st->req_selector, buf);
		}
	}

#ifdef HAVE_PASSWD
	/* Virtual userdir (~user -> /home/user/public_gopher)? */
	if (st->opt_personal_spaces && *(st->user_dir) &&
		sstrncmp(st->req_selector, "/~") == MATCH) {

		/* Parse userdir login name & path */;
		sstrlcpy(buf, st->req_selector + 2);
		if ((c = strchr(buf, '/'))) {
			*c = '\0';
			path = c + 1;
		}

		/* Check user validity */
		if ((pwd = getpwnam(buf)) == NULL)
			die(st, ERR_NOTFOUND, "User not found");
		if (pwd->pw_uid < PASSWD_MIN_UID)
			die(st, ERR_NOTFOUND, "User found but UID too low");

		/* Generate absolute path to users own gopher root */
		snprintf(st->req_realpath, sizeof(st->req_realpath),
			"%s/%s/%s", pwd->pw_dir, st->user_dir, path);

		/* Check ~public_gopher access rights */
		if (stat(st->req_realpath, &file) == ERROR)
			die(st, st->req_selector, ERR_NOTFOUND);
		if ((file.st_mode & S_IROTH) == 0)
			die(st, ERR_ACCESS, "~/public_gopher not world-readable");
		if (file.st_uid != pwd->pw_uid)
			die(st, ERR_ACCESS, "~/ and ~/public_gopher owned by different users");

		/* Userdirs always come from the default vhost */
		if (st->opt_vhost)
			sstrlcpy(st->server_host, st->server_host_default);
		return;
	}
#endif

	/* Virtual hosting */
	if (st->opt_vhost) {

		/* Try looking for the selector from the current vhost */
		snprintf(st->req_realpath, sizeof(st->req_realpath), "%s/%s%s",
			st->server_root, st->server_host, st->req_selector);
		if (stat(st->req_realpath, &file) == OK) return;

		/* Loop through all vhosts looking for the selector */
		if ((dp = opendir(st->server_root)) == NULL)
			die(st, st->req_selector, ERR_NOTFOUND);
		while ((dir = readdir(dp))) {

			/* Skip .hidden dirs and . & .. */
			if (dir->d_name[0] == '.') continue;

			/* Special case - skip lost+found (don't ask) */
			if (sstrncmp(dir->d_name, "lost+found") == MATCH) continue;

			/* Generate path to the found vhost */
			snprintf(st->req_realpath, sizeof(st->req_realpath), "%s/%s%s",
				st->server_root, dir->d_name, st->req_selector);

			/* Did we find the selector under this vhost? */
			if (stat(st->req_realpath, &file) == OK) {

				/* Virtual host found - update state & return */
				sstrlcpy(st->server_host, dir->d_name);
				return;
			}
		}
		closedir(dp);
	}

	/* Handle normal selectors */
	snprintf(st->req_realpath, sizeof(st->req_realpath),
		"%s%s", st->server_root, st->req_selector);
}


/*
 * Get local IP address
 */
static char *get_local_address(void)
{
#ifdef HAVE_IPv4
	struct sockaddr_in addr;
	socklen_t addrsize = sizeof(addr);
#endif
#ifdef HAVE_IPv6
	struct sockaddr_in6 addr6;
	socklen_t addr6size = sizeof(addr6);
	static char address[INET6_ADDRSTRLEN];
#endif
	char *c;

	/* Try IPv4 first */
#ifdef HAVE_IPv4
	if (getsockname(0, (struct sockaddr *) &addr, &addrsize) == OK) {
		c = inet_ntoa(addr.sin_addr);
		if (strlen(c) > 0 && *c != '0') return c;
	}
#endif

	/* IPv4 didn't work - try IPv6 */
#ifdef HAVE_IPv6
	if (getsockname(0, (struct sockaddr *) &addr6, &addr6size) == OK) {
		if (inet_ntop(AF_INET6, &addr6.sin6_addr, address, sizeof(address))) {

			/* Strip ::ffff: IPv4-in-IPv6 prefix */
			if (sstrncmp(address, "::ffff:") == MATCH) return (address + 7);
			else return address;
		}
	}
#endif

	/* Nothing works... I'm out of ideas */
	return UNKNOWN_ADDR;
}


/*
 * Get remote peer IP address
 */
static char *get_peer_address(void)
{
#ifdef HAVE_IPv4
	struct sockaddr_in addr;
	socklen_t addrsize = sizeof(addr);
#endif
#ifdef HAVE_IPv6
	struct sockaddr_in6 addr6;
	socklen_t addr6size = sizeof(addr6);
	static char address[INET6_ADDRSTRLEN];
#endif
	char *c;

	/* Are we a CGI script? */
	if ((c = getenv("REMOTE_ADDR"))) return c;
	/* if ((c = getenv("REMOTE_HOST"))) return c; */

	/* Try IPv4 first */
#ifdef HAVE_IPv4
	if (getpeername(0, (struct sockaddr *) &addr, &addrsize) == OK) {
		c = inet_ntoa(addr.sin_addr);
		if (strlen(c) > 0 && *c != '0') return c;
	}
#endif

	/* IPv4 didn't work - try IPv6 */
#ifdef HAVE_IPv6
	if (getpeername(0, (struct sockaddr *) &addr6, &addr6size) == OK) {
		if (inet_ntop(AF_INET6, &addr6.sin6_addr, address, sizeof(address))) {

			/* Strip ::ffff: IPv4-in-IPv6 prefix */
			if (sstrncmp(address, "::ffff:") == MATCH) return (address + 7);
			else return address;
		}
	}
#endif

	/* Nothing works... I'm out of ideas */
	return UNKNOWN_ADDR;
}


/*
 * Initialize state struct to default/empty values
 */
static void init_state(state *st)
{
	static const char *filetypes[] = { FILETYPES };
	char buf[BUFSIZE];
	char *c;
	int i;

	/* Request */
	strclear(st->req_selector);
	strclear(st->req_realpath);
	strclear(st->req_query_string);
	strclear(st->req_search);
	strclear(st->req_referrer);
	sstrlcpy(st->req_local_addr, get_local_address());
	sstrlcpy(st->req_remote_addr, get_peer_address());
	/* strclear(st->req_remote_host); */
	st->req_filetype = DEFAULT_TYPE;
	st->req_protocol = PROTO_GOPHER;
	st->req_filesize = 0;

	/* Output */
	st->out_width = DEFAULT_WIDTH;
	st->out_charset = DEFAULT_CHARSET;

	/* Settings */
	sstrlcpy(st->server_root, DEFAULT_ROOT);
	sstrlcpy(st->server_host_default, DEFAULT_HOST);

	if ((c = getenv("HOSTNAME")))
		sstrlcpy(st->server_host, c);
	else if ((gethostname(buf, sizeof(buf))) != ERROR)
		sstrlcpy(st->server_host, buf);

	st->server_port = DEFAULT_PORT;
	st->server_tls_port = DEFAULT_TLS_PORT;

	st->default_filetype = DEFAULT_TYPE;
	sstrlcpy(st->map_file, DEFAULT_MAP);
	sstrlcpy(st->tag_file, DEFAULT_TAG);
	sstrlcpy(st->cgi_file, DEFAULT_CGI);
	sstrlcpy(st->user_dir, DEFAULT_USERDIR);
	strclear(st->log_file);

	st->hidden_count = 0;
	st->filetype_count = 0;
	strclear(st->filter_dir);
	st->rewrite_count = 0;

	strclear(st->server_description);
	strclear(st->server_location);
	strclear(st->server_platform);
	strclear(st->server_admin);

#ifdef __OpenBSD__
	st->extra_unveil_paths = NULL;
#endif


	/* Session */
	st->session_timeout = DEFAULT_SESSION_TIMEOUT;
	st->session_max_kbytes = DEFAULT_SESSION_MAX_KBYTES;
	st->session_max_hits = DEFAULT_SESSION_MAX_HITS;

	/* Feature options */
	st->opt_vhost = TRUE;
	st->opt_parent = TRUE;
	st->opt_header = TRUE;
	st->opt_footer = TRUE;
	st->opt_date = TRUE;
	st->opt_syslog = TRUE;
	st->opt_magic = TRUE;
	st->opt_iconv = TRUE;
	st->opt_query = TRUE;
	st->opt_caps = TRUE;
	st->opt_status = TRUE;
	st->opt_shm = TRUE;
	st->opt_root = TRUE;
	st->opt_proxy = TRUE;
	st->opt_exec = TRUE;
	st->opt_personal_spaces = TRUE;
	st->opt_http_requests = TRUE;
	st->opt_plus_menu = TRUE;
	st->debug = FALSE;

	/* Load default suffix -> filetype mappings */
	for (i = 0; filetypes[i]; i += 2) {
		if (st->filetype_count < MAX_FILETYPES) {
			sstrlcpy(st->filetype[st->filetype_count].suffix, filetypes[i]);
			st->filetype[st->filetype_count].type = *filetypes[i + 1];
			st->filetype_count++;
		}
	}
}


/*
 * Main
 */
int main(int argc, char *argv[])
{
	struct stat file;
	state st;
	char self[64];
	char selector[BUFSIZE];
	char buf[BUFSIZE];
	char *dest;
	char *c;
#ifdef HAVE_SHMEM
	struct shmid_ds shm_ds;
	shm_state *shm;
	int shmid = -1;
#endif
#ifdef ENABLE_HAPROXY1
	char remote[BUFSIZE];
	char local[BUFSIZE];
	int dummy;
#endif
#ifdef __OpenBSD__
	char pledges[256];
	char *extra_unveil;
#endif

	/* Get the name of this binary */
	if ((c = strrchr(argv[0], '/'))) sstrlcpy(self, c + 1);
	else sstrlcpy(self, argv[0]);

	/* Initialize state */
#ifdef HAVE_LOCALES
	setlocale(LC_TIME, DATE_LOCALE);
#endif
	init_state(&st);
	srand(time(NULL) / (getpid() + getppid()));

	/* Handle command line arguments */
	parse_args(&st, argc, argv);

	/* Initalize logging */
	log_init(st.opt_syslog, st.debug);

	/* Convert relative gopher roots to absolute roots */
	if (st.server_root[0] != '/') {
		char cwd_buf[512];
		const char *cwd = getcwd(cwd_buf, sizeof(cwd_buf));
		if (cwd == NULL) {
			die(&st, "getcwd", "unable to get current path");
		}
		snprintf(buf, sizeof(buf), "%s/%s", cwd, st.server_root);
		sstrlcpy(st.server_root, buf);
	}

	/* Check if TCP wrappers have something to say about this connection */
#ifdef HAVE_LIBWRAP
	if (sstrncmp(st.req_remote_addr, UNKNOWN_ADDR) != MATCH &&
		hosts_ctl(self, STRING_UNKNOWN, st.req_remote_addr, STRING_UNKNOWN) == WRAP_DENIED)
		die(&st, ERR_ACCESS, "Refused connection");
#endif

#ifdef __OpenBSD__
	/* unveil(2) support.
	 *
	 * We only enable unveil(2) if the user isn't expecting to shell-out to
	 * arbitrary commands.
	 */
	if (st.opt_exec) {
		if (st.extra_unveil_paths != NULL) {
			die(&st, "flags", "-U and executable maps cannot co-exist");
		}
		log_debug("executable gophermaps are enabled, no unveil(2)");
	} else {
		if (unveil(st.server_root, "r") == -1)
			die(&st, "unveil", st.server_root);

		/*
		 * If we want personal gopherspaces, then we have to unveil(2) the user
		 * database. This isn't actually needed if pledge(2) is enabled, as the
		 * 'getpw' promise will ensure access to this file, but it doesn't hurt
		 * to unveil it anyway.
		 */
		if (st.opt_personal_spaces) {
			log_debug("unveiling /etc/pwd.db");
			if (unveil("/etc/pwd.db", "r") == -1)
				die(&st, "unveil", "/etc/pwd.db");
		}

		/* Any extra unveil paths that the user has specified */
		char *p = st.extra_unveil_paths;
		while (p != NULL) {
			extra_unveil = strsep(&p, ":");
			if (*extra_unveil == '\0')
				continue; /* empty path */

			log_debug("unveiling extra path: %s\n", extra_unveil);
			if (unveil(extra_unveil, "r") == -1)
				die(&st, "unveil", extra_unveil);
		}

		if (unveil(NULL, NULL) == -1)
			die(&st, "unveil", "locking unveil");
	}

	/* pledge(2) support */
	if (st.opt_shm) {
		/* pledge(2) never allows shared memory */
		log_debug("shared-memory enabled, can't pledge(2)");
	} else {
		strlcpy(pledges, "stdio rpath", sizeof(pledges));

		/* Executable maps shell-out using popen(3) */
		if (st.opt_exec) {
			strlcat(pledges, " proc exec", sizeof(pledges));
			log_debug("executable gophermaps enabled, adding `proc exec' to pledge(2)");
		}

		/* Personal spaces require getpwnam(3) and getpwent(3) */
		if (st.opt_personal_spaces) {
			strlcat(pledges, " getpw", sizeof(pledges));
			log_debug("personal gopherspaces enabled, adding `getpw' to pledge(2)");
		}

		if (pledge(pledges, NULL) == -1)
			die(&st, "pledge", pledges);
	}
#endif

	/* Make sure the computer is turned on */
#ifdef __HAIKU__
	if (is_computer_on() != TRUE)
		die(&st, ERR_ACCESS, "Please turn on the computer first");
#endif

	/* Refuse to run as root */
#ifdef HAVE_PASSWD
	if (st.opt_root && getuid() == 0)
		die(&st, ERR_ACCESS, "Cowardly refusing to run as root");
#endif

	/* Try to get shared memory */
#ifdef HAVE_SHMEM
	if (st.opt_shm) {
		if ((shmid = shmget(SHM_KEY, sizeof(shm_state), IPC_CREAT | SHM_MODE)) == ERROR) {

			/* Getting memory failed -> delete the old allocation */
			shmctl(shmid, IPC_RMID, &shm_ds);
			shm = NULL;
		}
		else {
			/* Map shared memory */
			if ((shm = (shm_state *) shmat(shmid, (void *) 0, 0)) == (void *) ERROR)
				shm = NULL;

			/* Initialize mapped shared memory */
			if (shm && shm->start_time == 0) {
				shm->start_time = time(NULL);

				/* Keep server platform & description in shm */
				platform(&st);
				sstrlcpy(shm->server_platform, st.server_platform);
				sstrlcpy(shm->server_description, st.server_description);
			}
		}
	} else {
		shm = NULL;
	}

	/* Get server platform and description */
	if (shm) {
		sstrlcpy(st.server_platform, shm->server_platform);

		if (!*st.server_description)
			sstrlcpy(st.server_description, shm->server_description);
	}
	else
#endif
		platform(&st);

	/* Read selector */
get_selector:
	if (fgets(selector, sizeof(selector) - 1, stdin) == NULL)
		strclear(selector);

	/* Remove trailing CRLF */
	chomp(selector);

	log_debug("client sent \"%s\"", selector);

	/* Handle HAproxy/Stunnel proxy protocol v1 */
#ifdef ENABLE_HAPROXY1
	if (sstrncmp(selector, "PROXY TCP") == MATCH && st.opt_proxy) {
		log_debug("got proxy protocol header \"%s\"", selector);

		sscanf(selector, "PROXY TCP%d %s %s %d %d",
			&dummy, remote, local, &dummy, &st.server_port);

		/* Strip ::ffff: IPv4-in-IPv6 prefix and override old addresses */
		sstrlcpy(st.req_local_addr, local + ((sstrncmp(local, "::ffff:") == MATCH) ? 7 : 0));
		sstrlcpy(st.req_remote_addr, remote + ((sstrncmp(remote, "::ffff:") == MATCH) ? 7 : 0));

		/* My precious \o/ */
		goto get_selector;
	}
#endif

	/* Handle hURL: redirect page */
	if (sstrncmp(selector, "URL:") == MATCH) {
		st.req_filetype = TYPE_HTML;
		sstrlcpy(st.req_selector, selector);
		url_redirect(&st);
		return OK;
	}

	/* Handle gopher+ root requests (UMN gopher client is seriously borken) */
	if (sstrncmp(selector, "\t$") == MATCH && st.opt_plus_menu == TRUE) {
		printf("+-1" CRLF);
		printf("+INFO: 1Main menu\t\t%s\t%i" CRLF,
			st.server_host,
			st.server_port);
		printf("+VIEWS:" CRLF " application/gopher+-menu: <512b>" CRLF);
		printf("." CRLF);

		log_debug("got a request for gopher+ root menu");
		return OK;
	}

	/* Convert HTTP request to gopher (respond using headerless HTTP/0.9) */
	if (st.opt_http_requests && (
		sstrncmp(selector, "GET ") == MATCH ||
		sstrncmp(selector, "POST ") == MATCH)) {

		if ((c = strchr(selector, ' '))) sstrlcpy(selector, c + 1);
		if ((c = strchr(selector, ' '))) *c = '\0';

		st.req_protocol = PROTO_HTTP;

		log_debug("got HTTP request for \"%s\"", selector);
	}

	/* Save default server_host & fetch session data (including new server_host) */
	sstrlcpy(st.server_host_default, st.server_host);
#ifdef HAVE_SHMEM
	if (shm) get_shm_session(&st, shm);
#endif


	/* Parse <tab>search from selector */
	if ((c = strchr(selector, '\t'))) {
		sstrlcpy(st.req_search, c + 1);
		*c = '\0';
	}

	/* Parse ?query from selector */
	if (st.opt_query && (c = strchr(selector, '?'))) {
		sstrlcpy(st.req_query_string, c + 1);
		*c = '\0';
	}

	/* Parse ;vhost from selector */
	if (st.opt_vhost && (c = strchr(selector, ';'))) {
		sstrlcpy(st.server_host, c + 1);
		*c = '\0';
	}

	/* Loop through the selector, fix it & separate query_string */
	dest = st.req_selector;
	if (selector[0] != '/') *dest++ = '/';

	for (c = selector; *c;) {

		/* Skip duplicate slashes and /./ */
		while (*c == '/' && *(c + 1) == '/') c++;
		if (*c == '/' && *(c + 1) == '.' && *(c + 2) == '/') c += 2;

		/* Copy valid char */
		*dest++ = *c++;
	}
	*dest = '\0';

	/* Main query parameters compatibility with older versions of Gophernicus */
	if (*st.req_query_string && !*st.req_search) sstrlcpy(st.req_search, st.req_query_string);
	if (!*st.req_query_string && *st.req_search) sstrlcpy(st.req_query_string, st.req_search);

	/* Remove encodings from selector */
	strndecode(st.req_selector, st.req_selector, sizeof(st.req_selector));

	/* Deny requests for Slashdot and /../ hackers */
	if (strstr(st.req_selector, "/."))
		die(&st, ERR_ACCESS, "Refusing to serve out dotfiles");

	/* Handle /server-status requests */
#ifdef HAVE_SHMEM
	if (st.opt_status && sstrncmp(st.req_selector, SERVER_STATUS) == MATCH) {
		if (shm) server_status(&st, shm, shmid);
		return OK;
	}
#endif

	/* Remove possible extra cruft from server_host */
	if ((c = strchr(st.server_host, '\t'))) *c = '\0';

	/* Guess request filetype so we can die() with style... */
	st.req_filetype = gopher_filetype(&st, st.req_selector, FALSE);

	/* Convert seletor to path & stat() */
	selector_to_path(&st);
	log_debug("path to resource is \"%s\"", st.req_realpath);

	if (stat(st.req_realpath, &file) == ERROR) {

		/* Handle virtual /caps.txt requests */
		if (st.opt_caps && sstrncmp(st.req_selector, CAPS_TXT) == MATCH) {
#ifdef HAVE_SHMEM
			caps_txt(&st, shm);
#else
			caps_txt(&st, NULL);
#endif
			return OK;
		}

		/* Requested file not found - die() */
		die(&st, st.req_selector, ERR_NOTFOUND);
	}

	/* Fetch request filesize from stat() */
	st.req_filesize = file.st_size;

	/* Everyone must have read access but no write access */
	if ((file.st_mode & S_IROTH) == 0)
		die(&st, ERR_ACCESS, "File or directory not world-readable");
	if ((file.st_mode & S_IWOTH) != 0)
		die(&st, ERR_ACCESS, "File or directory world-writeable");

	/* If stat said it was a dir then it's a menu */
	if ((file.st_mode & S_IFMT) == S_IFDIR) st.req_filetype = TYPE_MENU;

	/* Not a dir - let's guess the filetype again... */
	else if ((file.st_mode & S_IFMT) == S_IFREG)
		st.req_filetype = gopher_filetype(&st, st.req_realpath, st.opt_magic);

	/* Menu selectors must end with a slash */
	if (st.req_filetype == TYPE_MENU && strlast(st.req_selector) != '/')
		sstrlcat(st.req_selector, "/");

	/* Change directory to wherever the resource was */
	sstrlcpy(buf, st.req_realpath);

	if ((file.st_mode & S_IFMT) != S_IFDIR) c = dirname(buf);
	else c = buf;

	if (chdir(c) == ERROR) die(&st, ERR_ACCESS, "");

	/* Keep count of hits and data transfer */
#ifdef HAVE_SHMEM
	if (shm) {
		shm->hits++;
		shm->kbytes += st.req_filesize / 1024;

		/* Update user session */
		update_shm_session(&st, shm);
	}
#endif

	/* Log the request */
	log_info("request for \"gopher%s://%s:%i/%c%s\" from %s",
	         st.server_port == st.server_tls_port ? "s" : "",
	         st.server_host,
	         st.server_port,
	         st.req_filetype,
	         st.req_selector,
	         st.req_remote_addr);

	/* Check file type & act accordingly */
	switch (file.st_mode & S_IFMT) {
		case S_IFDIR:
			log_combined(&st, HTTP_OK);
			gopher_menu(&st);
			break;

		case S_IFREG:
			log_combined(&st, HTTP_OK);
			gopher_file(&st);
			break;

		default:
			die(&st, ERR_ACCESS, "Refusing to serve out special files");
	}

	/* Clean exit */
	return OK;
}
