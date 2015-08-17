#ifndef InternalString_h
#define InternalString_h

namespace escargot {
class ESString;

ALWAYS_INLINE const char * utf16ToUtf8(const wchar_t *t)
{
    unsigned strLength = 0;
    const wchar_t* pt = t;
    char buffer [MB_CUR_MAX];
    while(*pt) {
        int length = std::wctomb(buffer,*pt);
        if (length<1)
            break;
        strLength += length;
        pt++;
    }

    char* result = (char *)GC_malloc_atomic(strLength + 1);
    pt = t;
    unsigned currentPosition = 0;

    while(*pt) {
        int length = std::wctomb(buffer,*pt);
        if (length<1)
            break;
        memcpy(&result[currentPosition],buffer,length);
        currentPosition += length;
        pt++;
    }
    result[strLength] = 0;

    return result;
}

ALWAYS_INLINE NullableString toNullableUtf8(const std::wstring& m_string)
{
    unsigned strLength = m_string.length();
    const wchar_t* pt = m_string.data();
    char buffer [MB_CUR_MAX];
    memset(buffer, 0, MB_CUR_MAX);

    char* string = new char[strLength * MB_CUR_MAX + 1];

    int idx = 0;
    for (unsigned i = 0; i < strLength; i++) {
        int length = std::wctomb(buffer,*pt);
        if (length<1) {
            string[idx++] = '\0';
        } else {
            strncpy(string+idx, buffer, length);
            idx += length;
        }
        pt++;
    }
    return NullableString(string, idx);
}

//http://egloos.zum.com/profrog/v/1177107
ALWAYS_INLINE size_t utf8ToUtf16(char* UTF8, wchar_t& uc)
{
    size_t tRequiredSize = 0;

    uc = 0x0000;

    // ASCII byte
    if( 0 == (UTF8[0] & 0x80) )
    {
       uc = UTF8[0];
       tRequiredSize = 1;
    }
    else // Start byte for 2byte
    if( 0xC0 == (UTF8[0] & 0xE0) &&
           0x80 == (UTF8[1] & 0xC0) )
    {
       uc += (UTF8[0] & 0x1F) << 6;
       uc += (UTF8[1] & 0x3F) << 0;
       tRequiredSize = 2;
    }
    else // Start byte for 3byte
    if( 0xE0 == (UTF8[0] & 0xE0) &&
           0x80 == (UTF8[1] & 0xC0) &&
           0x80 == (UTF8[2] & 0xC0) )
    {
       uc += (UTF8[0] & 0x1F) << 12;
       uc += (UTF8[1] & 0x3F) << 6;
       uc += (UTF8[2] & 0x3F) << 0;
       tRequiredSize = 3;
    }
    else
    {
        // Invalid case
        tRequiredSize = 1;
        RELEASE_ASSERT_NOT_REACHED();
    }

    return tRequiredSize;
}

ESString* utf8ToUtf16(const char *s, int length);

}

#endif
