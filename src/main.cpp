#include <iostream>

#include "ByteCodeParser.h"
#include "./VirtualMachine/VirtualMachine.h"

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: ./VirtualMachine <filename>" << std::endl;
        return 1;
    }

    auto byteCodeParser = ByteCodeParser(argv[1]);
    Chunk chunk = byteCodeParser.Parse();

    VirtualMachine vm;
    vm.Interpret(&chunk);

    return 0;
}