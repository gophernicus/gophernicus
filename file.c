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
 * Send a binary file to the client
 */
void send_binary_file(state *st)
{
	/* Faster sendfile() version */
#ifdef HAVE_SENDFILE
	int fd;
	off_t offset = 0;

	if (st->debug) syslog(LOG_INFO, "outputting binary file \"%s\"", st->req_realpath);

	if ((fd = open(st->req_realpath, O_RDONLY)) == ERROR) return;
	sendfile(1, fd, &offset, st->req_filesize);
	close(fd);

	/* More compatible POSIX fread()/fwrite() version */
#else
	FILE *fp;
	char buf[BUFSIZE];
	int bytes;

	if (st->debug) syslog(LOG_INFO, "outputting binary file \"%s\"", st->req_realpath);

	if ((fp = fopen(st->req_realpath , "r")) == NULL) return;
	while ((bytes = fread(buf, 1, sizeof(buf), fp)) > 0)
		fwrite(buf, bytes, 1, stdout);
	fclose(fp);
#endif
}


/*
 * Send a text file to the client
 */
void send_text_file(state *st)
{
	FILE *fp;
	char in[BUFSIZE];
	char out[BUFSIZE];
	int line;

	if (st->debug) syslog(LOG_INFO, "outputting text file \"%s\"", st->req_realpath);
	if ((fp = fopen(st->req_realpath , "r")) == NULL) return;

	/* Loop through the file line by line */
	line = 0;

	while (fgets(in, sizeof(in), fp)) {

		/* Covert to output charset & print */
		if (st->opt_iconv) sstrniconv(st->out_charset, out, in);
		else sstrlcpy(out, in);

		chomp(out);

#ifdef ENABLE_STRICT_RFC1436
		if (strcmp(out, ".") == MATCH) printf(".." CRLF);
		else
#endif
		printf("%s" CRLF, out);
		line++;
	}

#ifdef ENABLE_STRICT_RFC1436
	printf("." CRLF);
#endif
	fclose(fp);
}


/*
 * Print hURL redirect page
 */
void url_redirect(state *st)
{
	char dest[BUFSIZE];
	char *c;

	/* Basic security checking */
	sstrlcpy(dest, st->req_selector + 4);

	if (sstrncmp(dest, "http://") != MATCH &&
	    sstrncmp(dest, "https://") != MATCH &&
	    sstrncmp(dest, "ftp://") != MATCH &&
	    sstrncmp(dest, "mailto:") != MATCH)
		die(st, ERR_ACCESS, "Refusing to HTTP redirect unsafe protocols");

	if ((c = strchr(dest, '"'))) *c = '\0';
	if ((c = strchr(dest, '?'))) *c = '\0';

	/* Log the redirect */
	if (st->opt_syslog) {
		syslog(LOG_INFO, "request for \"gopher%s://%s:%i/h%s\" from %s",
			(st->server_port == st->server_tls_port ? "s" : ""),
			st->server_host,
			st->server_port,
			st->req_selector,
			st->req_remote_addr);
	}
	log_combined(st, HTTP_OK);

	/* Output HTML */
	printf("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">\n"
		"<HTML>\n<HEAD>\n"
		"  <META HTTP-EQUIV=\"Refresh\" content=\"1;URL=%1$s\">\n"
		"  <META HTTP-EQUIV=\"Content-Type\" CONTENT=\"text/html;charset=iso-8859-1\">\n"
		"  <TITLE>URL Redirect page</TITLE>\n"
		"</HEAD>\n<BODY>\n"
		"<STRONG>Redirecting to <A HREF=\"%1$s\">%1$s</A></STRONG>\n"
		"<PRE>\n", dest);
	footer(st);
	printf("</PRE>\n</BODY>\n</HTML>\n");
}


/*
 * Handle /server-status
 */
