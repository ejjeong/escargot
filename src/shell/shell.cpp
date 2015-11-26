#include "Escargot.h"
#include "vm/ESVMInstance.h"
#include "runtime/ESValue.h"

#if defined(ENABLE_ESJIT) && !defined(NDEBUG)
#include "lirasm.cpp"
#endif

#ifdef ANDROID
void __attribute__((optimize("O0"))) fillStack(size_t siz)
{
    volatile char a[siz];
    for (unsigned i = 0 ; i < siz  ; i ++) {
        a[i] = 0x00;
    }
}
#endif
/*
#include <malloc.h>

void* gcm(size_t t)
{
    if (t>1024) {
        printf("gcm %d\n", (int)t);
    }
    return GC_malloc(t);
}
void* gca(size_t t)
{
    if (t>1024) {
        printf("gca %d\n", (int)t);
    }
    return GC_malloc_atomic(t);
}
 static void my_init_hook (void);
 static void *my_malloc_hook (size_t, const void *);
 static void my_free_hook (void*, const void *);

 void (* volatile __malloc_initialize_hook) (void) = my_init_hook;

 void *(*__MALLOC_HOOK_VOLATILE old_malloc_hook)(size_t __size,
                                                      const void *);

 void (*__MALLOC_HOOK_VOLATILE old_free_hook) (void *__ptr,
                                                    const void *);
 static void
 my_init_hook (void)
 {
   old_malloc_hook = __malloc_hook;
   old_free_hook = __free_hook;
   __malloc_hook = my_malloc_hook;
   __free_hook = my_free_hook;
 }

 static void *
 my_malloc_hook (size_t size, const void *caller)
 {
   void *result;
   __malloc_hook = old_malloc_hook;
   __free_hook = old_free_hook;
   result = malloc (size);
   old_malloc_hook = __malloc_hook;
   old_free_hook = __free_hook;
   // if (size > 1024)
   //    printf ("malloc (%u)(%p)\n", (unsigned int) size, result);
   __malloc_hook = my_malloc_hook;
   __free_hook = my_free_hook;
   return result;
 }

 static void
 my_free_hook (void *ptr, const void *caller)
 {
   __malloc_hook = old_malloc_hook;
   __free_hook = old_free_hook;
   // printf("free %p\n", ptr);
   free (ptr);
   old_malloc_hook = __malloc_hook;
   old_free_hook = __free_hook;
   __malloc_hook = my_malloc_hook;
   __free_hook = my_free_hook;
 }
*/
int main(int argc, char* argv[])
{
    // printf("%d", (int) sizeof(escargot::ByteCodeExtraData));
    // my_init_hook();
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
    fillStack(256*1024);
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
            std::jmp_buf tryPosition;
            if (setjmp(ES->registerTryPos(&tryPosition)) == 0) {
                escargot::ESValue ret = ES->evaluate(source);
                ES->printValue(ret);
                ES->unregisterTryPos(&tryPosition);
            } else {
                escargot::ESValue err = ES->getCatchedError();
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
                std::jmp_buf tryPosition;
                if (setjmp(ES->registerTryPos(&tryPosition)) == 0) {
                    escargot::ESValue ret = ES->evaluate(source);
#ifndef NDEBUG
                    if (ES->m_reportCompiledFunction) {
                        printf("(%zu)\n", escargot::ESVMInstance::currentInstance()->m_compiledFunctions);
                    }
                    if (ES->m_reportOSRExitedFunction) {
                        printf("(%zu)\n", escargot::ESVMInstance::currentInstance()->m_osrExitedFunctions);
                    }
#endif
                    ES->unregisterTryPos(&tryPosition);
                } else {
                    escargot::ESValue err = ES->getCatchedError();
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
                    std::jmp_buf tryPosition;
                    if (setjmp(ES->registerTryPos(&tryPosition)) == 0) {
                        escargot::ESValue ret = ES->evaluate(source);
                        ES->printValue(ret);
                        ES->unregisterTryPos(&tryPosition);
                    } else {
                        escargot::ESValue err = ES->getCatchedError();
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
    // delete ES;
    // GC_gcollect();
    return 0;
}
