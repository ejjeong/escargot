#include "Escargot.h"
#include "parser/ESScriptParser.h"
#include "ESVMInstance.h"


int main()
{
    ESVMInstance* ES = new ESVMInstance();
    while (true) {
        char buf[512];
        printf("shell> ");
        fgets(buf, sizeof buf/sizeof (char), stdin);

        escargot::ESScriptParser::parseScript(buf);
    }
    return 0;
}
