#ifndef __UTIL_H__
#define __UTIL_H__
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#pragma GCC diagnostic ignored "-Wmissing-prototypes"

#ifdef __GNUC__
#define NORETURN __attribute__((__noreturn__))
#else
#define NORETURN
#ifndef __attribute__
#define __attribute__(x)
#endif
#endif

#ifndef __maybe_unused
# define __maybe_unused         /* unimplemented */
#endif

#define is_dir_sep(c) ((c) == '/')

#define alloc_nr(x) (((x)+16)*3/2)

/*
 * Realloc the buffer pointed at by variable 'x' so that it can hold
 * at least 'nr' entries; the number of entries currently allocated
 * is 'alloc', using the standard growing factor alloc_nr() macro.
 *
 *  DO NOT USE any expression with side-effect for 'x' or 'alloc'.
 */
#define ALLOC_GROW(x, nr, alloc) \
        do { \
                if ((nr) > alloc) { \
                        if (alloc_nr(alloc) < (nr)) \
                                alloc = (nr); \
                        else \
                                alloc = alloc_nr(alloc); \
                        x = xrealloc((x), alloc * sizeof(*(x))); \
                } \
        } while(0)

#define zfree(ptr) ({ free(*ptr); *ptr = NULL; })

#define BUILD_BUG_ON_ZERO(e) (sizeof(struct { int:-!!(e); }))

static inline const char *skip_prefix(const char *str, const char *prefix)
{
        size_t len = strlen(prefix);
        return strncmp(str, prefix, len) ? NULL : str + len;
}

void usage(const char *err) NORETURN;
void die(const char *err, ...) NORETURN __attribute__((format (printf, 1, 2)));
int error(const char *err, ...) __attribute__((format (printf, 1, 2)));
void warning(const char *err, ...) __attribute__((format (printf, 1, 2)));
void set_die_routine(void (*routine)(const char *err, va_list params) NORETURN);
char *xstrdup(const char *str);
void *xrealloc(void *ptr, size_t size);
int prefixcmp(const char *str, const char *prefix);

#endif /* __UTIL_H__ */