#ifdef HAVE_SHMEM
void server_status(state *st, shm_state *shm, int shmid)
{
	struct shmid_ds shm_ds;
	time_t now;
	time_t uptime;
	int sessions;
	int i;

	/* Log the request */
	if (st->opt_syslog) {
		syslog(LOG_INFO, "request for \"gopher%s://%s:%i/0" SERVER_STATUS "\" from %s",
			(st->server_port == st->server_tls_port ? "s" : ""),
			st->server_host,
			st->server_port,
			st->req_remote_addr);
	}
	log_combined(st, HTTP_OK);

	/* Quit if shared memory isn't initialized yet */
	if (!shm) return;

	/* Update counters */
	shm->hits++;
	shm->kbytes += 1;

	/* Get server uptime */
	now = time(NULL);
	uptime = (now - shm->start_time) + 1;

	/* Get shared memory info */
	shmctl(shmid, IPC_STAT, &shm_ds);

	/* Print statistics */
	printf("Total Accesses: %li" CRLF
		"Total kBytes: %li" CRLF
		"Uptime: %i" CRLF
		"ReqPerSec: %.3f" CRLF
		"BytesPerSec: %li" CRLF
		"BytesPerReq: %li" CRLF
		"BusyServers: %i" CRLF
		"IdleServers: 0" CRLF
		"CPULoad: %.2f" CRLF,
			shm->hits,
			shm->kbytes,
			(int) uptime,
			(float) shm->hits / (float) uptime,
			shm->kbytes * 1024 / (int) uptime,
			shm->kbytes * 1024 / (shm->hits + 1),
			(int) shm_ds.shm_nattch,
			loadavg());

	/* Print active sessions */
	sessions = 0;

	for (i = 0; i < SHM_SESSIONS; i++) {
		if ((now - shm->session[i].req_atime) < st->session_timeout) {
			sessions++;

			printf("Session: %-4i %-40s %-4li %-7li gopher%s://%s:%i/%c%s" CRLF,
				(int) (now - shm->session[i].req_atime),
				shm->session[i].req_remote_addr,
				shm->session[i].hits,
				shm->session[i].kbytes,
				(shm->session[i].server_port == st->server_tls_port ? "s" : ""),
				shm->session[i].server_host,
				shm->session[i].server_port,
				shm->session[i].req_filetype,
				shm->session[i].req_selector);
		}
	}

	printf("Total Sessions: %i" CRLF, sessions);
}
#endif


/*
 * Handle /caps.txt
 */
void caps_txt(state *st, shm_state *shm)
{
	/* Log the request */
	if (st->opt_syslog) {
		syslog(LOG_INFO, "request for \"gopher%s://%s:%i/0" CAPS_TXT "\" from %s",
			(st->server_port == st->server_tls_port ? "s" : ""),
			st->server_host,
			st->server_port,
			st->req_remote_addr);
	}
	log_combined(st, HTTP_OK);

	/* Update counters */
#ifdef HAVE_SHMEM
	if (shm) {
		shm->hits++;
		shm->kbytes += 1;

		/* Update session data */
		st->req_filesize += 1024;
		update_shm_session(st, shm);
	}
#endif

	/* Standard caps.txt stuff */
	printf("CAPS" CRLF
		CRLF
		"##" CRLF
		"## This is an automatically generated caps file." CRLF
		"##" CRLF
		CRLF
		"CapsVersion=1" CRLF
		"ExpireCapsAfter=%i" CRLF
		CRLF
		"PathDelimeter=/" CRLF
		"PathIdentity=." CRLF
		"PathParent=.." CRLF
		"PathParentDouble=FALSE" CRLF
		"PathKeepPreDelimeter=FALSE" CRLF
		"ServerSupportsStdinScripts=TRUE" CRLF
		"ServerDefaultEncoding=%s" CRLF
		"ServerTLSPort=%i" CRLF
		CRLF
		"ServerSoftware=" SERVER_SOFTWARE CRLF
		"ServerSoftwareVersion=" VERSION " \"" CODENAME "\"" CRLF
		"ServerArchitecture=%s" CRLF,
			st->session_timeout,
			strcharset(st->out_charset),
			st->server_tls_port,
			st->server_platform);

	/* Optional keys */
	if (*st->server_description)
		printf("ServerDescription=%s" CRLF, st->server_description);
	if (*st->server_location)
		printf("ServerGeolocationString=%s" CRLF, st->server_location);
	if (*st->server_admin)
		printf("ServerAdmin=%s" CRLF, st->server_admin);
}


/*
 * Setup environment variables as per the CGI spec
 */
