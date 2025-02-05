#include "VMProtection.hpp"
#include <sstream>
#include "ControlFlow.hpp"
#include "../Logger.hpp"

std::string VMProtection::generateVM() {
    std::stringstream ss;
    
    ss << "local function __createVM()\n";
    ss << "    local env = setmetatable({}, {__index = _ENV})\n";
    ss << "    return function(code)\n";
    ss << "        local f = load(code, nil, 't', env)\n";
    ss << "        if not f then error('Code error') end\n";
    ss << "        return f()\n";
    ss << "    end\n";
    ss << "end\n\n";

    return ss.str();
}

std::string VMProtection::encryptCode(const std::string& code, const std::vector<uint8_t>& key, size_t chunkSize) {
    std::stringstream ss;
    
    
    std::vector<std::string> chunks;
    for (size_t i = 0; i < code.length(); i += chunkSize * 5) {
        chunks.push_back(code.substr(i, std::min<size_t>(chunkSize * 5, code.length() - i)));
    }

    
    for (size_t i = 0; i < chunks.size(); ++i) {
        const std::string& chunk = chunks[i];
        
        
        std::vector<std::string> subChunkVars;
        for (size_t j = 0; j < 5 && j * chunkSize < chunk.length(); ++j) {
            size_t subStart = j * chunkSize;
            size_t subLen = std::min<size_t>(chunkSize, chunk.length() - subStart);
            if (subLen == 0) break;
            
            std::string subChunk = chunk.substr(subStart, subLen);
            std::string varName = std::string("__enc_") + std::to_string(i) + "_" + std::to_string(j);
            ss << "local " << varName << " = string.char(";
            
            
            for (size_t k = 0; k < subChunk.length(); ++k) {
                if (k > 0) ss << ",";
                uint8_t byte = subChunk[k];
                
                byte ^= key[(k + subStart) % key.size()];
                byte = (byte << 3) | (byte >> 5); 
                ss << (int)byte;
            }
            ss << ")\n";
            subChunkVars.push_back(varName);
        }
        
        
        ss << "local __encrypted_" << i << " = string.format('%s%s%s%s%s',\n";
        for (size_t j = 0; j < subChunkVars.size(); ++j) {
            ss << "    " << subChunkVars[j];
            if (j < subChunkVars.size() - 1) ss << ",\n";
        }
        
        for (size_t j = subChunkVars.size(); j < 5; ++j) {
            ss << ",\n    ''";
        }
        ss << ")\n";
    }

    
    ss << "local __code = string.format('%s%s%s%s',\n";
    for (size_t i = 0; i < chunks.size(); ++i) {
        ss << "    __decrypt(__encrypted_" << i << ", __key)";
        if (i < chunks.size() - 1) ss << ",\n";
    }
    
    for (size_t i = chunks.size(); i < 4; ++i) {
        ss << ",\n    ''";
    }
    ss << ")\n\n";

    return ss.str();
}

std::string VMProtection::wrapCode(const std::string& code, const std::vector<uint8_t>& key, size_t chunkSize, const ConfigParser& config) {
    Logger::info("Starting VM protection...");
    Logger::info("Input code length: " + std::to_string(code.length()));
    
    std::stringstream ss;
    bool compressionEnabled = config.getBoolValue("Compression", "enabled", false);
    
    
    std::string vmCode;
    if (compressionEnabled) {
        vmCode = "local function __createVM()local env=setmetatable({},{__index=_ENV})return function(code)local f=load(code,nil,'t',env)if not f then error('Code error')end return f()end end\n";
    } else {
        vmCode = generateVM();
    }
    
    ss << vmCode;

    
    ss << "local __key = {";
    for (size_t i = 0; i < key.size(); ++i) {
        if (i > 0) ss << ",";
        ss << (int)key[i];
    }
    ss << "}\n\n";

    
    if (compressionEnabled) {
        ss << "_G.__decrypt=function(str,key)if not str then return\"\"end if type(str)~='string'then return\"\"end if #str==0 then return\"\"end local result={}for i=1,#str do local byte=str:byte(i)byte=(byte>>3)|(byte<<5)&0xFF byte=byte~key[(i-1)%#key+1]result[i]=string.char(byte)end return table.concat(result)end\n";
    } else {
        ss << "_G.__decrypt = function(str, key)\n"
           << "    if not str then return \"\" end\n"
           << "    if type(str) ~= 'string' then return \"\" end\n"
           << "    if #str == 0 then return \"\" end\n"
           << "    local result = {}\n"
           << "    for i = 1, #str do\n"
           << "        local byte = str:byte(i)\n"
           << "        byte = (byte >> 3) | (byte << 5) & 0xFF\n"
           << "        byte = byte ~ key[(i-1) % #key + 1]\n"
           << "        result[i] = string.char(byte)\n"
           << "    end\n"
           << "    return table.concat(result)\n"
           << "end\n\n";
    }
    
    
    std::string encryptedCode = encryptCode(code, key, chunkSize);
    Logger::info("Encrypted code length: " + std::to_string(encryptedCode.length()));
    
    
    std::string scrambledCode = ControlFlow::scramble(encryptedCode, config);
    
    ss << scrambledCode;
    
    Logger::info("VM protection completed");
    return ss.str();
} 