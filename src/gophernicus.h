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

#ifndef _GOPHERNICUS_H
#define _GOPHERNICUS_H


/*
 * Ignore useless snprintf() format truncation warnings on GCC 7+
 */
#if __GNUC__ >= 7
#pragma GCC diagnostic ignored "-Wformat-truncation"
#endif


/*
 * Features
 */
#undef  ENABLE_STRICT_RFC1436    /* Follow RFC1436 to the letter */
#undef  ENABLE_AUTOHIDING    /* Hide manually listed resources from generated menus */
#define ENABLE_HAPROXY1        /* Autodetect HAproxy/Stunnel proxy protocol v1 */


/*
 * Platform configuration
 */

/* Defaults should fit standard POSIX systems */
#define HAVE_IPv4        /* IPv4 should work anywhere */
#define HAVE_IPv6        /* Requires modern POSIX */
/* #define HAVE_PASSWD        autodetected, For systems with passwd-like userdb */
#define PASSWD_MIN_UID 100    /* Minimum allowed UID for ~userdirs */
#define HAVE_LOCALES        /* setlocale() and friends */
#define HAVE_SHMEM        /* Shared memory support */
#define HAVE_UNAME        /* uname() */
#define HAVE_POPEN        /* popen() */
#undef  HAVE_STRLCPY        /* strlcpy() from OpenBSD */
#undef  HAVE_SENDFILE        /* sendfile() in Linux & others */
/* #undef  HAVE_LIBWRAP           autodetected, don't enable here */

#include "../config.h"

/* Linux */
#ifdef __linux
#undef  PASSWD_MIN_UID
#define PASSWD_MIN_UID 500
#define _FILE_OFFSET_BITS 64
#endif

/* Embedded Linux with uClibc */
#ifdef __UCLIBC__
#undef HAVE_SHMEM
#undef HAVE_PASSWD
#endif

/* Haiku */
#ifdef __HAIKU__
#undef HAVE_SHMEM
#undef HAVE_PASSWD
#endif

/* OpenBSD */
#ifdef __OpenBSD__
#define HAVE_STRLCPY
#endif

/* MacOS */
#if defined(__APPLE__)
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= __MAC_10_10
#define HAVE_STRLCPY
#endif
#endif

/* AIX */
#if defined(_AIX)
#define _LARGE_FILES 1
#endif

/* Add other OS-specific defines here */

/*
 * Include headers
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <libgen.h>
#include <time.h>
#include <errno.h>
#include <pwd.h>
#include <limits.h>

#ifdef HAVE_SENDFILE
#include <sys/sendfile.h>
#include <fcntl.h>
#endif

#ifdef HAVE_LOCALES
#include <locale.h>
#endif

#ifdef HAVE_SHMEM
#include <sys/ipc.h>
#include <sys/shm.h>
#else
#define shm_state void
#endif

#if defined(HAVE_IPv4) || defined(HAVE_IPv6)
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#ifdef HAVE_UNAME
#include <sys/utsname.h>
#endif

#if !defined(HAVE_STRLCPY)
size_t strlcpy(char *dst, const char *src, size_t siz);
size_t strlcat(char *dst, const char *src, size_t siz);
#endif

#ifdef HAVE_LIBWRAP
#include <tcpd.h>
#endif

/*
 * Compile-time configuration
 */

/* Common stuff */
#define CRLF        "\r\n"
#define EMPTY        ""
#define PARENT        ".."
#define ROOT        "/"

#define FALSE        0
#define TRUE        1

#define QUIT        1
#define OK        0
#define ERROR        -1

#define MATCH        0
#define WRAP_DENIED    0


/* Gopher filetypes */
#define TYPE_TEXT    '0'
#define TYPE_MENU    '1'
#define TYPE_ERROR    '3'
#define TYPE_GZIP    '5'
#define TYPE_QUERY    '7'
#define TYPE_BINARY    '9'
#define TYPE_GIF    'g'
#define TYPE_HTML    'h'
#define TYPE_INFO    'i'
#define TYPE_IMAGE    'I'
#define TYPE_MIME    'M'
#define TYPE_DOC    'd'
#define TYPE_TITLE    '!'

/* Protocols */
#define PROTO_GOPHER    'g'
#define PROTO_HTTP    'h'

/* Charsets */
#define AUTO        0
#define US_ASCII    1
#define ISO_8859_1    2
#define UTF_8        3

/* HTTP protocol stuff for logging */
#define HTTP_OK        200
#define HTTP_404    404
#define HTTP_DATE    "%d/%b/%Y:%T %z"
#define HTTP_USERAGENT    "Unknown gopher client"

