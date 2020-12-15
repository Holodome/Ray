#if !defined(RAY_MISC_H)

#include "general.h"
#include "ray_random.h"
#include "ray_math.h"

inline u32 
rgba_pack_4x8(u32 r, u32 g, u32 b, u32 a) {
    // If values passed here are greater that 255 something for sure went wrong
    assert(r <= 0xFF && b <= 0xFF && b <= 0xFF && a <= 0xFF);
    return r << 0 | g << 8 | b << 16 | a << 24;
}

inline u32
rgba_pack_4x8_linear1(f32 r, f32 g, f32 b, f32 a) {
    if (r != r) { r = 0.0f; }
    if (g != g) { g = 0.0f; }
    if (b != b) { b = 0.0f; }
    if (a != a) { a = 0.0f; }
    
    u32 ru = roundf(clamp(r, 0, 0.999f) * 255.0f);
    u32 gu = roundf(clamp(g, 0, 0.999f) * 255.0f);
    u32 bu = roundf(clamp(b, 0, 0.999f) * 255.0f);
    u32 au = roundf(clamp(a, 0, 0.999f) * 255.0f);
    return rgba_pack_4x8(ru, gu, bu, au);
}

inline f32
linear1_to_srgb1(f32 l) {
    l = clamp(l, 0, 0.999f);
    
    f32 s = l * 12.92f;
    if (l > 0.0031308f) {
        s = 1.055f * powf(l, 1.0 / 2.4f) - 0.055f;
    }
    return s;
}

inline void 
format_time_ms(char *buffer, u64 buffer_size, u64 time) {
    u64 ms    = time % 1000;
    u64 sec   = time % 60000 / 1000;
    u64 min   = time % 3600000 / 60000;
    u64 hours = time / 3600000;
    
    if (hours) {
        u32 t = snprintf(buffer, buffer_size, "%lluh ", hours);
        buffer += t;
        buffer_size -= t;
    }
    if (min || hours) {
        u32 t = snprintf(buffer, buffer_size, "%llum ", min);
        buffer += t;
        buffer_size -= t;
    }
    if (sec || min || hours) {
        u32 t = snprintf(buffer, buffer_size, "%llus ", sec);
        buffer += t;
        buffer_size -= t;
    }
    snprintf(buffer, buffer_size, "%llums", ms);
}

inline void 
format_number_with_thousand_separators(char *buffer, u64 buffer_size, u64 number) {
    memset(buffer, 0, buffer_size);
    
    char digits_ascii[64];
    i32 cursor = 63;
    u64 n = number;
    while (n) {
        char digit = '0' + n % 10;
        digits_ascii[cursor--] = digit; 
        n /= 10;
    }
    ++cursor;
    
    for(u32 i = cursor;
        i < 64;
        ++i) {
        u32 t = snprintf(buffer, buffer_size, "%c", digits_ascii[i]);
        buffer += t;
        buffer_size -= t;              
        if ((63 - i) % 3 == 0 && 63 - i) {
            t = snprintf(buffer, buffer_size, ".");
            buffer += t;
            buffer_size -= t;              
        }
    }   
}

inline void 
format_bytes(char *buffer, u64 buffer_size, u64 bytes) {
    u64 b  = bytes % (1 << 10);
    u64 kb = bytes % (1 << 20) / (1 << 10);
    u64 mb = bytes % (1 << 30) / (1 << 20);
    u64 gb = bytes / (1 << 30);
    
    if (gb) {
        u32 t = snprintf(buffer, buffer_size, "%lluGB ", gb);
        buffer += t;
        buffer_size -= t;
    }
    if (mb || gb) {
        u32 t = snprintf(buffer, buffer_size, "%lluMB ", mb);
        buffer += t;
        buffer_size -= t;
    }
    if (kb || mb || gb) {
        u32 t = snprintf(buffer, buffer_size, "%lluKB ", kb);
        buffer += t;
        buffer_size -= t;
    }
    snprintf(buffer, buffer_size, "%lluB", b);
} 

#define RAY_MISC_H 1
#endif
