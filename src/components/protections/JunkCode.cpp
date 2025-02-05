#include "JunkCode.hpp"
#include <random>
#include <sstream>
#include <vector>

std::mt19937& JunkCode::getGenerator() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    return gen;
}

std::string JunkCode::generateRandomString(int length) {
    const std::string charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    std::uniform_int_distribution<> dis(0, charset.length() - 1);
    std::string result;
    for (int i = 0; i < length; ++i) {
        result += charset[dis(getGenerator())];
    }
    return result;
}

std::string JunkCode::generate(int count) {
    std::vector<std::string> junkTemplates = {
        "local __{} = {}",
        "local __{} = string.rep('x', {})",
        "local __{} = {{}, {}, {}}",
        "local __{} = math.random(1, {})",
        "local __{} = table.concat({{}, {}, {}})",
        "local __{} = bit32 and bit32.bxor({}, {}) or {}",
    };

    std::stringstream ss;
    std::uniform_int_distribution<> templateDis(0, junkTemplates.size() - 1);
    std::uniform_int_distribution<> numDis(1, 1000);

    for (int i = 0; i < count; ++i) {
        std::string varName = generateRandomString(8);
        std::string templ = junkTemplates[templateDis(getGenerator())];
        
        size_t pos = templ.find("{}");
        while (pos != std::string::npos) {
            templ.replace(pos, 2, std::to_string(numDis(getGenerator())));
            pos = templ.find("{}", pos + 1);
        }
        
        ss << templ << "\n";
    }

    return ss.str();
} 