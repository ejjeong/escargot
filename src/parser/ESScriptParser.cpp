#include "Escargot.h"
#include "ESScriptParser.h"
#include "ast/AST.h"

namespace escargot {

AST* ESScriptParser::parseScript(const char* source)
{
    std::string sc = source;
    auto replace = [](std::string& str, const std::string& from, const std::string& to) {
        size_t start_pos = str.find(from);
        if(start_pos == std::string::npos)
            return ;
        str.replace(start_pos, from.length(), to);
        return ;
    };

    replace(sc, "\n", "\\\n");
    std::string sourceString = std::string("print(JSON.stringify(Reflect.parse('") + sc + "')))";

    FILE *fp;

    fp = fopen("/tmp/input.js", "w");
    fputs(sourceString.c_str(), fp);
    fflush(fp);
    fclose(fp);

    char path[1035];

    fp = popen("./mozjs /tmp/input.js", "r");
    if (fp == NULL) {
        printf("Failed to run command\n" );
        exit(1);
    }
    fputs(sourceString.c_str(), fp);

    std::string outputString;
    while (fgets(path, sizeof(path)-1, fp) != NULL) {
        outputString += path;
    }

    pclose(fp);

    //puts(outputString.c_str());
    rapidjson::Document jsonDocument;
    rapidjson::MemoryStream stream(outputString.c_str(),outputString.length());
    jsonDocument.ParseStream(stream);

    //READ SAMPLE
    //std::string type = jsonDocument["type"].GetString();
    //puts(type.c_str());

    return new AST();
}

}
