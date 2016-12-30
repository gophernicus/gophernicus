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
 * Alphabetic folders first sort for sortdir()
 */
int foldersort(const void *a, const void *b)
{
	mode_t amode;
	mode_t bmode;

	amode = (*(sdirent *) a).mode & S_IFMT;
	bmode = (*(sdirent *) b).mode & S_IFMT;

	if (amode == S_IFDIR && bmode != S_IFDIR) return -1;
	if (amode != S_IFDIR && bmode == S_IFDIR) return 1;

	return strcmp((*(sdirent *) a).name, (*(sdirent *) b).name);
}


/*
 * Scan, stat and sort a directory folders first (scandir replacement)
 */
int sortdir(char *path, sdirent *list, int max)
{
	DIR *dp;
	struct dirent *d;
	struct stat s;
	char buf[BUFSIZE];
	int i;

	/* Try to open the dir */
	if ((dp = opendir(path)) == NULL) return 0;
	i = 0;

	/* Loop through the directory & stat() everything */
	while (max--) {
		if ((d = readdir(dp)) == NULL) break;

		snprintf(buf, sizeof(buf), "%s/%s", path, d->d_name);
		if (stat(buf, &s) == ERROR) continue;

		if (strlen(d->d_name) > sizeof(list[i].name)) continue;
		sstrlcpy(list[i].name, d->d_name);

		list[i].mode  = s.st_mode;
		list[i].uid   = s.st_uid;
		list[i].gid   = s.st_gid;
		list[i].size  = s.st_size;
		list[i].mtime = s.st_mtime;
		i++;
	}
	closedir(dp);

	/* Sort the entries */
	if (i > 1) qsort(list, i, sizeof(sdirent), foldersort);

	/* Return number of entries found */
	return i;
}


/*
 * Print a list of users with ~/public_gopher
 */
#ifdef HAVE_PASSWD
void userlist(state *st)
{
	struct passwd *pwd;
	struct stat dir;
	char buf[BUFSIZE];
	struct tm *ltime;
	char timestr[20];
	int width;

	/* Width of filenames for fancy listing */
	width = st->out_width - DATE_WIDTH - 15;

	/* Loop through all users */
	setpwent();
	while ((pwd = getpwent())) {

		/* Skip too small uids */
		if (pwd->pw_uid < PASSWD_MIN_UID) continue;

		/* Look for a world-readable user-owned ~/public_gopher */
		snprintf(buf, sizeof(buf), "%s/%s", pwd->pw_dir, st->user_dir);
		if (stat(buf, &dir) == ERROR) continue;
		if ((dir.st_mode & S_IROTH) == 0) continue;
		if (dir.st_uid != pwd->pw_uid) continue;

		/* Found one */
		snprintf(buf, sizeof(buf), USERDIR_FORMAT);

		if (st->opt_date) {
			ltime = localtime(&dir.st_mtime);
			strftime(timestr, sizeof(timestr), DATE_FORMAT, ltime);

			printf("1%-*.*s   %s        -  \t/~%s/\t%s\t%i" CRLF,
				width, width, buf, timestr, pwd->pw_name,
				st->server_host, st->server_port);
		}
		else {
			printf("1%.*s\t/~%s/\t%s\t%i" CRLF, st->out_width, buf,
				pwd->pw_name, st->server_host_default, st->server_port);
		}
	}

	endpwent();
}
#endif


/*
 * Print a list of available virtual hosts
 */
void vhostlist(state *st)
{
	sdirent dir[MAX_SDIRENT];
	struct tm *ltime;
	char timestr[20];
	char buf[BUFSIZE];
	int width;
	int num;
	int i;

	/* Scan the root dir for vhost dirs */
	num = sortdir(st->server_root, dir, MAX_SDIRENT);
	if (num < 0) die(st, ERR_NOTFOUND, "WTF?");

	/* Width of filenames for fancy listing */
	width = st->out_width - DATE_WIDTH - 15;

	/* Loop through the directory entries */
	for (i = 0; i < num; i++) {

		/* Skip dotfiles */
		if (dir[i].name[0] == '.') continue;

		/* Require FQDN */
		if (!strchr(dir[i].name, '.')) continue;

		/* We only want world-readable directories */
		if ((dir[i].mode & S_IROTH) == 0) continue;
		if ((dir[i].mode & S_IFMT) != S_IFDIR) continue;

		/* Generate display string for vhost */
		snprintf(buf, sizeof(buf), VHOST_FORMAT, dir[i].name);

		/* Fancy listing */
		if (st->opt_date) {
			ltime = localtime(&dir[i].mtime);
			strftime(timestr, sizeof(timestr), DATE_FORMAT, ltime);

			printf("1%-*.*s   %s        -  \t/;%s\t%s\t%i" CRLF,
				width, width, buf, timestr, dir[i].name, 
				dir[i].name, st->server_port);
		}

		/* Teh boring version */
		else {
			printf("1%.*s\t/;%s\t%s\t%i" CRLF, st->out_width, buf,
				dir[i].name, dir[i].name, st->server_port);
		}
	}
}


