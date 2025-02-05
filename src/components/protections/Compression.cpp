#include "Compression.hpp"
#include <sstream>
#include <regex>
#include <map>
#include <set>
#include <variant>
#include <functional>
#include "../Logger.hpp"
#include "../ProgressBar.hpp"

std::string Compression::removeWhitespace(const std::string& code) {
    Logger::debug("Starting whitespace removal...");
    
    std::string result;
    bool inString = false;
    char stringDelimiter = 0;
    bool inComment = false;
    bool inMultilineComment = false;
    bool lastWasWhitespace = false;
    
    size_t processedChars = 0;
    size_t totalChars = code.length();
    
    for (size_t i = 0; i < code.length(); ++i) {
        processedChars++;
        if (processedChars % 50000 == 0) {
            Logger::debug("Processed " + std::to_string(processedChars) + "/" + std::to_string(totalChars) + " characters");
        }
        
        char c = code[i];
        char next = (i + 1 < code.length()) ? code[i + 1] : '\0';
        
        
        if (!inComment && !inMultilineComment && (c == '"' || c == '\'')) {
            if (!inString) {
                inString = true;
                stringDelimiter = c;
            } else if (c == stringDelimiter && i > 0 && code[i - 1] != '\\') {
                inString = false;
            }
            result += c;
            continue;
        }
        
        
        if (!inString && !inComment && !inMultilineComment && c == '-' && next == '-') {
            if (i + 3 < code.length() && code[i + 2] == '[' && code[i + 3] == '[') {
                inMultilineComment = true;
                i += 3;  
            } else {
                inComment = true;
            }
            continue;
        }
        
        
        if (inMultilineComment && c == ']' && next == ']') {
            inMultilineComment = false;
            i++;  
            continue;
        }
        
        
        if (inComment && c == '\n') {
            inComment = false;
            result += '\n';
            continue;
        }
        
        
        if (inComment || inMultilineComment) continue;
        
        
        if (std::isspace(static_cast<unsigned char>(c))) {
            if (inString) {
                result += c;
            } else if (!lastWasWhitespace) {
                
                if (i > 0 && i + 1 < code.length() &&
                    std::isalnum(static_cast<unsigned char>(code[i - 1])) && 
                    std::isalnum(static_cast<unsigned char>(next))) {
                    result += ' ';
                }
            }
            lastWasWhitespace = true;
            continue;
        }
        
        lastWasWhitespace = false;
        result += c;
    }
    
    Logger::debug("Whitespace removal completed");
    Logger::debug("Original size: " + std::to_string(code.length()) + ", New size: " + std::to_string(result.length()));
    
    return result;
}

std::string Compression::shortenVariables(const std::string& code) {
    std::regex varPattern(R"(\b(?:local\s+)?([a-zA-Z_][a-zA-Z0-9_]*)\b)");
    std::map<std::string, std::string> varMap;
    std::set<std::string> keywords = {
        "and", "break", "do", "else", "elseif", "end", "false", "for",
        "function", "if", "in", "local", "nil", "not", "or", "repeat",
        "return", "then", "true", "until", "while"
    };
    
    Logger::debug("Starting variable shortening process...");
    
    
    std::set<std::string> uniqueVars;
    std::string::const_iterator searchStart(code.cbegin());
    std::smatch match;
    
    Logger::debug("First pass: finding unique variables...");
    while (std::regex_search(searchStart, code.cend(), match, varPattern)) {
        std::string varName = match[1];
        if (keywords.find(varName) == keywords.end() && varName.substr(0, 2) != "__") {
            uniqueVars.insert(varName);
        }
        searchStart = match.suffix().first;
    }
    
    size_t totalVars = uniqueVars.size();
    Logger::debug("Found " + std::to_string(totalVars) + " unique variables to process");
    
    
    ProgressBar progress(totalVars, 50, "Shortening variables");
    size_t processedVars = 0;
    
    
    auto generateName = [&]() {
        static const std::string alphabet = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
        static size_t counter = 0;
        std::string name;
        
        do {
            
            name = alphabet[counter % alphabet.length()];
            
            
            size_t num = counter / alphabet.length();
            if (num > 0) {
                name += std::to_string(num);
            }
            
            counter++;
            
            
        } while (keywords.find(name) != keywords.end() || 
                 varMap.find(name) != varMap.end() || 
                 name.find("__") != std::string::npos);
        
        return name;
    };
    
    
    for (const auto& varName : uniqueVars) {
        if (keywords.find(varName) == keywords.end() && varName.substr(0, 2) != "__") {
            varMap[varName] = generateName();
            progress.update(++processedVars);
        }
    }
    
    progress.finish("");
    
    
    Logger::debug("Replacing variables in code...");
    std::string result = code;
    size_t currentVar = 0;
    ProgressBar replaceProgress(varMap.size(), 50, "Replacing variables");
    
    for (const auto& [oldName, newName] : varMap) {
        std::regex wordPattern("\\b" + oldName + "\\b");
        result = std::regex_replace(result, wordPattern, newName);
        replaceProgress.update(++currentVar);
    }
    
    replaceProgress.finish("Variable replacement completed - Processed " + std::to_string(currentVar) + " variables");
    
    return result;
}

