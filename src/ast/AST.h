#ifndef __AST__
#define __AST__

namespace escargot {

class ESVMInstance;
class AST : public gc_cleanup {
    void execute(ESVMInstance*);
};

}

#endif