/*
 * Return gopher filetype for a file
 */
char gopher_filetype(state *st, char *file, char magic)
{
	FILE *fp;
	char buf[BUFSIZE];
	char *c;
	int i;

	/* If it ends with an slash it's a menu */
	if (!*file) return st->default_filetype;
	if (strlast(file) == '/') return TYPE_MENU;

	/* Get file suffix */
	if ((c = strrchr(file, '.'))) {
		c++;

		/* Loop through the filetype array looking for a match*/
		for (i = 0; i < st->filetype_count; i++)
			if (strcasecmp(st->filetype[i].suffix, c) == MATCH)
				return st->filetype[i].type;
	}

	/* Are we allowed to look inside files? */
	if (!magic) return st->default_filetype;

	/* Read data from the file */
	if ((fp = fopen(file , "r")) == NULL) return st->default_filetype;
	i = fread(buf, 1, sizeof(buf) - 1, fp);
	buf[i] = '\0';
	fclose(fp);

	/* GIF images */
	if (sstrncmp(buf, "GIF89a") == MATCH || 
	    sstrncmp(buf, "GIF87a") == MATCH) return TYPE_GIF;

	/* JPEG images */
	if (sstrncmp(buf, "\377\330\377\340") == MATCH) return TYPE_IMAGE;

	/* PNG images */
	if (sstrncmp(buf, "\211PNG") == MATCH) return TYPE_IMAGE;

	/* mbox */
	if (strstr(buf, "\nFrom: ") &&
	    strstr(buf, "\nSubject: ")) return TYPE_MIME;

	/* MIME */
	if (strstr(buf, "\nContent-Type: ")) return TYPE_MIME;

	/* HTML files */
	if (buf[0] == '<' &&
	    (strstr(buf, "<html") ||
	     strstr(buf, "<HTML"))) return TYPE_HTML;

	/* PDF and PostScript */
	if (sstrncmp(buf, "%PDF-") == MATCH ||
	    sstrncmp(buf, "%!") == MATCH) return TYPE_DOC;

	/* compress and gzip */
	if (sstrncmp(buf, "\037\235\220") == MATCH ||
	    sstrncmp(buf, "\037\213\010") == MATCH) return TYPE_GZIP;

	/* Unknown content - binary or text? */
	if (memchr(buf, '\0', i)) return TYPE_BINARY;
	return st->default_filetype;
}


/*
 * Handle gophermaps
 */
