/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

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

}

#endif
