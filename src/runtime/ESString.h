#ifndef ESString_h
#define ESString_h

namespace escargot {

//borrow concept from coffeemix/runtime/gc_helper.h
typedef std::basic_string<wchar_t, std::char_traits<wchar_t>, gc_allocator<wchar_t> > ESStringStd;
class ESString : public gc_cleanup, public ESStringStd {
public:
    ESString() : ESStringStd() { }
    ESString(const char* s) : ESStringStd()
    {
        std::mbstate_t state = std::mbstate_t();
        int len = 1 + std::mbsrtowcs(NULL, &s, 0, &state);
        resize(len);
        std::mbsrtowcs((wchar_t *)c_str(), &s, size(), &state);
    }
    ESString(const ESStringStd& s) : ESStringStd(s) { }
    ESString substring(size_t pos = 0, size_t len = npos) const
    {
        return ESString(ESStringStd::substr(pos, len));
    }
protected:

};

}

#endif
