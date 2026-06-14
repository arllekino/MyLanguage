#include <iostream>
#include <fstream>
#include <sstream>

#include "Lexer/Lexer.h"
#include "ASTBuilder/ASTBuilder.h"
#include "Compiler/Compiler.h"
#include "Compiler/ByteCodeExporter.h"
#include "VirtualMachine/ByteCodeParser.h"
#include "VirtualMachine/VirtualMachine.h"

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: ./MyLanguage <filename.txt | filename.bc>" << std::endl;
        return 1;
    }

    std::string filename = argv[1];

    try
    {
        FunctionPtr mainFunc = nullptr;

        if (filename.ends_with(".rocket") || filename.ends_with(".txt"))
        {
            std::ifstream file(filename);
            std::stringstream buffer;
            buffer << file.rdbuf();
            std::string sourceCode = buffer.str();

            Lexer lexer(sourceCode);
            auto tokens = lexer.Tokenize();

            ASTBuilder parser(tokens);
            auto ast = parser.Parse();

            Compiler compiler;
            FunctionPtr compiledFunc = compiler.Compile(ast, filename);

            std::string bcFilename = filename.substr(0, filename.find_last_of('.')) + ".bc";
            ByteCodeExporter exporter;
            exporter.Export(compiledFunc, bcFilename);

            filename = bcFilename;
        }

        if (filename.ends_with(".bc"))
        {
            ByteCodeParser bcParser(filename);
            mainFunc = bcParser.Parse();

            if (!mainFunc)
            {
                throw std::runtime_error("No 'main' function found in bytecode.");
            }

            VirtualMachine vm;
            vm.Interpret(mainFunc);
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "\033[1;31mError:\033[0m " << e.what() << std::endl;
        return 1;
    }

    return 0;
}