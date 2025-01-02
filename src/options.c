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
 * Add one suffix->filetype mapping to the filetypes array
 */
void add_ftype_mapping(state *st, char *suffix)
{
	char *type;
	int i;

	/* Let's not do anything stupid */
	if (!*suffix) return;
	if (!(type = strchr(suffix, '='))) return;

	/* Extract type from the suffix=X string */
	*type++ = '\0';
	if (!*type) return;

	/* Loop through the filetype array */
	for (i = 0; i < st->filetype_count; i++) {

		/* Old entry found? */
		if (strcasecmp(st->filetype[i].suffix, suffix) == MATCH) {
			st->filetype[i].type = *type;
			return;
		}
	}

	/* No old entry found - add new entry */
	if (i < MAX_FILETYPES) {
		sstrlcpy(st->filetype[i].suffix, suffix);
		st->filetype[i].type = *type;
		st->filetype_count++;
	}
}


/*
 * Add one selector rewrite mapping to the array
 */
static void add_rewrite_mapping(state *st, char *match)
{
	char *replace;

	/* Check input and split it into match & replace */
	if (!*match) return;
	if (!(replace = strchr(match, '='))) return;

	*replace++ = '\0';
	if (!*replace) return;

	/* Insert match/replace values into the array */
	if (st->rewrite_count < MAX_REWRITE) {
		sstrlcpy(st->rewrite[st->rewrite_count].match, match);
		sstrlcpy(st->rewrite[st->rewrite_count].replace, replace);
		st->rewrite_count++;
	}
}


/*
 * Parse command-line arguments
 */
void parse_args(state *st, int argc, char *argv[])
{
	FILE *fp;
	static const char readme[] = README;
	static const char license[] = LICENSE;
	struct stat file;
	char buf[BUFSIZE];
	int opt;

	/* Parse args */
	while ((opt = getopt(argc, argv,
#ifdef __OpenBSD__
		"U:" /* extra unveil(2) paths are OpenBSD only */
#endif
		"h:p:T:r:t:g:a:c:u:m:l:w:o:s:i:k:f:e:R:D:L:A:P:n:dbv?-")) != ERROR) {
		switch(opt) {
			case 'h': sstrlcpy(st->server_host, optarg); break;
			case 'p': st->server_port = atoi(optarg); break;
			case 'T': st->server_tls_port = atoi(optarg); break;
			case 'r': sstrlcpy(st->server_root, optarg); break;
			case 't': st->default_filetype = *optarg; break;
			case 'g': sstrlcpy(st->map_file, optarg); break;
			case 'a': sstrlcpy(st->tag_file, optarg); break;
			case 'c': sstrlcpy(st->cgi_file, optarg); break;
			case 'u': sstrlcpy(st->user_dir, optarg);  break;
			case 'm': /* obsolete, replaced by -l */
			case 'l': sstrlcpy(st->log_file, optarg);  break;

			case 'w': st->out_width = atoi(optarg); break;
			case 'o':
				if (sstrncasecmp(optarg, "UTF-8") == MATCH) st->out_charset = UTF_8;
				if (sstrncasecmp(optarg, "US-ASCII") == MATCH) st->out_charset = US_ASCII;
				if (sstrncasecmp(optarg, "ISO-8859-1") == MATCH) st->out_charset = ISO_8859_1;
				break;

			case 's': st->session_timeout = atoi(optarg); break;
			case 'i': st->session_max_kbytes = abs(atoi(optarg)); break;
			case 'k': st->session_max_hits = abs(atoi(optarg)); break;

			case 'f': sstrlcpy(st->filter_dir, optarg); break;
			case 'e': add_ftype_mapping(st, optarg); break;

			case 'R': add_rewrite_mapping(st, optarg); break;
			case 'D': sstrlcpy(st->server_description, optarg); break;
			case 'L': sstrlcpy(st->server_location, optarg); break;
			case 'A': sstrlcpy(st->server_admin, optarg); break;
#ifdef __OpenBSD__
			case 'U': st->extra_unveil_paths = optarg; break;
#endif
			case 'n':
				if (*optarg == 'v') { st->opt_vhost = FALSE; break; }
				if (*optarg == 'l') { st->opt_parent = FALSE; break; }
				if (*optarg == 'h') { st->opt_header = FALSE; break; }
				if (*optarg == 'f') { st->opt_footer = FALSE; break; }
				if (*optarg == 'd') { st->opt_date = FALSE; break; }
				if (*optarg == 'c') { st->opt_magic = FALSE; break; }
				if (*optarg == 'o') { st->opt_iconv = FALSE; break; }
				if (*optarg == 'q') { st->opt_query = FALSE; break; }
				if (*optarg == 's') { st->opt_syslog = FALSE; break; }
				if (*optarg == 'a') { st->opt_caps = FALSE; break; }
				if (*optarg == 't') { st->opt_status = FALSE; break; }
				if (*optarg == 'm') { st->opt_shm = FALSE; break; }
				if (*optarg == 'r') { st->opt_root = FALSE; break; }
				if (*optarg == 'p') { st->opt_proxy = FALSE; break; }
				if (*optarg == 'x') { st->opt_exec = FALSE; break; }
				if (*optarg == 'u') { st->opt_personal_spaces = FALSE; break; }
				if (*optarg == 'H') { st->opt_http_requests = FALSE; break; }
				if (*optarg == 'g') { st->opt_plus_menu = FALSE; break; }
				break;

			case 'd': st->debug = TRUE; break;
			case 'b': puts(license); exit(EXIT_SUCCESS);

			case 'v':
				printf("%s/%s \"%s\" (built %s)\n", SERVER_SOFTWARE, VERSION, CODENAME, __DATE__);
				exit(EXIT_SUCCESS);

			default : puts(readme); exit(EXIT_SUCCESS);
		}
	}

	/* Sanitize options */
	if (st->out_width > MAX_WIDTH) st->out_width = MAX_WIDTH;
	if (st->out_width < MIN_WIDTH) st->out_width = MIN_WIDTH;
	if (st->out_width < MIN_WIDTH + DATE_WIDTH) st->opt_date = FALSE;
	if (!st->opt_syslog) st->debug = FALSE;

	/* Primary vhost directory must exist or we disable vhosting */
	if (st->opt_vhost) {
		snprintf(buf, sizeof(buf), "%s/%s", st->server_root, st->server_host);
		if (stat(buf, &file) == ERROR) st->opt_vhost = FALSE;
	}

	/* If -D arg looks like a file load the file contents */
	if (*st->server_description == '/') {

		if ((fp = fopen(st->server_description , "r"))) {
			if (fgets(st->server_description, sizeof(st->server_description), fp) == NULL)
				strclear(st->server_description);

			chomp(st->server_description);
			fclose(fp);
		}
		else strclear(st->server_description);
	}

	/* If -L arg looks like a file load the file contents */
	if (*st->server_location == '/') {

		if ((fp = fopen(st->server_location , "r"))) {
			if (fgets(st->server_location, sizeof(st->server_location), fp) == NULL)
				strclear(st->server_description);

			chomp(st->server_location);
			fclose(fp);
		}
		else strclear(st->server_location);
	}
}