int gophermap(state *st, char *mapfile, int depth)
{
	FILE *fp;
	struct stat file;
	char line[BUFSIZE];
#ifdef HAVE_POPEN
	char command[BUFSIZE];
#endif
	char *selector;
	char *name;
	char *host;
	char *c;
	char type;
	int port;
	int exe;

	/* Prevent include loops */
	if (depth > 4) return OK;

	/* Try to figure out whether the map is executable */
	if (stat(mapfile, &file) == OK) {
		if ((file.st_mode & S_IXOTH)) {
#ifdef HAVE_POPEN
			/* Quote the command in case path has spaces */
			snprintf(command, sizeof(command), "'%s'", mapfile);
#endif
			exe = TRUE;
		}
		else exe = FALSE;
	}

	/* This must be a shell include */
	else {
#ifdef HAVE_POPEN
		/* Let's assume the shell command runs as is without quoting */
		sstrlcpy(command, mapfile);
#endif
		exe = TRUE;
	}

	/* Debug output */
	if (st->debug) {
		if (exe) syslog(LOG_INFO, "parsing executable gophermap \"%s\"", mapfile);
		else syslog(LOG_INFO, "parsing static gophermap \"%s\"", mapfile);
	}

	/* Try to execute or open the mapfile */
#ifdef HAVE_POPEN
	if (exe) {
		setenv_cgi(st, mapfile);
		if ((fp = popen(command, "r")) == NULL) return OK;
	}
	else
#endif
		if ((fp = fopen(mapfile, "r")) == NULL) return OK;

	/* Read lines one by one */
	while (fgets(line, sizeof(line) - 1, fp)) {

		/* Parse type & name */
		chomp(line);
		type = line[0];
		name = line + 1;

		/* Ignore #comments */
		if (type == '#') continue;

		/* Stop handling gophermap? */
		if (type == '*') return OK;
		if (type == '.') return QUIT;

		/* Print a list of users with public_gopher */
		if (type == '~') {
#ifdef HAVE_PASSWD
			userlist(st);
#endif
			continue;
		}

		/* Print a list of available virtual hosts */
		if (type == '%') {
			if (st->opt_vhost) vhostlist(st);
			continue;
		}

		/* Hide files in menus */
		if (type == '-') {
			if (st->hidden_count < MAX_HIDDEN)
				sstrlcpy(st->hidden[st->hidden_count++], name);
			continue;
		}

		/* Override filetype mappings */
		if (type == ':') {
			add_ftype_mapping(st, name);
			continue;
		}

		/* Include gophermap or shell exec */
		if (type == '=') {
			gophermap(st, name, depth + 1);
			continue;
		}

		/* Title resource */
		if (type == TYPE_TITLE) {
			info(st, name, TYPE_TITLE);
			continue;
		}

		/* Print out non-resources as info text */
		if (!strchr(line, '\t')) {
			info(st, line, TYPE_INFO);
			continue;
		}

		/* Parse selector */
		selector = EMPTY;
		if ((c = strchr(name, '\t'))) {
			*c = '\0';
			selector = c + 1;
		}
		if (!*selector) selector = name;

		/* Parse host */
		host = st->server_host;
		if ((c = strchr(selector, '\t'))) {
			*c = '\0';
			host = c + 1;
		}

		/* Parse port */
		port = st->server_port;
		if ((c = strchr(host, '\t'))) {
			*c = '\0'; 
			port = atoi(c + 1);
		}

		/* Handle remote, absolute and hURL gopher resources */
		if (sstrncmp(selector, "URL:") == MATCH ||
		    selector[0] == '/' ||
		    host != st->server_host) {

			printf("%c%s\t%s\t%s\t%i" CRLF, type, name,
				selector, host, port);
		}

		/* Handle relative resources */
		else {
			printf("%c%s\t%s%s\t%s\t%i" CRLF, type, name,
				st->req_selector, selector, host, port);

			/* Automatically hide manually defined selectors */
#ifdef ENABLE_AUTOHIDING
			if (st->hidden_count < MAX_HIDDEN)
				sstrlcpy(st->hidden[st->hidden_count++], selector);
#endif
		}
	}

	/* Clean up & return */
#ifdef HAVE_POPEN
	if (exe) pclose(fp);
	else
#endif
		fclose(fp);

	return QUIT;
}


/*
 * Handle gopher menus
 */
