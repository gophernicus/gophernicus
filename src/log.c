#include <stdarg.h>
#include <syslog.h>

#include "gophernicus.h"

#if defined(LOG_UPTO)
#define _LOG_UPTO LOG_UPTO
#else
#define _LOG_UPTO(pri)							\
	(								\
	((pri) == LOG_EMERG)   ? (LOG_MASK(LOG_EMERG)) :		\
	((pri) == LOG_ALERT)   ? (LOG_MASK(LOG_EMERG) | LOG_MASK(LOG_ALERT)) : \
	((pri) == LOG_CRIT)    ? (LOG_MASK(LOG_EMERG) | LOG_MASK(LOG_ALERT) | LOG_MASK(LOG_CRIT)) : \
	((pri) == LOG_ERR)     ? (LOG_MASK(LOG_EMERG) | LOG_MASK(LOG_ALERT) | LOG_MASK(LOG_CRIT) | LOG_MASK(LOG_ERR)) : \
	((pri) == LOG_WARNING) ? (LOG_MASK(LOG_EMERG) | LOG_MASK(LOG_ALERT) | LOG_MASK(LOG_CRIT) | LOG_MASK(LOG_ERR) | LOG_MASK(LOG_WARNING)) : \
	((pri) == LOG_NOTICE)  ? (LOG_MASK(LOG_EMERG) | LOG_MASK(LOG_ALERT) | LOG_MASK(LOG_CRIT) | LOG_MASK(LOG_ERR) | LOG_MASK(LOG_WARNING) | LOG_MASK(LOG_NOTICE)) : \
	((pri) == LOG_INFO)    ? (LOG_MASK(LOG_EMERG) | LOG_MASK(LOG_ALERT) | LOG_MASK(LOG_CRIT) | LOG_MASK(LOG_ERR) | LOG_MASK(LOG_WARNING) | LOG_MASK(LOG_NOTICE) | LOG_MASK(LOG_INFO)) : \
	((pri) == LOG_DEBUG)   ? (LOG_MASK(LOG_EMERG) | LOG_MASK(LOG_ALERT) | LOG_MASK(LOG_CRIT) | LOG_MASK(LOG_ERR) | LOG_MASK(LOG_WARNING) | LOG_MASK(LOG_NOTICE) | LOG_MASK(LOG_INFO) | LOG_MASK(LOG_DEBUG)) : \
	0)
#endif

static int _enable = 0;

void log_init(int enable, int debug)
{
	if (!enable) return;

	_enable = enable;

	openlog(PROGNAME, LOG_PID, LOG_DAEMON);

	setlogmask(_LOG_UPTO(debug ? LOG_DEBUG : LOG_INFO));
}

static void _vlog(int priority, const char *fmt, va_list ap)
{
	char buf[BUFSIZE];

	if (!_enable) return;

	vsnprintf(buf, sizeof(buf), fmt, ap);

	syslog(priority, "%s", buf);
}

static void _log(int priority, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	_vlog(priority, fmt, ap);
	va_end(ap);
}


void log_fatal(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);

	if (errno) {
		int en = errno;
		char buf[BUFSIZE];

		vsnprintf(buf, sizeof(buf), fmt, ap);
		_log(LOG_CRIT, "%s (%s)", buf, strerror(en));
	} else {
		_vlog(LOG_CRIT, fmt, ap);
	}
	va_end(ap);

}

#define _GOPHERNICUS_MK_LOG_FUNCTION(FCT_LVL, LOG_LVL)	\
void FCT_LVL(const char *fmt, ...)			\
{							\
	va_list ap;					\
							\
	va_start(ap, fmt);				\
	_vlog(LOG_LVL, fmt, ap);			\
	va_end(ap);					\
}
_GOPHERNICUS_MK_LOG_FUNCTION(log_warning, LOG_WARNING)
_GOPHERNICUS_MK_LOG_FUNCTION(log_info, LOG_INFO)
_GOPHERNICUS_MK_LOG_FUNCTION(log_debug, LOG_DEBUG)
#undef _GOPHERNICUS_MK_LOG_FUNCTION
