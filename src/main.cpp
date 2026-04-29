#include <iostream>
#include <fstream>
#include <sstream>

#include "Lexer/Lexer.h"
#include "ASTBuilder/ASTBuilder.h"
#include "Compiler/Compiler.h"
#include "VirtualMachine/VirtualMachine.h"

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: ./MyLanguage <filename>" << std::endl;
        return 1;
    }

    std::ifstream file(argv[1]);
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string sourceCode = buffer.str();

    Lexer lexer(sourceCode);
    auto tokens = lexer.Tokenize();

    ASTBuilder parser(tokens);
    auto ast = parser.Parse();

    Compiler compiler;
    FunctionPtr mainFunc = compiler.Compile(ast);

    VirtualMachine vm;
    vm.Interpret(mainFunc);

    return 0;
}