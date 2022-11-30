#ifndef JP_UTIL_H
#define JP_UTIL_H

_Noreturn void
__attribute__((format(printf, 1, 2)))
jp_panic(const char *fmt, ...);

#endif /* JP_UTIL_H */