/* Defaults for settings */
#define DEFAULT_HOST		"localhost"
#define DEFAULT_PORT		70
#define DEFAULT_TLS_PORT	0
#define DEFAULT_TYPE		TYPE_TEXT
#define DEFAULT_MAP		"gophermap"
#define DEFAULT_TAG		"gophertag"
#define DEFAULT_CGI		"/cgi-bin/"
#define DEFAULT_USERDIR		"public_gopher"
#define DEFAULT_WIDTH       67
#define DEFAULT_CHARSET		UTF_8
#define MIN_WIDTH		33
#define MAX_WIDTH		200
#define UNKNOWN_ADDR		"unknown"

/* Session defaults */
#define DEFAULT_SESSION_TIMEOUT        1800
#define DEFAULT_SESSION_MAX_KBYTES    4194304
#define DEFAULT_SESSION_MAX_HITS    4096

/* Dummy values for gopher protocol */
#define DUMMY_SELECTOR    "null"
#define DUMMY_HOST    "null.host\t1"

/* Safe $PATH for exec() */
#ifdef __HAIKU__
#define SAFE_PATH    "/boot/common/bin:/bin"
#else
#define SAFE_PATH    "/usr/bin:/bin"
#endif

/* Special requests */
#define SERVER_STATUS    "/server-status"
#define CAPS_TXT    "/caps.txt"

/* Error messages */
#define ERR_ACCESS    "Access denied!"
#define ERR_NOTFOUND    "File or directory not found!"

#define ERROR_HOST    "error.host\t1"
#define ERROR_PREFIX    "Error: "

/* Strings */
#define SERVER_SOFTWARE    "Gophernicus"
#define SERVER_SOFTWARE_FULL SERVER_SOFTWARE "/" VERSION " (%s)"
#define PROGNAME             "gophernicus"

#define HEADER_FORMAT    "[%s]"
#define FOOTER_FORMAT    "Gophered by Gophernicus/" VERSION " on %s"

#define UNITS        "KB", "MB", "GB", "TB", "PB", NULL
#define DATE_FORMAT    "%Y-%b-%d %H:%M"    /* See man 3 strftime */
#define DATE_WIDTH    17
#define DATE_LOCALE     "POSIX"

#define USERDIR_FORMAT    "~%s", users[i].user    /* See man 3 getpwent */
#define VHOST_FORMAT    "gopher://%s/"

/* ISO-8859-1 to US-ASCII look-alike conversion table */
#define ASCII \
    "E?,f..++^%S<??Z?" \
    "?''\"\"*--~?s>??zY" \
    " !c_*Y|$\"C?<?-R-" \
    "??23'u?*,1?>????" \
    "AAAAAAACEEEEIIII" \
    "DNOOOOO*OUUUUYTB" \
    "aaaaaaaceeeeiiii" \
    "dnooooo/ouuuuyty"

#define UNKNOWN '?'

/* Sizes & maximums */
#define BUFSIZE        1024    /* Default size for string buffers */
#define MAX_HIDDEN    32    /* Maximum number of hidden files */
#define MAX_FILETYPES    1024    /* Maximum number of suffix to filetype mappings */
#define MAX_FILTERS    16    /* Maximum number of file filters */
#define MAX_SDIRENT    1024    /* Maximum number of files per directory to handle */
#define MAX_REWRITE    32    /* Maximum number of selector rewrite options */
#define MAX_USERS    1024 /* Maximum number of users for the ~ option */

/* Struct for file suffix -> gopher filetype mapping */
typedef struct {
    char suffix[15];
    char type;
} ftype;

/* Struct for selector rewriting */
typedef struct {
    char match[BUFSIZE];
    char replace[BUFSIZE];
} srewrite;

