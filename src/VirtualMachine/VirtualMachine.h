#pragma once
#include <iostream>
#include <vector>
#include <variant>
#include "./Value/Value.h"
#include "OpCode.h"
#include "./Chunk.h"
#include "./Value/ValueUtilsForLogic.h"
#include "Value/ValueUtilsForPrint.h"

class VirtualMachine
{
public:
    VirtualMachine()
    {
        m_stack.reserve(STACK_MAX);
    }

    void Interpret(Chunk* chunk)
    {
        m_currentChunk = chunk;
        m_ip = 0;
        m_locals.clear();
        m_locals.resize(256);

        for (size_t i = 0; i < m_stack.size(); ++i)
        {
            m_locals[i] = m_stack[i];
        }
        m_stack.clear();

        Run();
    }

    void Push(const Value& value) // TODO: убрать из public
    {
        m_stack.push_back(value);
    }

private:
    static constexpr int STACK_MAX = 256;

    Chunk* m_currentChunk{};
    size_t m_ip{};
    std::vector<Value> m_stack;
    std::vector<Value> m_locals;

    uint8_t ReadByte()
    {
        return m_currentChunk->code[m_ip++];
    }

    uint16_t ReadShort()
    {
        const uint8_t high = ReadByte();
        const uint8_t low = ReadByte();
        return static_cast<uint16_t>((high << 8) | low);
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
            {
                if (!m_stack.empty())
                {
                    const auto result = Pop();
                    PrintValue(result);
                    std::cout << std::endl;
                }
                return;
            }

            case OP_ADD:
            {
                ExecuteAddOperation();
                break;
            }
            case OP_SUB:
            {
                ExecuteSubOperation();
                break;
            }
            case OP_MUL:
            {
                ExecuteMulOperation();
                break;
            }
            case OP_DIV:
            {
                ExecuteDivOperation();
                break;
            }
            case OP_MOD:
            {
                ExecuteModOperation();
                break;
            }
            case OP_NEG:
            {
                ExecuteNegOperation();
                break;
            }

            case OP_GREATER:
            {
                ExecuteOperationGreater();
                break;
            }
            case OP_LESS:
            {
                ExecuteOperationLess();
                break;
            }
            case OP_EQUAL:
            {
                ExecuteOperationEqual();
                break;
            }
            case OP_NOT:
            {
                ExecuteOperationNot();
                break;
            }

            case OP_JUMP_IF_FALSE:
            {
                ExecuteOperationJumpIfFalse();
                break;
            }
            case OP_JUMP:
            {
                ExecuteOperationJump();
                break;
            }
            case OP_LOOP:
            {
                ExecuteOperationLoop();
                break;
            }

            case OP_GET_LOCAL:
            {
                ExecuteOperationGetLocal();
                break;
            }
            case OP_SET_LOCAL:
            {
                ExecuteOperationSetLocal();
                break;
            }
            case OP_CONSTANT:
            {
                ExecuteOperationConstant();
                break;
            }

            case OP_BUILD_ARRAY:
            {
                ExecuteOperationBuildArray();
                break;
            }
            case OP_GET_INDEX:
            {
                ExecuteOperationGetIndex();
                break;
            }
            case OP_SET_INDEX:
            {
                ExecuteOperationSetIndex();
                break;
            }
            case OP_ARRAY_LEN:
            {
                ExecuteOperationArrayLen();
                break;
            }

            case OP_ARRAY_PUSH:
            {
                ExecuteOperationArrayPush();
                break;
            }
            case OP_POP:
            {
                Pop();
                break;
            }
            case OP_PRINT:
            {
                ExecuteOperationPrint();
                break;
            }

            default:
            {
                std::cerr << "Unknown opcode: " << static_cast<int>(opcode) << std::endl;
                return;
            }
        }
    }
}

    void ExecuteAddOperation()
    {
        const Value left = Pop();
        const Value right = Pop();

        m_stack.push_back(left + right);
    }

    void ExecuteOperationConstant()
    {
        const auto index = ReadByte();
        Value constant = m_currentChunk->constants[index];
        Push(constant);
    }

    void ExecuteOperationGreater()
    {
        const Value right = Pop();
        const Value left = Pop();
        Push(left > right);
    }

    void ExecuteOperationJumpIfFalse()
    {
        uint16_t targetAddress = ReadShort();

        if (Value condition = Pop(); std::holds_alternative<bool>(condition))
        {
            if (!std::get<bool>(condition))
            {
                m_ip = targetAddress;
            }
        }
        else
        {
            throw std::runtime_error("Condition must be a boolean");
        }
    }
    void ExecuteOperationJump()
    {
        uint16_t address = ReadShort();
        m_ip = address;
    }

    void ExecuteOperationLoop()
    {
        uint16_t address = ReadShort();
        m_ip = address;
    }

    void ExecuteOperationGetLocal()
    {
        uint8_t slot = ReadByte();
        Push(m_locals[slot]);
    }

    void ExecuteOperationSetLocal()
    {
        uint8_t slot = ReadByte();
        m_locals[slot] = m_stack.back();
    }

    void ExecuteOperationGetIndex()
    {
        Value indexVal = Pop();
        Value arrayVal = Pop();

        auto arrayPtr = std::get<ArrayPtr>(arrayVal);
        int64_t index = std::get<int64_t>(indexVal);

        Push(arrayPtr->values[index]);
    }

    void ExecuteOperationSetIndex()
    {
        Value value = Pop();
        Value indexVal = Pop();
        Value arrayVal = Pop();

        auto arrayPtr = std::get<ArrayPtr>(arrayVal);
        int64_t index = std::get<int64_t>(indexVal);

        arrayPtr->values[index] = value;
        Push(value);
    }

    void ExecuteOperationPrint() const
    {
        const Value value = m_stack.back();
        PrintValue(value);
    }

    void ExecuteSubOperation()
    {
        const Value right = Pop();
        const Value left = Pop();
        Push(left - right);
    }

    void ExecuteMulOperation()
    {
        const Value right = Pop();
        const Value left = Pop();
        Push(left * right);
    }

    void ExecuteDivOperation()
    {
        const Value right = Pop();
        const Value left = Pop();
        Push(left / right);
    }

    void ExecuteModOperation()
    {
        const Value right = Pop();
        const Value left = Pop();

        Push(left % right);
    }

    void ExecuteNegOperation()
    {
        Push(-Pop());
    }

    void ExecuteOperationLess()
    {
        const Value right = Pop();
        const Value left = Pop();
        Push(left < right);
    }

    void ExecuteOperationEqual()
    {
        const Value right = Pop();
        const Value left = Pop();
        Push(left == right);
    }

    void ExecuteOperationNot()
    {
        const Value val = Pop();
        if (std::holds_alternative<bool>(val))
        {
            Push(!std::get<bool>(val));
        }
        else
        {
            throw std::invalid_argument("OP_NOT applies only to booleans");
        }
    }

    void ExecuteOperationBuildArray()
    {
        auto newArray = std::make_shared<Array>();
        Push(newArray);
    }

    void ExecuteOperationArrayLen()
    {
        const Value val = Pop();
        if (std::holds_alternative<ArrayPtr>(val))
        {
            auto arr = std::get<ArrayPtr>(val);
            Push(static_cast<int64_t>(arr->values.size()));
        }
        else
        {
            throw std::invalid_argument("OP_ARRAY_LEN applies only to arrays");
        }
    }

    void ExecuteOperationArrayPush()
    {
        Value value = Pop();
        Value arrayVal = Pop();

        if (std::holds_alternative<ArrayPtr>(arrayVal))
        {
            auto arrayPtr = std::get<ArrayPtr>(arrayVal);
            arrayPtr->values.push_back(value);
            Push(value);
        }
        else
        {
            throw std::invalid_argument("OP_ARRAY_PUSH applies only to arrays");
        }
    }
};
