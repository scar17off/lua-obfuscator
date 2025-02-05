#include "StringEncryption.hpp"
#include "../Logger.hpp"
#include "../ProgressBar.hpp"
#include <sstream>
#include <regex>
#include <algorithm>
#include <numeric>

std::string StringEncryption::encryptBytes(const std::string& input, const std::vector<uint8_t>& key, const std::string& varName, size_t chunkSize) {
    std::vector<uint8_t> encrypted;
    for (size_t i = 0; i < input.length(); ++i) {
        uint8_t byte = input[i];
        byte ^= key[i % key.size()];
        byte = (byte << 3) | (byte >> 5); 
        encrypted.push_back(byte);
    }

    std::stringstream ss;
    
    
    if (encrypted.empty()) {
        ss << "local " << varName << " = \"\"\n";
        return ss.str();
    }
    
    
    size_t numChunks = (encrypted.size() + chunkSize - 1) / chunkSize;
    
    
    std::vector<std::string> chunkVars;
    for (size_t chunk = 0; chunk < numChunks; ++chunk) {
        std::string chunkVar = varName + "_" + std::to_string(chunk);
        chunkVars.push_back(chunkVar);
        
        ss << "local " << chunkVar << " = string.char(";
        size_t start = chunk * chunkSize;
        size_t end = std::min<size_t>(start + chunkSize, encrypted.size());
        
        for (size_t i = start; i < end; ++i) {
            if (i > start) ss << ",";
            ss << std::to_string((int)encrypted[i]); 
        }
        ss << ")\n";
    }

    
    ss << "local " << varName << " = table.concat({";
    for (size_t i = 0; i < chunkVars.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << "[" << (i+1) << "]=" << chunkVars[i];
    }
    ss << "})\n";

    return ss.str();
}

std::string StringEncryption::generateDecryptor(const std::vector<uint8_t>& key) {
    std::stringstream ss;
    
    
    ss << "_G.__decrypt = function(str, key)\n"
         "    if not str then return \"\" end\n"
         "    if type(str) ~= 'string' then return \"\" end\n"
         "    if #str == 0 then return \"\" end\n"
         "    local result = {}\n"
         "    for i = 1, #str do\n"
         "        local byte = str:byte(i)\n"
         "        byte = (byte >> 3) | (byte << 5) & 0xFF\n"
         "        byte = byte ~ key[(i-1) % #key + 1]\n"
         "        result[i] = string.char(byte)\n"
         "    end\n"
         "    return table.concat(result)\n"
         "end\n\n";

    return ss.str();
}

