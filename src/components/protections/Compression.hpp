#pragma once
#include <string>
#include "../../components/ConfigParser.hpp"

class Compression {
private:
    static std::string compressLua(const std::string& code);
    static std::string optimizeAst(const std::string& code);
    static std::string shortenVariables(const std::string& code);
    static std::string removeWhitespace(const std::string& code);
    static std::string optimizeStrings(const std::string& code);
    static std::string mergeAdjacentStrings(const std::string& code);
    
    static std::string removeEmptyLines(const std::string& code);
    static std::string optimizeNumbers(const std::string& code);
    static std::string optimizeOperators(const std::string& code);
    static std::string optimizeBooleans(const std::string& code);
    static std::string optimizeLocalDeclarations(const std::string& code);

public:
    static std::string compress(const std::string& code, const ConfigParser& config);
}; 