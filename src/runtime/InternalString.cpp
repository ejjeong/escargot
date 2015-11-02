#include "Escargot.h"

namespace escargot {

u16string utf8ToUtf16(const char *s, int length)
{
    u16string ws;
    ws.reserve(length);
    char* pt8 = (char*) s;
    // wprintf(L"start length:%d\n", length);
    int wlen = 0;
    int idx = 0;
    int decodeLength = 0;
    while (decodeLength < length) {
        char16_t wc;
        wlen = utf8ToUtf16(pt8,wc);
        ws.push_back(wc);
        pt8 += wlen;
        decodeLength += wlen;
    }
    return std::move(ws);
}


}



