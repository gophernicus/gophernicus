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
 * Locate shared memory session ID
 */
#ifdef HAVE_SHMEM
int get_shm_session_id(state *st, shm_state *shm)
{
	time_t now;
	int i;

	/* Get current time */
	now = time(NULL);

	/* Locate user's old session using remote_addr */
	for (i = 0; i < SHM_SESSIONS; i++) {
		if (strcmp(st->req_remote_addr, shm->session[i].req_remote_addr) == MATCH &&
		    (now - shm->session[i].req_atime) < st->session_timeout) break;
	}

	/* Return -1 on error */
	if (i == SHM_SESSIONS) return ERROR;
	else return i;
}
#endif


/*
 * Get shared memory session data
 */
#ifdef HAVE_SHMEM
void get_shm_session(state *st, shm_state *shm)
{
	int i;

	/* Get session id */
	if ((i = get_shm_session_id(st, shm)) == ERROR) return;

	/* Get session data */
	if (st->opt_vhost) {
		sstrlcpy(st->server_host, shm->session[i].server_host);
	}
}
#endif


/*
 * Update shared memory session data
 */
#ifdef HAVE_SHMEM
void update_shm_session(state *st, shm_state *shm)
{
	time_t now;
	char buf[BUFSIZE];
	int delay;
	int i;

	/* Get current time */
	now = time(NULL);

	/* No existing session found? */
	if ((i = get_shm_session_id(st, shm)) == ERROR) {

		/* Look for an empty/expired session slot */
		for (i = 0; i < SHM_SESSIONS; i++) {

			if ((now - shm->session[i].req_atime) > st->session_timeout) {

				/* Found slot -> initialize it */
				sstrlcpy(shm->session[i].req_remote_addr, st->req_remote_addr);
				shm->session[i].hits = 0;
				shm->session[i].kbytes = 0;
				shm->session[i].session_id = rand();
				break;
			}
		}
	}

	/* No available session slot found? */
	if (i == SHM_SESSIONS) return;

	/* Get referrer from old session data */
	if (*shm->session[i].server_host) {
		snprintf(buf, sizeof(buf), "gopher%s://%s:%i/%c%s",
			(shm->session[i].server_port == st->server_tls_port ? "s" : ""),
			shm->session[i].server_host,
			shm->session[i].server_port,
			shm->session[i].req_filetype,
			shm->session[i].req_selector);
		sstrlcpy(st->req_referrer, buf);
	}

	/* Get public session id */
	st->session_id = shm->session[i].session_id;

	/* Update session data */
	sstrlcpy(shm->session[i].server_host, st->server_host);
	shm->session[i].server_port = st->server_port;

	sstrlcpy(shm->session[i].req_selector, st->req_selector);
	shm->session[i].req_filetype = st->req_filetype;
	shm->session[i].req_atime = now;

	shm->session[i].hits++;
	shm->session[i].kbytes += st->req_filesize / 1024;

	/* Transfer limits exceeded? */
	if ((st->session_max_kbytes && shm->session[i].kbytes > st->session_max_kbytes) ||
	    (st->session_max_hits && shm->session[i].hits > st->session_max_hits)) {

		/* Calculate throttle delay */
		delay = max(shm->session[i].kbytes / st->session_max_kbytes,
			shm->session[i].hits / st->session_max_hits);

		/* Throttle user */
		syslog(LOG_INFO, "throttling user from %s for %i seconds",
			st->req_remote_addr, delay);
		sleep(delay);
	}
}
#endif

