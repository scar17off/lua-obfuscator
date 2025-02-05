cmake_minimum_required(VERSION 3.10)
project(lua_obfuscator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
    src/main.cpp
    src/LuaObfuscator.cpp
    src/components/ConfigParser.cpp
    src/components/Logger.cpp
    src/components/protections/StringEncryption.cpp
    src/components/protections/VMProtection.cpp
    src/components/protections/JunkCode.cpp
    src/components/protections/ControlFlow.cpp
    src/components/protections/Compression.cpp
    src/components/ProgressBar.cpp
)

add_executable(obfuscator ${SOURCES})

target_include_directories(obfuscator PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/config.ini
    ${CMAKE_CURRENT_BINARY_DIR}/config.ini
    COPYONLY
)

add_custom_command(
    TARGET obfuscator POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/config.ini
        $<TARGET_FILE_DIR:obfuscator>/config.ini
)