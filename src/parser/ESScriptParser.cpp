#include "Escargot.h"
#include "ESScriptParser.h"
#include "ast/AST.h"

namespace escargot {

AST* ESScriptParser::parseScript(const std::string& source)
{
    std::string sc;
    for(unsigned i = 0 ; i < source.length() ; i ++) {
        char c = source[i];

        if(c == '\n') {
            sc.push_back('\\');
            c = '\n';
        } else if(c == '/') {
            if(i + 1 < source.length() && source[i + 1] == '/') {
                while(source[i] != '\n' && i < source.length()) {
                    i ++;
                }
                continue;
            }
            else if(i + 1 < source.length() && source[i + 1] == '*') {
                while(i < source.length()) {
                    if(source[i] == '*') {
                        if(i + 1 < source.length()) {
                            if(source[i + 1] == '/') {
                                i++;
                                break;
                            }
                        }
                    }
                    i ++;
                }
                continue;
            }

        }

        sc.push_back(c);
    }

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

    puts(outputString.c_str());
    rapidjson::Document jsonDocument;
    rapidjson::MemoryStream stream(outputString.c_str(),outputString.length());
    jsonDocument.ParseStream(stream);

    //READ SAMPLE
    //std::string type = jsonDocument["type"].GetString();
    //puts(type.c_str());

    return new AST();
}

}