void setenv_cgi(state *st, char *script)
{
	char buf[BUFSIZE];

	/* Security */
	setenv("PATH", SAFE_PATH, 1);

	/* Set up the environment as per CGI spec */
	setenv("GATEWAY_INTERFACE", "CGI/1.1", 1);
	setenv("CONTENT_LENGTH", "0", 1);
	setenv("QUERY_STRING", st->req_query_string, 1);
	snprintf(buf, sizeof(buf), SERVER_SOFTWARE_FULL, st->server_platform);
	setenv("SERVER_SOFTWARE", buf, 1);
	setenv("SERVER_ARCH", st->server_platform, 1);
	setenv("SERVER_DESCRIPTION", st->server_description, 1);
	snprintf(buf, sizeof(buf), SERVER_SOFTWARE "/" VERSION);
	setenv("SERVER_VERSION", buf, 1);

	if (st->req_protocol == PROTO_HTTP)
		setenv("SERVER_PROTOCOL", "HTTP/0.9", 1);
	else
		setenv("SERVER_PROTOCOL", "RFC1436", 1);

	if (st->server_port == st->server_tls_port) {
		setenv("HTTPS", "on", 1);
		setenv("TLS", "on", 1);
	}

	setenv("SERVER_NAME", st->server_host, 1);
	snprintf(buf, sizeof(buf), "%i", st->server_port);
	setenv("SERVER_PORT", buf, 1);
	snprintf(buf, sizeof(buf), "%i", st->server_tls_port);
	setenv("SERVER_TLS_PORT", buf, 1);
	setenv("REQUEST_METHOD", "GET", 1);
	setenv("DOCUMENT_ROOT", st->server_root, 1);
	setenv("SCRIPT_NAME", st->req_selector, 1);
	setenv("SCRIPT_FILENAME", script, 1);
	setenv("LOCAL_ADDR", st->req_local_addr, 1);
	setenv("REMOTE_ADDR", st->req_remote_addr, 1);
	setenv("HTTP_REFERER", st->req_referrer, 1);
#ifdef HAVE_SHMEM
	snprintf(buf, sizeof(buf), "%x", st->session_id);
	setenv("SESSION_ID", buf, 1);
#endif
	setenv("HTTP_ACCEPT_CHARSET", strcharset(st->out_charset), 1);

	/* Gophernicus extras */
	snprintf(buf, sizeof(buf), "%c", st->req_filetype);
	setenv("GOPHER_FILETYPE", buf, 1);
	setenv("GOPHER_CHARSET", strcharset(st->out_charset), 1);
	setenv("GOPHER_REFERER", st->req_referrer, 1);
	snprintf(buf, sizeof(buf), "%i", st->out_width);
	setenv("COLUMNS", buf, 1);
	snprintf(buf, sizeof(buf), CODENAME);
	setenv("SERVER_CODENAME", buf, 1);

	/* Bucktooth extras */
	if (*st->req_query_string) {
		snprintf(buf, sizeof(buf), "%s?%s",
			st->req_selector, st->req_query_string);
		setenv("SELECTOR", buf, 1);
	}
	else setenv("SELECTOR", st->req_selector, 1);

	setenv("SERVER_HOST", st->server_host, 1);
	setenv("REQUEST", st->req_selector, 1);
	setenv("SEARCHREQUEST", st->req_search, 1);
}


/*
 * Execute a CGI script
 */
void run_cgi(state *st, char *script, char *arg)
{
	/* Setup environment & execute the binary */
	if (st->debug) syslog(LOG_INFO, "executing script \"%s\"", script);

	setenv_cgi(st, script);
	execl(script, script, arg, NULL);

	/* Didn't work - die */
	die(st, ERR_ACCESS, NULL);
}


/*
 * Handle file selectors
 */
void gopher_file(state *st)
{
	struct stat file;
	char buf[BUFSIZE];
	char *c;

	/* Refuse to serve out gophermaps/tags */
	if ((c = strrchr(st->req_realpath, '/'))) c++;
	else c = st->req_realpath;

	if (strcmp(c, st->map_file) == MATCH)
		die(st, ERR_ACCESS, "Refusing to serve out a gophermap file");
	if (strcmp(c, st->tag_file) == MATCH)	
		die(st, ERR_ACCESS, "Refusing to serve out a gophertag file");

	/* Check for & run CGI and query scripts */
	if (strstr(st->req_realpath, st->cgi_file) || st->req_filetype == TYPE_QUERY)
		run_cgi(st, st->req_realpath, NULL);

	/* Check for a file suffix filter */
	if (*st->filter_dir && (c = strrchr(st->req_realpath, '.'))) {
		snprintf(buf, sizeof(buf), "%s/%s", st->filter_dir, c + 1);

		/* Filter file through the script */
		if (stat(buf, &file) == OK && (file.st_mode & S_IXOTH))
			run_cgi(st, buf, st->req_realpath);
	}

	/* Check for a filetype filter */
	if (*st->filter_dir) {
		snprintf(buf, sizeof(buf), "%s/%c", st->filter_dir, st->req_filetype);

		/* Filter file through the script */
		if (stat(buf, &file) == OK && (file.st_mode & S_IXOTH))
			run_cgi(st, buf, st->req_realpath);
	}

	/* Output regular files */
	if (st->req_filetype == TYPE_TEXT || st->req_filetype == TYPE_MIME)
		send_text_file(st);
	else
		send_binary_file(st);
}