/* Struct for keeping the current options & state */
typedef struct {

    /* Request */
    char req_selector[BUFSIZE];
    char req_realpath[BUFSIZE];
    char req_query_string[BUFSIZE];
    char req_search[BUFSIZE];
    char req_referrer[BUFSIZE];
    char req_local_addr[64];
    char req_remote_addr[64];
    char req_filetype;
    char req_protocol;
    off_t req_filesize;

    /* Output */
    int out_width;
    int out_charset;

    /* Settings */
    char server_description[64];
    char server_location[64];
    char server_platform[64];
    char server_admin[64];
    char server_root[256];
    char server_host_default[64];
    char server_host[64];
    int  server_port;
    int  server_tls_port;

    char default_filetype;
    char map_file[64];
    char tag_file[64];
    char cgi_file[64];
    char user_dir[64];
    char log_file[256];

    char hidden[MAX_HIDDEN][256];
    int hidden_count;

    ftype filetype[MAX_FILETYPES];
    int filetype_count;
    char filter_dir[64];

    srewrite rewrite[MAX_REWRITE];
    int rewrite_count;

#ifdef __OpenBSD__
	char *extra_unveil_paths;
#endif

    /* Session */
    int session_timeout;
    int session_max_kbytes;
    int session_max_hits;
    int session_id;

    /* Feature options */
    char opt_parent;
    char opt_header;
    char opt_footer;
    char opt_date;
    char opt_syslog;
    char opt_magic;
    char opt_iconv;
    char opt_vhost;
    char opt_query;
    char opt_caps;
    char opt_status;
    char opt_shm;
    char opt_root;
    char opt_proxy;
    char opt_exec;
    char opt_personal_spaces;
    char opt_http_requests;
    char opt_plus_menu;
    char debug;
} state;

/* Shared memory for session & accounting data */
#ifdef HAVE_SHMEM

#define SHM_KEY        0xbeeb0008    /* Unique identifier + struct version */
#define SHM_MODE    0600        /* Access mode for the shared memory */
#define SHM_SESSIONS    256        /* Max amount of user sessions to track */

typedef struct {
    long hits;
    long kbytes;

    time_t req_atime;
    char req_selector[128];
    char req_remote_addr[64];
    char req_filetype;
    int session_id;

    char server_host[64];
    int  server_port;
} shm_session;

typedef struct {
    time_t start_time;
    long hits;
    long kbytes;
    char server_platform[64];
    char server_description[64];
    shm_session session[SHM_SESSIONS];
} shm_state;

#endif

/* Struct for directory sorting */
typedef struct {
    char    name[128];    /* Should be 256 but we're saving stack space */
    mode_t    mode;
    uid_t    uid;
    gid_t    gid;
    off_t    size;
    time_t    mtime;
} sdirent;

/* Struct for the userlist with date */
typedef struct {
	char   user[32]; /* Maximum in most systems */
	time_t mtime;
} user_date;

/*
 * Useful macros
 */
#define strclear(str) str[0] = '\0';
#define sstrlcpy(dest, src) strlcpy(dest, src, sizeof(dest))
#define sstrlcat(dest, src) strlcat(dest, src, sizeof(dest))
#define sstrncmp(s1, s2) strncmp(s1, s2, sizeof(s2) - 1)
#define sstrncasecmp(s1, s2) strncasecmp(s1, s2, sizeof(s2) - 1)
#define sstrniconv(charset, out, in) strniconv(charset, out, in, sizeof(out))
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))

/*
 * Include generated headers
 */
#include "files.h"
#include "filetypes.h"


/* gophernicus.c */
void info(state *st, char *str, char type);
void footer(state *st);
void die(state *st, const char *message, const char *description);
void log_combined(state *st, int status);
void html_encode(const char *unsafe, char *dest, int bufsize);

/* file.c */
void send_binary_file(state *st);
void send_text_file(state *st);
void url_redirect(state *st);
void server_status(state *st, shm_state *shm, int shmid);
void caps_txt(state *st, shm_state *shm);
void setenv_cgi(state *st, char *script);
void gopher_file(state *st);

/* menu.c */
char gopher_filetype(state *st, char *file, char magic);
void gopher_menu(state *st);

/* string.c */
void strrepeat(char *dest, char c, size_t num);
void strreplace(char *str, char from, char to);
size_t strcut(char *str, size_t width);
char *strkey(char *header, char *key);
char strlast(char *str);
void chomp(char *str);
char *strcharset(int charset);
void strniconv(int charset, char *out, char *in, size_t outsize);
void strnencode(char *out, const char *in, size_t outsize);
void strndecode(char *out, char *in, size_t outsize);
void strfsize(char *out, off_t size, size_t outsize);

/* platform.c */
void platform(state *st);
float loadavg(void);

/* session.c */
void get_shm_session(state *st, shm_state *shm);
void update_shm_session(state *st, shm_state *shm);

/* options.c */
void add_ftype_mapping(state *st, char *suffix);
void parse_args(state *st, int argc, char *argv[]);

/* log.c */
void log_init(int enable, int debug);
void log_fatal(const char *fmt, ...);
void log_warning(const char *fmt, ...);
void log_info(const char *fmt, ...);
void log_debug(const char *fmt, ...);

#endif
