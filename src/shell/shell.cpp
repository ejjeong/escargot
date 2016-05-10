#include "Escargot.h"
#include "vm/ESVMInstance.h"
#include "runtime/ESValue.h"
#include "ast/AST.h"

#ifdef ESCARGOT_PROFILE
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <stdio.h>
#endif

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

#ifdef ESCARGOT_PROFILE
void dumpStats()
{
    unsigned stat;
    auto stream = stderr;

    stat = GC_get_heap_size();
    fwprintf(stream, L"[BOEHM] heap_size: %d\n", stat);
    stat = GC_get_unmapped_bytes();
    fwprintf(stream, L"[BOEHM] unmapped_bytes: %d\n", stat);
    stat = GC_get_total_bytes();
    fwprintf(stream, L"[BOEHM] total_bytes: %d\n", stat);
    stat = GC_get_memory_use();
    fwprintf(stream, L"[BOEHM] memory_use: %d\n", stat);
    stat = GC_get_gc_no();
    fwprintf(stream, L"[BOEHM] gc_no: %d\n", stat);

    struct rusage ru;
    getrusage(RUSAGE_SELF, &ru);
    stat = ru.ru_maxrss;
    fwprintf(stream, L"[LINUX] rss: %d\n", stat);

#if 0
    if (stat > 10000) {
        while (true) { }
    }
#endif
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

#ifdef PROFILE_MASSIF
std::unordered_map<void*, void*> g_addressTable;
std::vector<void *> g_freeList;

void unregisterGCAddress(void* address)
{
    // ASSERT(g_addressTable.find(address) != g_addressTable.end());
    if (g_addressTable.find(address) != g_addressTable.end()) {
        auto iter = g_addressTable.find(address);
        free(iter->second);
        g_addressTable.erase(iter);
    }
}

void registerGCAddress(void* address, size_t siz)
{
    if (g_addressTable.find(address) != g_addressTable.end()) {
        unregisterGCAddress(address);
    }
    g_addressTable[address] = malloc(siz);
}

void* GC_malloc_hook(size_t siz)
{
    void* ptr;
#ifdef NDEBUG
    ptr = GC_malloc(siz);
#else
    ptr = GC_malloc(siz);
#endif
    registerGCAddress(ptr, siz);
    return ptr;
}
void* GC_malloc_atomic_hook(size_t siz)
{
    void* ptr;
#ifdef NDEBUG
    ptr = GC_malloc_atomic(siz);
#else
    ptr = GC_malloc_atomic(siz);
#endif
    registerGCAddress(ptr, siz);
    return ptr;
}

void GC_free_hook(void* address)
{
#ifdef NDEBUG
    GC_free(address);
#else
    GC_free(address);
#endif
    unregisterGCAddress(address);
}


#endif

bool evaluate(escargot::ESVMInstance* instance, escargot::ESString* str, bool printValue, bool readFromFile)
{
    std::jmp_buf tryPosition;
    if (setjmp(instance->registerTryPos(&tryPosition)) == 0) {
        escargot::ESValue ret = instance->evaluate(str);
        if (readFromFile) {
#ifndef NDEBUG
            if (instance->m_reportCompiledFunction) {
                printf("(%zu)\n", instance->m_compiledFunctions);
            }
            if (instance->m_reportOSRExitedFunction) {
                printf("(%zu)\n", instance->m_osrExitedFunctions);
            }
#endif
        }
        if (printValue) {
            instance->printValue(ret);
        }
        instance->unregisterTryPos(&tryPosition);
        instance->unregisterCheckedObjectAll();
    } else {
        escargot::ESValue err = instance->getCatchedError();
        printf("Uncaught ");
        escargot::ESVMInstance::printValue(err);
        if (readFromFile) {
            instance->exit();
            return false;
        }
    }
    return true;
}

int main(int argc, char* argv[])
{
#ifdef PROFILE_MASSIF
    GC_is_valid_displacement_print_proc = [](void* ptr)
    {
        g_freeList.push_back(ptr);
    };
    GC_set_on_collection_event([](GC_EventType evtType) {
        if (GC_EVENT_PRE_START_WORLD == evtType) {
            auto iter = g_addressTable.begin();
            while (iter != g_addressTable.end()) {
                GC_is_valid_displacement(iter->first);
                iter++;
            }

            for (unsigned i = 0; i < g_freeList.size(); i ++) {
                unregisterGCAddress(g_freeList[i]);
            }

            g_freeList.clear();
        }
    });
#endif
    // printf("%d", sizeof (escargot::ExecutionContext));
    /*
    static void* root[300000];
    int j = 0;
    for(int i = 0; i < 100000; i ++) {
        root[j++] = malloc(4);
        root[j++] = malloc(8);
        root[j++] = malloc(12);
        // root[j++] = GC_malloc(4);
        // root[j++] = GC_malloc(8);
        // root[j++] = GC_malloc(12);
    }
    abort();
    */
    // printf("%d", (int) sizeof(escargot::ProgramNode));
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

    ES->setlocale(icu::Locale::getUS());
    char* tz;
    tz = getenv("TZ");
    if (tz)
        ES->setTimezoneID(icu::UnicodeString(tz));
    else
        ES->setTimezoneID(icu::UnicodeString("Asia/Seoul"));

    if (argc == 1) {
        while (true) {
            char buf[512];
            printf("shell> ");
            if (!fgets(buf, sizeof buf, stdin)) {
                printf("ERROR: Cannot read interactive shell input\n");
                ES->exit();
                return 3;
            }
            escargot::ESString* str = escargot::ESString::create(buf);
            evaluate(ES, str, true, false);
        }
    } else {
        for (int i = 1; i < argc; i ++) {
#ifndef NDEBUG
            if (strcmp(argv[i], "-d") == 0) {
                ES->m_dumpByteCode = true;
            }
            if (strcmp(argv[i], "-de") == 0) {
                ES->m_dumpExecuteByteCode = true;
            }
#endif
            if (strcmp(argv[i], "-e") == 0) {
                escargot::ESString* str = escargot::ESString::create(argv[++i]);
                evaluate(ES, str, false, false);
            }
#ifndef NDEBUG
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
                escargot::ASCIIString str;
                char buf[512];
                while (fgets(buf, sizeof buf, fp) != NULL) {
                    str += buf;
                }
                fclose(fp);

                escargot::ESString* source = escargot::ESString::createUTF16StringIfNeeded(std::move(str));
                if (!evaluate(ES, source, false, true))
                    return 3;
            }

            if (strcmp(argv[i], "--shell") == 0) {
                while (true) {
                    char buf[512];
                    printf("shell> ");
                    if (!fgets(buf, sizeof buf, stdin)) {
                        printf("ERROR: Cannot read interactive shell input\n");
                        ES->exit();
                        return 3;
                    }
                    escargot::ESString* str = escargot::ESString::create(buf);
                    evaluate(ES, str, true, false);
                }
            }
        }
    }
#ifdef ESCARGOT_PROFILE
    dumpStats();
#endif
    ES->exit();
    // delete ES;
    // GC_gcollect();
    return 0;
}
