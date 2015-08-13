#ifndef NullableString_h
#define NullableString_h

namespace escargot {

class NullableString {
public:
    NullableString(char* str, int len)
        :m_string(str),
         m_length(len)
    {
    }
    ~NullableString() {
        delete [] m_string;
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