void StringEncryption::processString(std::string& code, const std::vector<uint8_t>& key, size_t chunkSize) {
    std::string decryptor = generateDecryptor(key);
    code = decryptor + code;
    
    
    std::vector<std::pair<size_t, std::string>> strings;  
    std::regex stringPattern(R"((['"])((?:(?!\1|[\r\n])[^\\]|\\.)*)\1)");
    std::string::const_iterator searchStart(code.cbegin());
    std::smatch match;
    
    
    size_t totalStrings = 0;
    auto countIter = code.cbegin();
    std::smatch countMatch;
    while (std::regex_search(countIter, code.cend(), countMatch, stringPattern)) {
        totalStrings++;
        countIter = countMatch.suffix().first;
    }
    
    
    ProgressBar progress(totalStrings, 50, "Analyzing strings");
    size_t processedCount = 0;
    
    try {
        while (std::regex_search(searchStart, code.cend(), match, stringPattern)) {
            size_t pos = std::distance(code.cbegin(), match[0].first);
            
            
            if (pos >= decryptor.length()) {
                std::string str = match[0].str();
                
                
                if (str.length() > 2 && str.find("__") == std::string::npos) {
                    
                    std::string content = str.substr(1, str.length() - 2);
                    if (content.find('\n') == std::string::npos && 
                        content.find('\r') == std::string::npos &&
                        content.length() < 1000) {  
                        
                        
                        std::string beforeMatch = code.substr(std::max<size_t>(0, pos - 100), 100);
                        std::string afterMatch = code.substr(pos, std::min<size_t>(match[0].length() + 100, code.length() - pos));
                        
                        
                        size_t lineStart = code.rfind('\n', pos);
                        if (lineStart == std::string::npos) lineStart = 0;
                        else lineStart++; 
                        
                        
                        bool isCommented = false;
                        size_t blockStart = code.rfind("\n--", pos);
                        if (blockStart != std::string::npos) {
                            
                            size_t nextNonComment = blockStart + 1;
                            while (nextNonComment < pos) {
                                nextNonComment = code.find('\n', nextNonComment);
                                if (nextNonComment == std::string::npos) break;
                                nextNonComment++;
                                
                                
                                if (nextNonComment < code.length() && 
                                    (code.substr(nextNonComment, 2) != "--")) {
                                    break;
                                }
                            }
                            
                            
                            if (nextNonComment > pos || nextNonComment == std::string::npos) {
                                isCommented = true;
                            }
                        }
                        
                        
                        if (!isCommented) {
                            std::string linePrefix = code.substr(lineStart, pos - lineStart);
                            isCommented = std::regex_match(linePrefix, std::regex(R"(^\s*--.*$)"));
                        }
                        
                        
                        bool isTableEntry = 
                            std::regex_search(beforeMatch, std::regex(R"(\{\s*$)")) ||
                            std::regex_search(beforeMatch, std::regex(R"(,\s*$)")) ||
                            std::regex_search(afterMatch, std::regex(R"(^\s*[,}])")) ||
                            std::regex_search(afterMatch, std::regex(R"(^\s*=)")) ||
                            std::regex_search(beforeMatch, std::regex(R"(\bkey\s*=\s*$)")) ||
                            std::regex_search(beforeMatch, std::regex(R"([a-zA-Z_][a-zA-Z0-9_]*\s*=\s*$)"));

                        
                        bool isInFunction = false;
                        try {
                            size_t contextStart = (pos > 200) ? pos - 200 : 0;
                            std::string context = code.substr(contextStart, pos - contextStart);
                            
                            
                            std::regex funcPattern(R"(([a-zA-Z_][a-zA-Z0-9_]*(?:\.[a-zA-Z_][a-zA-Z0-9_]*)*)\s*\([^{}\n]*$)");
                            std::smatch funcMatch;
                            
                            if (std::regex_search(context, funcMatch, funcPattern)) {
                                std::string funcName = funcMatch[1].str();
                                if (funcName != "if" && funcName != "while" && 
                                    funcName != "for" && funcName != "elseif" && 
                                    funcName.find("vec") != 0) {
                                    isInFunction = true;
                                }
                            }
                        } catch (const std::exception& e) {
                            Logger::warning("Error checking function context: " + std::string(e.what()));
                            isInFunction = false;
                        }

                        if (!isTableEntry && !isCommented && !isInFunction) {
                            strings.push_back({pos, str});
                        } else {
                            if (isCommented) {
                                
                            } else if (isTableEntry) {
                                
                            } else if (isInFunction) {
                                
                            }
                        }
                    }
                }
            }
            searchStart = match.suffix().first;
            progress.update(++processedCount);
        }
        
        progress.finish("Found " + std::to_string(strings.size()) + " strings");
        
        if (strings.empty()) {
            return;
        }
        
        
        std::sort(strings.rbegin(), strings.rend());
        
        
        ProgressBar encProgress(strings.size(), 50, "Encrypting strings");
        processedCount = 0;
        std::vector<std::string> encryptedStrings;
        
        for (const auto& [pos, str] : strings) {
            try {
                
                std::string content = str.substr(1, str.length() - 2);
                std::string varName = "__str_" + std::to_string(processedCount);
                
                
                std::string encrypted = encryptBytes(content, key, varName, chunkSize);
                std::string replacement = "__decrypt(" + varName + ", __key)";
                
                encryptedStrings.push_back(encrypted);
                code.replace(pos, str.length(), replacement);
                
                encProgress.update(++processedCount);
                
            } catch (const std::exception& e) {
                Logger::error("Failed to process string at position " + std::to_string(pos) + ": " + e.what());
            }
        }
        
        encProgress.finish("Completed");
        
        
        std::string allEncrypted;
        for (const auto& encrypted : encryptedStrings) {
            allEncrypted += encrypted + "\n";
        }
        
        if (!allEncrypted.empty()) {
            code.insert(decryptor.length(), allEncrypted);
        }
        
    } catch (const std::exception& e) {
        Logger::error("String encryption failed: " + std::string(e.what()));
        throw;
    }
} 