#include "Escargot.h"
#include "parser/ESScriptParser.h"
#include "vm/ESVMInstance.h"


int main()
{
    escargot::ESVMInstance* ES = new escargot::ESVMInstance();
    while (true) {
        char buf[512];
        printf("shell> ");
        fgets(buf, sizeof buf/sizeof (char), stdin);
        ES->evaluate(buf);
    }
    return 0;
}
