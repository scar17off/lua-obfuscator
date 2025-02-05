#include "ControlFlow.hpp"
#include <sstream>
#include <map>
#include <set>
#include "../Logger.hpp"

std::set<int> ControlFlow::validStates;

std::mt19937& ControlFlow::getGenerator() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    return gen;
}

int ControlFlow::generateRandomState() {
    static std::uniform_int_distribution<> dis(1000, 9999);
    return dis(getGenerator());
}

std::vector<int> ControlFlow::generateStates(const ConfigParser& config) {
    std::vector<int> states;
    int tableSize = config.getIntValue("ControlFlow", "jump_table_size", 10);
    
    
    for (int i = 0; i < tableSize; ++i) {
        states.push_back(generateRandomState());
    }
    
    return states;
}

std::string ControlFlow::generateJumpTable(const std::vector<int>& states) {
    std::stringstream ss;
    ss << "local __jumptable = {\n";
    
    for (size_t i = 0; i < states.size(); ++i) {
        ss << "    [" << states[i] << "] = function(__next)\n";
        ss << "        if __debug then return nil end\n";
        ss << "        local nextState = " << states[(i + 1) % states.size()] << "\n";
        ss << "        return __next(nextState)\n";
        ss << "    end,\n";
    }
    ss << "}\n\n";
    
    return ss.str();
}

std::string ControlFlow::generateDispatcher() {
    std::stringstream ss;
    ss << "local function __dispatch()\n";
    ss << "    local __debug = false\n";
    ss << "    if not __state then return nil end\n";
    ss << "    local __jumps = 0\n";
    ss << "    local MAX_JUMPS = 100\n";
    ss << "    local function __next(newState)\n";
    ss << "        __jumps = __jumps + 1\n";
    ss << "        if __jumps > MAX_JUMPS then return false end\n";
    ss << "        __state = newState\n";
    ss << "        return true\n";
    ss << "    end\n";
    ss << "    while true do\n";
    ss << "        if __jumptable[__state] then\n";
    ss << "            local ok = __jumptable[__state](__next)\n";
    ss << "            if not ok then return nil end\n";
    ss << "        elseif __handlers[__state] then\n";
    ss << "            local result = __handlers[__state](__next)\n";
    ss << "            if result ~= true then return result end\n";
    ss << "        else\n";
    ss << "            return nil\n";
    ss << "        end\n";
    ss << "    end\n";
    ss << "end\n\n";
    return ss.str();
}

std::string ControlFlow::generateStateHandler(int state, const std::string& code, const ConfigParser& config) {
    std::stringstream ss;
    ss << "    [" << state << "] = function(__next)\n";
    validStates.insert(state);
    
    auto getValidState = []() {
        auto& states = validStates;
        auto it = states.begin();
        std::advance(it, std::uniform_int_distribution<>(0, states.size() - 1)(getGenerator()));
        return *it;
    };

    
    ss << "        if __state == " << state << " then\n";  
    ss << "            local key = __key\n";
    ss << "            local decrypt = __decrypt\n";
    ss << "            local chunk = load([[\n";
    ss << "                local __key = ...\n";
    ss << "                local __decrypt = select(2, ...)\n";
    ss << "                if not __key then return nil end\n";
    ss << "                if not __decrypt then return nil end\n";
    ss << "                " << code << "\n";
    ss << "                local decrypted = __code\n";
    ss << "                local f, err = load(decrypted, '@', 't', _G)\n";
    ss << "                if not f then return nil end\n";
    ss << "                return f()\n";
    ss << "            ]], '@')\n";
    ss << "            if not chunk then return nil end\n";
    ss << "            local ok, result = pcall(chunk, key, decrypt)\n";
    ss << "            if not ok then return nil end\n";
    ss << "            if result ~= nil then return result end\n";
    ss << "        end\n";
    
    
    if (config.getBoolValue("ControlFlow", "debug_traps", true)) {
        std::uniform_int_distribution<> condition_dis(0, 3);
        int condition_type = condition_dis(getGenerator());
        
        switch (condition_type) {
            case 0:
                ss << "        if __debug then return __next(" << getValidState() << ") end\n";
                break;
            case 1:
                if (config.getBoolValue("ControlFlow", "gc_hooks", true)) {
                    ss << "        if collectgarbage('count') > 0 then\n";
                    ss << "            return __next(" << getValidState() << ")\n";
                    ss << "        end\n";
                }
                break;
            case 2:
                ss << "        if _VERSION ~= _VERSION then return __next(" << getValidState() << ") end\n";
                break;
            case 3:
                ss << "        if type(_G) ~= 'table' then return __next(" << getValidState() << ") end\n";
                break;
        }
    }
    
    ss << "        return __next(" << getValidState() << ")\n";
    ss << "    end,\n";
    return ss.str();
}

