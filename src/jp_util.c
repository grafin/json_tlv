#include <jp_util.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

_Noreturn void
__attribute__((format(printf, 1, 2)))
jp_panic(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	exit(-1);
}
