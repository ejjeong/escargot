#ifndef AST_h
#define AST_h

namespace escargot {

class ESVMInstance;
class AST : public gc_cleanup {
    void execute(ESVMInstance* );
};

}

#endif