void gopher_menu(state *st)
{
	FILE *fp;
	sdirent dir[MAX_SDIRENT];
	struct tm *ltime;
	struct stat file;
	char buf[BUFSIZE];
	char pathname[BUFSIZE];
	char displayname[BUFSIZE];
	char encodedname[BUFSIZE];
	char timestr[20];
	char sizestr[20];
	char *parent;
	char *c;
	char type;
	int width;
	int num;
	int i;
	int n;

	/* Check for a gophermap */
	snprintf(pathname, sizeof(pathname), "%s/%s",
		st->req_realpath, st->map_file);

	if (stat(pathname, &file) == OK &&
	    (file.st_mode & S_IFMT) == S_IFREG) {

		/* Parse gophermap */
		if (gophermap(st, pathname, 0) == QUIT) {
			footer(st);
			return;
		}
	}

	else {
		/* Check for a gophertag */
		snprintf(pathname, sizeof(pathname), "%s/%s",
			st->req_realpath, st->tag_file);

		if (stat(pathname, &file) == OK &&
		    (file.st_mode & S_IFMT) == S_IFREG) {

			/* Read & output gophertag */
			if ((fp = fopen(pathname , "r"))) {

				fgets(buf, sizeof(buf), fp);
				chomp(buf);

				info(st, buf, TYPE_TITLE);
				info(st, EMPTY, TYPE_INFO);
				fclose(fp);
			}
		}

		/* No gophermap or tag found - print default header */
		else if (st->opt_header) {

			/* Use the selector as menu title */
			sstrlcpy(displayname, st->req_selector);

			/* Shorten too long titles */
			while (strlen(displayname) > (st->out_width - sizeof(HEADER_FORMAT))) {
				if ((c = strchr(displayname, '/')) == NULL) break;

				if (!*++c) break;
				sstrlcpy(displayname, c);
			}

			/* Output menu title */
			snprintf(buf, sizeof(buf), HEADER_FORMAT, displayname);
			info(st, buf, TYPE_TITLE);
			info(st, EMPTY, TYPE_INFO);
		}
	}

	/* Scan the directory */
	num = sortdir(st->req_realpath, dir, MAX_SDIRENT);
	if (num < 0) die(st, ERR_NOTFOUND, "WTF?");

	/* Create link to parent directory */
	if (st->opt_parent) {
		sstrlcpy(buf, st->req_selector);
		parent = dirname(buf);

		/* Root has no parent */
		if (strcmp(st->req_selector, ROOT) != MATCH) {

			/* Prevent double-slash */
			if (strcmp(parent, ROOT) == MATCH) parent++;

			/* Print link */
			printf("1%-*s\t%s/\t%s\t%i" CRLF,
				st->opt_date ? (st->out_width - 1) : (int) strlen(PARENT),
				PARENT, parent, st->server_host, st->server_port);
		}
	}

	/* Width of filenames for fancy listing */
	width = st->out_width - DATE_WIDTH - 15;

	/* Loop through the directory entries */
	for (i = 0; i < num; i++) {

		/* Get full path+name */
		snprintf(pathname, sizeof(pathname), "%s/%s",
			st->req_realpath, dir[i].name);

		/* Skip dotfiles and non world-readables */
		if (dir[i].name[0] == '.') continue;
		if ((dir[i].mode & S_IROTH) == 0) continue;

		/* Skip gophermaps and tags (but not dirs) */
		if ((dir[i].mode & S_IFMT) != S_IFDIR) {
			if (strcmp(dir[i].name, st->map_file) == MATCH) continue;
			if (strcmp(dir[i].name, st->tag_file) == MATCH) continue;
		}

		/* Skip files marked for hiding */
		for (n = 0; n < st->hidden_count; n++)
			if (strcmp(dir[i].name, st->hidden[n]) == MATCH) break;
		if (n < st->hidden_count) continue;	/* Cruel hack... */

		/* Generate display name with correct output charset */
		if (st->opt_iconv)
			sstrniconv(st->out_charset, displayname, dir[i].name);
		else
			sstrlcpy(displayname, dir[i].name);

		/* #OCT-encode filename */
		strnencode(encodedname, dir[i].name, sizeof(encodedname));

		/* Handle inline .gophermap */
		if (strstr(displayname, st->map_file) > displayname) {
			gophermap(st, pathname, 0);
			continue;
		}

		/* Handle directories */
		if ((dir[i].mode & S_IFMT) == S_IFDIR) {

			/* Check for a gophertag */
			snprintf(buf, sizeof(buf), "%s/%s",
				pathname, st->tag_file);

			if (stat(buf, &file) == OK &&
			    (file.st_mode & S_IFMT) == S_IFREG) {

				/* Use the gophertag as displayname */
				if ((fp = fopen(buf , "r"))) {

					fgets(buf, sizeof(buf), fp);
					chomp(buf);
					fclose(fp);

					/* Skip empty gophertags */
					if (*buf) {

						/* Convert to output charset */
						if (st->opt_iconv) sstrniconv(st->out_charset, displayname, buf);
						else sstrlcpy(displayname, buf);
					}

				}
			}

			/* Dir listing with dates */
			if (st->opt_date) {
				ltime = localtime(&dir[i].mtime);
				strftime(timestr, sizeof(timestr), DATE_FORMAT, ltime);

				/* Hack to get around UTF-8 byte != char */
				n = width - strcut(displayname, width);
				strrepeat(buf, ' ', n);

				printf("1%s%s   %s        -  \t%s%s/\t%s\t%i" CRLF,
					displayname,
					buf,
					timestr,
					st->req_selector,
					encodedname,
					st->server_host,
					st->server_port);
			}

			/* Regular dir listing */
			else {
				strcut(displayname, st->out_width);
				printf("1%s\t%s%s/\t%s\t%i" CRLF,
					displayname,
					st->req_selector,
					encodedname,
					st->server_host,
					st->server_port);
			}

			continue;
		}

		/* Skip special files (sockets, fifos etc) */
		if ((dir[i].mode & S_IFMT) != S_IFREG) continue;

		/* Get file type */
		type = gopher_filetype(st, pathname, st->opt_magic);

		/* File listing with dates & sizes */
		if (st->opt_date) {
			ltime = localtime(&dir[i].mtime);
			strftime(timestr, sizeof(timestr), DATE_FORMAT, ltime);
			strfsize(sizestr, dir[i].size, sizeof(sizestr));

			/* Hack to get around UTF-8 byte != char */
			n = width - strcut(displayname, width);
			strrepeat(buf, ' ', n);

			printf("%c%s%s   %s %s\t%s%s\t%s\t%i" CRLF, type,
				displayname,
				buf,
				timestr,
				sizestr,
				st->req_selector,
				encodedname,
				st->server_host,
				st->server_port);
		}

		/* Regular file listing */
		else {
			strcut(displayname, st->out_width);
			printf("%c%s\t%s%s\t%s\t%i" CRLF, type,
				displayname,
				st->req_selector,
				encodedname,
				st->server_host,
				st->server_port);
		}
	}

	/* Print footer */
	footer(st);
}

