#include "Escargot.h"
#include "vm/ESVMInstance.h"
#include "runtime/ESValue.h"

#ifdef ENABLE_ESJIT
#include "lirasm.cpp"
#endif

#ifdef ANDROID
void __attribute__((optimize("O0"))) fillStack(size_t siz)
{
    volatile char a[siz];
    for (unsigned i = 0 ; i < siz  ; i ++) {
        a[i] = 0xff;
    }
}
#endif

int main(int argc, char* argv[])
{
    /*    test* ptr = new test;
    char* pool = (char *)GC_malloc(1024);
    memcpy(pool + 16, &ptr, 8);
    ptr = nullptr;
    GC_gcollect();
    */
    // GC_malloc(32);
    // GC_disable();
    /*
    // ESValue test
    escargot::ESValue* u= escargot::undefined;
    escargot::ESValue* n = escargot::null;

    escargot::ESUndefined* uu = u->toHeapObject()->toESUndefined();
    escargot::ESNull* nn = n->toHeapObject()->toESNull();

    ASSERT(uu == escargot::undefined);
    ASSERT(nn == escargot::null);

    escargot::Smi* s = escargot::Smi::fromInt(2);
    ASSERT(s->toSmi()->value() == 2);
    */
    // ESObject & gc_allocator test
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
#ifndef NDEBUG
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);
#endif
#ifdef ANDROID
    /*
    struct rlimit rl;
    int result = getrlimit(RLIMIT_STACK, &rl);
    // printf("result %d,current stask Size %d(%p)\n", (int)result, (int)rl.rlim_cur, &result);
    const rlim_t kStackSize = 16 * 1024 * 1024;   // min stack size = 16 MB
    if (result == 0)
    {
        if (rl.rlim_cur < kStackSize)
        {
            rl.rlim_cur = kStackSize;
            result = setrlimit(RLIMIT_STACK, &rl);
            if (result != 0) {
                // fprintf(stdout, "setrlimit returned result = %d\n", result);
            } else {
                result = getrlimit(RLIMIT_STACK, &rl);
                // printf("result2 %d,current stask Size %d(%p)\n", (int)result, (int)rl.rlim_cur, &result);
            }
        }
    }
    */
    fillStack(0.2*1024*1024);
#endif
#if defined(ENABLE_ESJIT) && !defined(NDEBUG)
    if (argc >= 2 && strcmp(argv[1], "-a") == 0) {
        // Assembler Test
        return lirasm_main(argc-2, &argv[2]);
    }
#endif
    escargot::ESVMInstance* ES = new escargot::ESVMInstance();
    ES->enter();
    if (argc == 1) {
        while (true) {
            char buf[512];
            printf("shell> ");
            if (!fgets(buf, sizeof buf, stdin)) {
                printf("ERROR: Cannot read interactive shell input\n");
                ES->exit();
                return 1;
            }
            escargot::ESStringData source(buf);
            try {
                escargot::ESValue ret = ES->evaluate(source);
                ES->printValue(ret);
            } catch(const escargot::ESValue& err) {
                printf("Uncaught %s\n", err.toString()->utf8Data());
            }
        }
    } else {
        for (int i = 1; i < argc; i ++) {
#ifndef NDEBUG
            if (strcmp(argv[i], "-d") == 0) {
                ES->m_dumpByteCode = true;
            }
            if (strcmp(argv[i], "-e") == 0) {
                ES->m_dumpExecuteByteCode = true;
            }
            if (strcmp(argv[i], "-usever") == 0) {
                ES->m_useVerboseWriter = true;
            }
            if (strcmp(argv[i], "-useexp") == 0) {
                ES->m_useExprFilter = true;
            }
            if (strcmp(argv[i], "-usecse") == 0) {
                ES->m_useCseFilter = true;
            }
            if (strcmp(argv[i], "-vj") == 0) {
                ES->m_verboseJIT = true;
            }
            if (strcmp(argv[i], "-us") == 0) {
                ES->m_reportUnsupportedOpcode = true;
            }
            if (strcmp(argv[i], "-rcf") == 0) {
                ES->m_reportCompiledFunction = true;
            }
            if (strcmp(argv[i], "-rof") == 0) {
                ES->m_reportOSRExitedFunction = true;
            }
#endif
            if (strcmp(argv[i], "-jt") == 0) {
                ES->m_jitThreshold = atoi(argv[++i]);
            }
            if (strcmp(argv[i], "-ot") == 0) {
                ES->m_osrExitThreshold = atoi(argv[++i]);
            }
            if (strcmp(argv[i], "-p") == 0) {
                ES->m_profile = true;
            }
            FILE* fp = fopen(argv[i], "r");
            if (fp) {
                std::string str;
                char buf[512];
                while (fgets(buf, sizeof buf, fp) != NULL) {
                    str += buf;
                }
                fclose(fp);
                escargot::ESStringData source(str.c_str());
                try {
                    escargot::ESValue ret = ES->evaluate(source);
#ifndef NDEBUG
                    if (ES->m_reportCompiledFunction) {
                        printf("(%zu)\n", escargot::ESVMInstance::currentInstance()->m_compiledFunctions);
                    }
                    if (ES->m_reportOSRExitedFunction) {
                        printf("(%zu)\n", escargot::ESVMInstance::currentInstance()->m_osrExitedFunctions);
                    }
#endif
                } catch(const escargot::ESValue& err) {
                    printf("Uncaught %s\n", err.toString()->utf8Data());
                    ES->exit();
                    return 1;
                }
            }

            if (strcmp(argv[i], "--shell") == 0) {
                while (true) {
                    char buf[512];
                    printf("shell> ");
                    if (!fgets(buf, sizeof buf, stdin)) {
                        printf("ERROR: Cannot read interactive shell input\n");
                        ES->exit();
                        return 1;
                    }
                    escargot::ESStringData source(buf);
                    try {
                        escargot::ESValue ret = ES->evaluate(source);
                        ES->printValue(ret);
                    } catch(const escargot::ESValue& err) {
                        printf("Uncaught %s\n", err.toString()->utf8Data());
                    }
                }
            }
        }
    }
#ifdef ESCARGOT_PROFILE
    escargot::ESScriptParser::dumpStats();
#endif
    ES->exit();
    return 0;
}
