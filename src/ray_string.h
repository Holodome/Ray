#if !defined(STRING_H)

#include "general.h"

typedef struct {
    char *s;
    u64 l;
} String;

#define STR(_cs) ((String) { .s = _cs, .l = sizeof(_cs) - 1 })

inline String 
string(char *s, u64 l) {
    return (String) {
        .s = s,
        .l = l
    };
}
inline String
string_from_cstring(char *cstring) {
    String result;
    result.s = cstring;
    result.l = strlen(cstring);
    return result;
}

inline bool
string_equals(String a, String b) {
    bool result = a.l == b.l;
    if (result) {
        result = strncmp(a.s, b.s, a.l) == 0;
    }
    return result;
}

inline bool
string_startswith(String str, String start) {
    bool result = false;
    if (str.l >= start.l) {
        result = strncmp(str.s, start.s, start.l) == 0;
    }

    return result;
}

inline String
string_advance(String a, u64 count) {
    String result = a;
    result.s += count;
    result.l -= count;
    return result;
}

inline String
string_lstrip(String str) {
    String result = str;
    while (result.l) {
        if (!isspace(*result.s)) {
            break;
        } else {
            --result.l;
            ++result.s;
        }
    }

    return result;
}

inline String
string_rstrip(String str) {
    String result = str;
    while (result.l) {
        if (!isspace(result.s[result.l - 1])) {
            break;
        } else {
            --result.l;
        }
    }
    return result;
}

inline String
string_strip(String str) {
    String result = str;
    result = string_lstrip(result);
    result = string_rstrip(result);
    return result;
}

inline char *
string_find(String str, char symb) {
    char *result = memchr(str.s, symb, str.l);
    return result;
}

inline u64 
string_to_buffer(String str, void *buffer, u64 buffer_size) {
    u64 size = str.l;
    if (size > buffer_size) {
        size = buffer_size;
    }
    memcpy(buffer, str.s, size);
    return size;
}

inline String
substr_till_symb(String str, char symb, bool *reached_bounds) {
    char *end = memchr(str.s, symb, str.l);
    if (!end) {
        end = str.s + str.l + 1;
        
        if (reached_bounds) {
            *reached_bounds = true;
        }
    }
    
    return string(str.s, end - str.s);
}

inline char *
skip_to_next_line(char *cursor) {
    while (*cursor && *cursor != '\n') {
        ++cursor;
    }
    
    if (*cursor == '\n') {
        ++cursor;
    }
    return cursor;
}


#define STRING_H 1
#endif
