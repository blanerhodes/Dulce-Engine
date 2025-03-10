#include "defines.h"

struct StringHeader {
    u8 size;
    u8 length;
    //allocator??
};

struct String {
    char* cstr; 
};

u32 StringLength(u8* str);
b8 StringsEqual(u8* str1, u8* str2);
b8 StringsEquali(char* str1, char* str2);
b32 IsEndOfLine(char c);
b32 IsWhiteSpace(char c);
u32 StringCopyToWS(u8* src, u8* dest, b32 include_ws = false);
void StringCopy(u8* src, u8* dest);