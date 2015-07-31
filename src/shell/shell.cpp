#include "Escargot.h"
#include "vm/ESVMInstance.h"
#include "runtime/ESValue.h"


int main(int argc, char* argv[])
{
/*
    //ESValue test
    escargot::ESValue* u= escargot::undefined;
    escargot::ESValue* n = escargot::null;

    escargot::ESUndefined* uu = u->toHeapObject()->toESUndefined();
    escargot::ESNull* nn = n->toHeapObject()->toESNull();

    ASSERT(uu == escargot::undefined);
    ASSERT(nn == escargot::null);

    escargot::Smi* s = escargot::Smi::fromInt(2);
    ASSERT(s->toSmi()->value() == 2);
*/
    //ESObject & gc_allocator test
    /*
    escargot::ESObject* obj = escargot::ESObject::create();
    obj->set("asdf",escargot::Smi::fromInt(2));

    ASSERT(obj->toHeapObject()->isESObject());
    escargot::ESObject* o = escargot::ESObject::create();
    obj->set("obj",o);
    o = NULL;

    GC_gcollect();
    ASSERT(obj->get(L"asdf")->toSmi()->value() == 2);
    ASSERT(obj->get("obj")->toHeapObject()->isESObject());

    obj->set("obj",escargot::esUndefined);
    GC_gcollect();
    escargot::ESValue* val = obj->get("obj");
    ASSERT(val->isHeapObject());
    ASSERT(obj->get("obj")->toHeapObject()->isESUndefined());
     */
    escargot::ESVMInstance* ES = new escargot::ESVMInstance();
    ES->enter();
    if(argc == 1) {
        while (true) {
            char buf[512];
            wprintf(L"shell> ");
            fgets(buf, sizeof buf, stdin);
            ES->evaluate(buf);
        }
    } else {
        for(int i = 1; i < argc; i ++) {
            FILE *fp = fopen(argv[i],"r");
            if(fp) {
                std::string str;
                char buf[512];
                while(fgets(buf, sizeof buf, fp) != NULL) {
                    str += buf;
                }
                fclose(fp);
                ES->evaluate(str);
            }
        }
    }
    ES->exit();

    return 0;
}