std::string ControlFlow::generateFakeStates(const ConfigParser& config) {
    std::stringstream ss;
    int fakeCount = config.getIntValue("ControlFlow", "fake_states", 15);

    auto getValidState = []() {
        auto& states = validStates;  
        auto it = states.begin();
        std::advance(it, std::uniform_int_distribution<>(0, states.size() - 1)(getGenerator()));
        return *it;
    };

    for (int i = 0; i < fakeCount; i++) {
        int state = generateRandomState();
        validStates.insert(state);  
        ss << "    [" << state << "] = function(__next)\n";
        ss << "        if __debug then\n";
        ss << "            return __next(" << getValidState() << ")\n";
        ss << "        end\n";
        ss << "        return nil\n";
        ss << "    end,\n";
    }
    return ss.str();
}

std::string ControlFlow::generateCoroutineWrapper() {
    std::stringstream ss;
    ss << "local function __wrap(f)\n";
    ss << "    return function(...)\n";
    ss << "        local co = coroutine.create(f)\n";
    ss << "        local success, result\n";
    ss << "        repeat\n";
    ss << "            success, result = coroutine.resume(co, ...)\n";
    ss << "            if not success then\n";
    ss << "                error(result)  -- Propagate error directly\n";
    ss << "            end\n";
    ss << "        until coroutine.status(co) == 'dead'\n";
    ss << "        return result\n";
    ss << "    end\n";
    ss << "end\n\n";
    return ss.str();
}

std::string ControlFlow::generateVM() {
    
    return "";
}

std::string ControlFlow::scramble(const std::string& code, const ConfigParser& config) {
    Logger::info("Original code length: " + std::to_string(code.length()));
    std::stringstream ss;
    
    
    int mainState = generateRandomState();
    Logger::info("Main state ID: " + std::to_string(mainState));
    
    ss << "local __state = " << mainState << "\n\n";
    
    
    Logger::info("Generating jump table...");
    std::vector<int> states = generateStates(config);
    ss << generateJumpTable(states);
    
    
    Logger::info("Generating state handlers...");
    ss << "local __handlers = {\n";
    
    
    ss << "    [" << mainState << "] = function(__next)\n";
    ss << "        if __state == " << mainState << " then\n";
    ss << "            local key = __key\n";
    ss << "            local decrypt = __decrypt\n";
    ss << "            local chunk = load([[\n";
    ss << "                local __key = ...\n";
    ss << "                local __decrypt = select(2, ...)\n";
    ss << "                if not __key then return nil end\n";
    ss << "                if not __decrypt then return nil end\n";
    ss << "                " << code;
    ss << "\n                local decrypted = __code\n";
    ss << "                local f, err = load(decrypted, '@', 't', _G)\n";
    ss << "                if not f then return nil end\n";
    ss << "                _G.__key = __key\n";
    ss << "                local result = f()\n";
    ss << "                _G.__key = nil\n";
    ss << "                return result\n";
    ss << "            ]], '@')\n";
    ss << "            if not chunk then return nil end\n";
    ss << "            local ok, result = pcall(chunk, key, decrypt)\n";
    ss << "            if not ok then return nil end\n";
    ss << "            if result ~= nil then return result end\n";
    ss << "        end\n";
    ss << "        return __next(" << states[0] << ")\n";
    ss << "    end,\n";
    
    
    Logger::info("Generating fake states...");
    int numFakeStates = config.getIntValue("ControlFlow", "fake_states", 15);
    for (int i = 0; i < numFakeStates; ++i) {
        int state = generateRandomState();
        int nextState = states[rand() % states.size()];
        ss << "    [" << state << "] = function(__next)\n";
        ss << "        if __debug then return __next(" << nextState << ") end\n";
        ss << "        return nil\n";
        ss << "    end,\n";
    }
    ss << "}\n\n";
    
    
    Logger::info("Setting up dispatcher...");
    ss << generateDispatcher();
    
    ss << "return __dispatch()\n";
    
    std::string result = ss.str();
    Logger::info("Scrambled code length: " + std::to_string(result.length()));
    Logger::info("Final code length: " + std::to_string(result.length()));
    
    return result;
} 