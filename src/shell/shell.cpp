#include "Escargot.h"
#include "parser/ESScriptParser.h"
#include "vm/ESVMInstance.h"


int main(int argc, char* argv[])
{
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
