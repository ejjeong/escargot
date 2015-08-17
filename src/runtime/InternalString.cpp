#include "Escargot.h"

namespace escargot {

ESString* utf8ToUtf16(const char *s, int length)
{
    wchar_t* wstr = (wchar_t *)malloc(length * 6 + 1);
    char* pt8 = (char*) s;
    //wprintf(L"start length:%d\n", length);
    int wlen = 0;
    int idx = 0;
    int decodeLength = 0;
    while (decodeLength < length) {
        wchar_t wc;
        wlen = utf8ToUtf16(pt8,wc);
        wstr[idx++] = wc;
        pt8 += wlen;
        decodeLength += wlen;
    }
    ESString* str = ESString::create(std::move(std::wstring(wstr, idx)));
    free(wstr);
    return str;
}


}
