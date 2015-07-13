#include "Escargot.h"
#include "parser/ESScriptParser.h"


int main()
{
    while (true) {
        char buf[512];
        printf("shell> ");
        fgets(buf, sizeof buf/sizeof (char), stdin);

        escargot::ESScriptParser::parseScript(buf);
    }
    return 0;
}
