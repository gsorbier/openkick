#ifndef EXEC_LIBC_HPP
#define EXEC_LIBC_HPP 1

#include <types.hpp>

//int strcmp(const char *, const char *);
//size_t strlen(const char *);

void *memset(void *, int, size_t) __attribute__((nonnull));
void *bzero(void *, size_t) __attribute__((nonnull));

int strcmp(const char *, const char *) __attribute__((nonnull));
inline int strcmp(const char *s1, const char *s2) {
    for(; *s1 == *s2; ++s1, ++s2)
        if(!*s1)
            return 0;
    return *s1 - *s2;
}

size_t strnlen(const char *, size_t) __attribute__((nonnull));
inline size_t strnlen(const char *string, size_t maxlen) {
    size_t length = 0;
    while(*string++ && length <= maxlen)
        ++length;
    return length;
}

template<typename T> inline T min(T left, T right) {
    return left < right ? left : right;
}


#endif
