#ifndef MISSING_THINGS_FROM_CYGWIN_H_
#define MISSING_THINGS_FROM_CYGWIN_H_

inline size_t strnlen(const char *s, size_t maxlen){
        size_t len;

        for (len = 0; len < maxlen; len++, s++) {
                if (!*s)
                        break;
        }
        return (len);
}

#define M_PI 3.14159265358979323846

#endif

