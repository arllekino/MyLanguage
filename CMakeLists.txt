cmake_minimum_required(VERSION 3.30)
project(MyLanguage)

set(CMAKE_CXX_STANDARD 20)

add_executable(MyLanguage
        src/main.cpp
        src/VirtualMachine/Value/Value.h
        src/VirtualMachine/OpCode.h
        src/VirtualMachine/Chunk.h
        src/VirtualMachine/VirtualMachine.h
        src/VirtualMachine/Value/ValueUtilsForLogic.h
        src/VirtualMachine/Value/ValueUtilsForPrint.h
        src/ByteCodeParser.h
        src/Lexer/Lexer.h
        src/Lexer/TokenType.h
        src/Lexer/Token.h
        src/ASTBuilder/ASTBuilder.h
        src/ASTBuilder/AST.h
        src/Compiler/Compiler.h
)
