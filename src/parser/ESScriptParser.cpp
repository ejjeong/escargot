#include "Escargot.h"
#include "ESScriptParser.h"
#include "ast/AST.h"

namespace escargot {

AST* ESScriptParser::parseScript(const char* source)
{
    std::string sourceString = std::string("JSON.stringify(Reflect.parse('") + source + "'))";
    FILE* fp = fopen("input.js","w");
    fputs(sourceString.c_str(), fp);
    fflush(fp);
    fclose(fp);
    system("./mozjs input.js");
    return new AST();
}

}
