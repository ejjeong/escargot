#ifndef InternalString_h
#define InternalString_h

namespace escargot {
class ESString;

// TODO implement utf-16, 4-bytes case

ALWAYS_INLINE size_t utf16ToUtf8(char16_t uc, char* UTF8)
{
    size_t tRequiredSize = 0;

    if (uc <= 0x7f) {
        if (NULL != UTF8) {
            UTF8[0] = (char) uc;
            UTF8[1] = (char) '\0';
        }
        tRequiredSize = 1;
    } else if (uc <= 0x7ff) {
        if (NULL != UTF8) {
            UTF8[0] = (char) (0xc0 + uc / (0x01 << 6));
            UTF8[1] = (char) (0x80 + uc % (0x01 << 6));
            UTF8[2] = (char) '\0';
        }
        tRequiredSize = 2;
    } else { // uc <= 0xffff
        if (NULL != UTF8) {
            UTF8[0] = (char) (0xe0 + uc / (0x01 << 12));
            UTF8[1] = (char) (0x80 + uc / (0x01 << 6) % (0x01 << 6));
            UTF8[2] = (char) (0x80 + uc % (0x01 << 6));
            UTF8[3] = (char) '\0';
        }
        tRequiredSize = 3;
    }

    return tRequiredSize;
}

inline const char * utf16ToUtf8(const char16_t *t, const size_t& len, size_t* bufferSize = NULL)
{
    unsigned strLength = 0;
    char buffer[MB_CUR_MAX];
    for (size_t i = 0; i < len ; i ++) {
        int length = utf16ToUtf8(t[i], buffer);
        strLength += length;
    }

    char* result = (char *)GC_MALLOC_ATOMIC(strLength + 1);
    if (bufferSize)
        *bufferSize = strLength + 1;
    unsigned currentPosition = 0;

    for (size_t i = 0; i < len ; i ++) {
        int length = utf16ToUtf8(t[i], buffer);
        memcpy(&result[currentPosition], buffer, length);
        currentPosition += length;
    }
    result[strLength] = 0;

    return result;
}

// http://egloos.zum.com/profrog/v/1177107
ALWAYS_INLINE size_t utf8ToUtf16(char* UTF8, char16_t& uc)
{
    size_t tRequiredSize = 0;

    uc = 0x0000;

    // ASCII byte
    if (0 == (UTF8[0] & 0x80)) {
        uc = UTF8[0];
        tRequiredSize = 1;
    } else {
        // Start byte for 2byte
        if (0xC0 == (UTF8[0] & 0xE0)
            && 0x80 == (UTF8[1] & 0xC0) ) {
            uc += (UTF8[0] & 0x1F) << 6;
            uc += (UTF8[1] & 0x3F) << 0;
            tRequiredSize = 2;
        } else { // Start byte for 3byte
            if (0xE0 == (UTF8[0] & 0xE0)
                && 0x80 == (UTF8[1] & 0xC0)
                && 0x80 == (UTF8[2] & 0xC0)) {
                uc += (UTF8[0] & 0x1F) << 12;
                uc += (UTF8[1] & 0x3F) << 6;
                uc += (UTF8[2] & 0x3F) << 0;
                tRequiredSize = 3;
            } else {
                // Invalid case
                tRequiredSize = 1;
                RELEASE_ASSERT_NOT_REACHED();
            }
        }
    }

    return tRequiredSize;
}

}

#endif
