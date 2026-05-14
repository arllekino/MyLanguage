cmake_minimum_required(VERSION 3.30)
project(MyLanguage)

set(CMAKE_CXX_STANDARD 20)

include(FetchContent)
FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.5.0
)
FetchContent_MakeAvailable(Catch2)

add_executable(MyLanguage
        src/main.cpp
        src/VirtualMachine/Value/Value.h
        src/VirtualMachine/OpCode.h
        src/VirtualMachine/Chunk.h
        src/VirtualMachine/VirtualMachine.h
        src/VirtualMachine/Value/ValueUtilsForLogic.h
        src/VirtualMachine/Value/ValueUtilsForPrint.h
        src/VirtualMachine/ByteCodeParser.h
        src/Lexer/Lexer.h
        src/Lexer/TokenType.h
        src/Lexer/Token.h
        src/ASTBuilder/ASTBuilder.h
        src/ASTBuilder/AST.h
        src/Compiler/Compiler.h
        src/Utils/Parameter.h
        test/array/2d_array.txt
        src/Compiler/ByteCodeExporter.h
        src/Utils/TypeInfo.h
        src/Signature/Signature.h
        src/ASM/ASMGenerator.h
)

add_executable(MyLanguageTest test/test.cpp)

target_link_libraries(MyLanguageTest PRIVATE Catch2::Catch2WithMain)

include(CTest)
include(Catch)
catch_discover_tests(MyLanguageTest)