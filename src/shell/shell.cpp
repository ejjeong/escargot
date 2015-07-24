#include "Escargot.h"
#include "vm/ESVMInstance.h"
#include "runtime/ESValue.h"


int main(int argc, char* argv[])
{
/*
    //ESValue test
    escargot::ESValue* u= escargot::undefined;
    escargot::ESValue* n = escargot::null;

    escargot::Undefined* uu = u->toHeapObject()->toUndefined();
    escargot::Null* nn = n->toHeapObject()->toNull();

    ASSERT(uu == escargot::undefined);
    ASSERT(nn == escargot::null);

    escargot::Smi* s = escargot::Smi::fromInt(2);
    ASSERT(s->toSmi()->value() == 2);
*/
    //JSObject & gc_allocator test
    /*
    escargot::JSObject* obj = escargot::JSObject::create();
    obj->set("asdf",escargot::Smi::fromInt(2));

    ASSERT(obj->toHeapObject()->isJSObject());
    escargot::JSObject* o = escargot::JSObject::create();
    obj->set("obj",o);
    o = NULL;

    GC_gcollect();
    ASSERT(obj->get(L"asdf")->toSmi()->value() == 2);
    ASSERT(obj->get("obj")->toHeapObject()->isJSObject());

    obj->set("obj",escargot::esUndefined);
    GC_gcollect();
    escargot::ESValue* val = obj->get("obj");
    ASSERT(val->isHeapObject());
    ASSERT(obj->get("obj")->toHeapObject()->isUndefined());
     */
    escargot::ESVMInstance* ES = new escargot::ESVMInstance();
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

    return 0;
}
