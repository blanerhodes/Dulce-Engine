struct StringHeader {
    u8 size;
    u8 length;
    //allocator??
};

struct String {
    char* cstr; 
};

u32 StringLength(u8* str) {
    u32 result = 0;

    while (*str) {
        result++;
        str++;
    }
    result++;
    return result;
}

static b8 StringsEqual(u8* str1, u8* str2) {
    if (!str1 || !str2) {
        DERROR("StringsEqual - Null string passed.");
        return false;
    }
    while ((*str1 && *str2) && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return (*str1 == '\0' && *str2 == '\0');
}

static b8 StringsEquali(char* str1, char* str2) {
    if (!str1 || !str2) {
        DERROR("StringsEquali - Null string passed.")
    }
    u32 lowercase_offset = 'a' - 'A';
    while (*str1 && *str2) {
        char char1 = *str1;
        char char2 = *str2;
        if (char1 >= 'A' && char1 <= 'Z') {
            char1 += lowercase_offset;
        }
        if (char2 >= 'A' && char2 <= 'Z') {
            char2 += lowercase_offset;
        }
        if (char1 != char2) {
            break;
        }
        str1++;
        str2++;
    }
    return (*str1 == '\0' && *str2 == '\0');
}

inline b32 IsEndOfLine(char c) {
    return (c == '\n') || (c == '\r');
}

inline b32 IsWhiteSpace(char c) {
    return (c == ' ') || (c == '\t') || (c == '\v') || (c == '\f') || IsEndOfLine(c);
}

inline u32 StringCopyToWS(u8* src, u8* dest, b32 include_ws = false) {
    u32 num_copied = 0;
    while(*src && dest && !IsWhiteSpace(*src)) {
        *dest = *src;
        src++;
        dest++;
        num_copied++;
    }
    if (include_ws) {
        while (*src && IsWhiteSpace(*src)) {
            src++;
            num_copied++;
        }
    }
    return num_copied;
}

inline void StringCopy(u8* src, u8* dest) {
    while(src && dest) {
        *src = *dest;
        src++;
        dest++;
    }
}
//TODO: make string compare that takes lengths