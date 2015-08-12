#ifndef NullableString_h
#define NullableString_h

namespace escargot {

class NullableString : public gc_cleanup {
public:
    NullableString(char* str, int len)
        :m_string(str),
         m_length(len)
    {
    }
    char* string() {
        return m_string;
    }
    int length() {
        return m_length;
    }
private:
    char* m_string;
    int m_length;
};
}

#endif
