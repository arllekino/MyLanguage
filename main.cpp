#include <iostream>
#include <vector>
#include <variant>
#include "./Value.h"

enum OpCode : uint8_t
{
    OP_CONSTANT = 0,
    OP_RETURN   = 255
};

struct Chunk
{
    std::vector<uint8_t> code;
    std::vector<Value> constants;
};

class VM
{
public:
    VM()
    {
        m_stack.reserve(STACK_MAX);
    }

    void Interpret(Chunk* chunk)
    {
        m_currentChunk = chunk;
        m_ip = 0;
        Run();
    }

private:
    static constexpr int STACK_MAX = 256;

    Chunk* m_currentChunk{};
    size_t m_ip{};
    std::vector<Value> m_stack;

    uint8_t ReadByte()
    {
        return m_currentChunk->code[m_ip++];
    }

    void Push(const Value& value)
    {
        m_stack.push_back(value);
    }

    Value Pop()
    {
        const Value val = m_stack.back();
        m_stack.pop_back();
        return val;
    }

    void Run()
    {
        for (;;)
        {
            switch (const auto opcode = ReadByte())
            {
                case OP_RETURN:
                    if (!m_stack.empty())
                    {
                        const auto result = Pop();
                        std::cout << std::get<double>(result) << std::endl;
                    }
                    return;

                case OP_CONSTANT:
                    const auto index = ReadByte();
                    Value constant = m_currentChunk->constants[index];
                    Push(constant);
                    break;

                default:
                    std::cerr << "Unknown opcode: " << static_cast<int>(opcode) << std::endl;
                    return;
            }
        }
    }
};

int main()
{
    VM vm;
    Chunk chunk;

    chunk.constants.emplace_back(42.5);
    chunk.code.push_back(OP_CONSTANT);
    chunk.code.push_back(0);

    chunk.code.push_back(OP_RETURN);

    vm.Interpret(&chunk);

    return 0;
}