std::string Compression::optimizeStrings(const std::string& code) {
    std::regex strPattern(R"((['"])((?:(?!\1).|\\.)*)\1)");
    std::string result = code;
    std::map<std::string, int> stringCount;
    
    
    std::smatch match;
    std::string::const_iterator searchStart(code.cbegin());
    while (std::regex_search(searchStart, code.cend(), match, strPattern)) {
        std::string str = match[0];
        stringCount[str]++;
        searchStart = match.suffix().first;
    }
    
    
    int varCount = 0;
    for (const auto& [str, count] : stringCount) {
        if (count > 1 && str.length() * count > str.length() + 10) {
            std::string varName = "_s" + std::to_string(varCount++);
            std::string declaration = "local " + varName + "=" + str + ";";
            result = declaration + std::regex_replace(result, std::regex(str), varName);
        }
    }
    
    return result;
}

std::string Compression::mergeAdjacentStrings(const std::string& code) {
    std::regex pattern(R"((['"])((?:(?!\1).|\\.)*)\1\s*\.\.\s*(['"])((?:(?!\3).|\\.)*)\3)");
    std::string result = code;
    std::smatch match;
    
    while (std::regex_search(result, match, pattern)) {
        std::string merged = "'" + match[2].str() + match[4].str() + "'";
        result = result.substr(0, match.position()) + merged + result.substr(match.position() + match.length());
    }
    
    return result;
}

std::string Compression::removeEmptyLines(const std::string& code) {
    std::stringstream ss;
    std::istringstream iss(code);
    std::string line;
    
    while (std::getline(iss, line)) {
        if (!line.empty() && line.find_first_not_of(" \t\r\n") != std::string::npos) {
            ss << line << '\n';
        }
    }
    return ss.str();
}

std::string Compression::optimizeNumbers(const std::string& code) {
    std::regex numPattern(R"(\b(\d+\.\d+|\d+)\b)");
    std::string result = code;
    std::smatch match;
    
    while (std::regex_search(result, match, numPattern)) {
        std::string num = match[1];
        if (num.find('.') != std::string::npos) {
            
            while (num.back() == '0') num.pop_back();
            if (num.back() == '.') num.pop_back();
        }
        result = result.substr(0, match.position()) + num + 
                result.substr(match.position() + match[0].length());
    }
    return result;
}

std::string Compression::optimizeOperators(const std::string& code) {
    std::string result = code;
    std::vector<std::pair<std::string, std::string>> replacements = {
        {" + ", "+"},
        {" - ", "-"},
        {" * ", "*"},
        {" / ", "/"},
        {" = ", "="},
        {" == ", "=="},
        {" ~= ", "~="},
        {" <= ", "<="},
        {" >= ", ">="},
        {" < ", "<"},
        {" > ", ">"},
        {" and ", " and "},  
        {" or ", " or "},
        {" not ", " not "},
        {" .. ", ".."},
        {", ", ","}
    };
    
    for (const auto& [pattern, replacement] : replacements) {
        size_t pos = 0;
        while ((pos = result.find(pattern, pos)) != std::string::npos) {
            result.replace(pos, pattern.length(), replacement);
            pos += replacement.length();
        }
    }
    return result;
}

std::string Compression::optimizeBooleans(const std::string& code) {
    std::string result = code;
    std::regex truePattern(R"(\btrue\b)");
    std::regex falsePattern(R"(\bfalse\b)");
    
    
    result = std::regex_replace(result, truePattern, "1==1");
    result = std::regex_replace(result, falsePattern, "1==2");
    
    return result;
}

std::string Compression::optimizeLocalDeclarations(const std::string& code) {
    std::regex localPattern(R"(local\s+([a-zA-Z_][a-zA-Z0-9_]*(?:\s*,\s*[a-zA-Z_][a-zA-Z0-9_]*)*)\s*=\s*([^;]+))");
    std::string result = code;
    std::string::const_iterator searchStart(result.cbegin());
    std::smatch match;
    
    std::map<std::string, std::vector<std::string>> groupedLocals;
    
    while (std::regex_search(searchStart, result.cend(), match, localPattern)) {
        std::string vars = match[1];
        std::string value = match[2];
        groupedLocals[value].push_back(vars);
        searchStart = match.suffix().first;
    }
    
    
    for (const auto& [value, vars] : groupedLocals) {
        if (vars.size() > 1) {
            std::string combined = "local " + vars[0];
            for (size_t i = 1; i < vars.size(); ++i) {
                combined += "," + vars[i];
            }
            combined += "=" + value;
            
            for (const auto& var : vars) {
                std::regex pattern("local\\s+" + var + "\\s*=\\s*" + value);
                result = std::regex_replace(result, pattern, "");
            }
            result = combined + "\n" + result;
        }
    }
    
    return result;
}

std::string Compression::compress(const std::string& code, const ConfigParser& config) {
    bool enabled = config.getBoolValue("Compression", "enabled", false);
    if (!enabled) return code;
    
    Logger::debug("Original code length: " + std::to_string(code.length()));
    
    size_t threshold = config.getIntValue("Compression", "threshold", 1024);
    if (code.length() < threshold) {
        Logger::info("Code size below compression threshold");
        return code;
    }
    
    Logger::info("Applying compression...");
    
    
    Logger::debug("Removing whitespace...");
    std::string result = removeWhitespace(code);
    Logger::debug("Code length after whitespace removal: " + std::to_string(result.length()));
    
    
    Logger::debug("Shortening variable names...");
    result = shortenVariables(result);
    Logger::debug("Code length after variable shortening: " + std::to_string(result.length()));
    
    
    Logger::debug("Optimizing strings...");
    result = optimizeStrings(result);
    Logger::debug("Code length after string optimization: " + std::to_string(result.length()));
    
    
    Logger::debug("Applying Lua-specific compression...");
    result = compressLua(result);
    Logger::debug("Code length after Lua compression: " + std::to_string(result.length()));
    
    
    Logger::debug("Applying AST optimizations...");
    result = optimizeAst(result);
    Logger::debug("Final code length: " + std::to_string(result.length()));
    
    double compressionRatio = ((double)code.length() - result.length()) / code.length() * 100;
    Logger::info("Compression completed - Reduced size by " + std::to_string((int)compressionRatio) + "%");
    
    return result;
}

std::string Compression::compressLua(const std::string& code) {
    std::string result = code;
    
    struct Replacement {
        std::string pattern;
        std::string replacement;
    };
    
    std::vector<Replacement> replacements = {
        
        {R"(function\s*\(\s*\))", "function()"},
        {R"(\)\s*then\b)", ")then"},
        {R"(\bdo\s*\n)", "do "},
        {R"(\bend\s*\n)", "end "},
        
        
        {R"(table\.concat)", "concat"},
        {R"(table\.insert)", "insert"},
        {R"(table\.remove)", "remove"},
        
        
        {R"(string\.format)", "format"},
        {R"(string\.sub)", "sub"},
        {R"(string\.len)", "len"},
        
        
        {R"(\blocal\s+function\b)", "local function"},
        {R"(\breturn\s+function\b)", "return function"},
        {R"(\bif\s+not\b)", "if not"},
        {R"(\bwhile\s+true\b)", "while true"},
        
        
        {R"(if\s+([%w_]+)\s*==\s*nil)", "if not $1"},
        {R"(if\s+([%w_]+)\s*~=\s*nil)", "if $1"},
        
        
        {R"(\b0*(\d+)\.0+\b)", "$1"},
        {R"(\b(\d+)\.(\d*[1-9])0+\b)", "$1.$2"},
    };
    
    for (const auto& rep : replacements) {
        std::regex regex(rep.pattern);
        result = std::regex_replace(result, regex, rep.replacement);
    }
    
    return result;
}

std::string Compression::optimizeAst(const std::string& code) {
    std::string result = code;
    
    
    struct Optimization {
        std::string pattern;
        std::variant<std::string, std::function<std::string(const std::smatch&)>> replacement;
    };
    
    std::vector<Optimization> optimizations = {
        
        {R"((\d+)\s*\+\s*(\d+))", std::function<std::string(const std::smatch&)>([](const std::smatch& m) {
            return std::to_string(std::stoi(m[1].str()) + std::stoi(m[2].str()));
        })},
        {R"((\d+)\s*\-\s*(\d+))", std::function<std::string(const std::smatch&)>([](const std::smatch& m) {
            return std::to_string(std::stoi(m[1].str()) - std::stoi(m[2].str()));
        })},
        {R"((\d+)\s*\*\s*(\d+))", std::function<std::string(const std::smatch&)>([](const std::smatch& m) {
            return std::to_string(std::stoi(m[1].str()) * std::stoi(m[2].str()));
        })},
        
        
        {R"(not\s*\(\s*true\s*\))", std::string("false")},
        {R"(not\s*\(\s*false\s*\))", std::string("true")},
        {R"(true\s*and\s*true)", std::string("true")},
        {R"(false\s*or\s*false)", std::string("false")},
        {R"(true\s*or\s*([%w_]+))", std::string("true")},
        {R"(false\s*and\s*([%w_]+))", std::string("false")},
        
        
        {R"(\"([^\"]*)\"\.\.\"([^\"]*)\")", std::string("\"$1$2\"")},
        {R"(\'([^\']*)\'\.\.\'([^\']*)\')", std::string("'$1$2'")},
        
        
        {R"(\{\s*\})", std::string("{}")},  
        {R"(\[\s*\])", std::string("[]")},  
        
        
        {R"(\(\s*\))", std::string("()")}, 
        {R"(\{\s*\})", std::string("{}")}, 
    };
    
    for (const auto& opt : optimizations) {
        std::regex regex(opt.pattern);
        if (std::holds_alternative<std::string>(opt.replacement)) {
            result = std::regex_replace(result, regex, std::get<std::string>(opt.replacement));
        } else {
            const auto& func = std::get<std::function<std::string(const std::smatch&)>>(opt.replacement);
            std::string temp;
            std::string::const_iterator searchStart(result.cbegin());
            std::smatch match;
            
            while (std::regex_search(searchStart, result.cend(), match, regex)) {
                temp += std::string(searchStart, match[0].first);
                temp += func(match);
                searchStart = match[0].second;
            }
            
            if (!temp.empty()) {
                temp += std::string(searchStart, result.cend());
                result = temp;
            }
        }
    }
    
    return result;
} 