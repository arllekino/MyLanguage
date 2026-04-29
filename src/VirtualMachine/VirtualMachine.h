#pragma once
#include <iostream>
#include <vector>
#include <random>
#include <unordered_map>
#include <stdexcept>
#include "Value/Value.h"
#include "OpCode.h"
#include "Chunk.h"
#include "Value/ValueUtilsForLogic.h"
#include "Value/ValueUtilsForPrint.h"

struct CallFrame
{
    FunctionPtr function;
    size_t ip = 0;
    size_t baseSlot = 0;
};

class VirtualMachine
{
public:
    VirtualMachine()
    {
        m_stack.reserve(STACK_MAX);
        DefineNativeFunctions();
    }

    void DefineGlobal(const std::string& name, const Value& value)
    {
        m_globals[name] = value;
    }

    void Interpret(FunctionPtr mainFunc)
    {
        m_stack.clear();
        m_frames.clear();

        Push(mainFunc);

        CallFrame mainFrame;
        mainFrame.function = mainFunc;
        mainFrame.ip = 0;
        mainFrame.baseSlot = 1;
        m_frames.push_back(mainFrame);

        for(int i = 0; i < 20; i++) Push(false);

        Run();
    }

private:
    static constexpr int STACK_MAX = 2048;

    std::vector<Value> m_stack;
    std::vector<CallFrame> m_frames;
    std::unordered_map<std::string, Value> m_globals;

    uint8_t ReadByte() {
        return m_frames.back().function->chunk->code[m_frames.back().ip++];
    }

    uint16_t ReadShort() {
        uint8_t high = ReadByte();
        uint8_t low = ReadByte();
        return static_cast<uint16_t>((high << 8) | low);
    }

    Value ReadConstant() {
        return m_frames.back().function->chunk->constants[ReadByte()];
    }

    Value Pop() {
        Value val = m_stack.back();
        m_stack.pop_back();
        return val;
    }

    void Push(const Value& value) {
        m_stack.push_back(value);
    }

    template<typename T>
    T Expect(const Value& val, const std::string& errorMessage)
    {
        if (std::holds_alternative<T>(val))
        {
            return std::get<T>(val);
        }
        throw std::runtime_error("Type mismatch: " + errorMessage);
    }

    void Run()
    {
        for (;;) {
            switch (const auto opcode = ReadByte())
            {
                case OP_CONSTANT:
                {
                    Push(ReadConstant());
                    break;
                }
                case OP_GET_GLOBAL:
                {
                    Value nameVal = ReadConstant();
                    std::string name = *Expect<StringPtr>(nameVal, "Global name must be a string");
                    if (m_globals.contains(name))
                    {
                        Push(m_globals[name]);
                    }
                    else
                    {
                        throw std::runtime_error("Undefined global: " + name);
                    }
                    break;
                }
                case OP_GET_LOCAL:
                {
                    uint8_t slot = ReadByte();
                    Push(m_stack[m_frames.back().baseSlot + slot]);
                    break;
                }
                case OP_SET_LOCAL:
                {
                    uint8_t slot = ReadByte();
                    m_stack[m_frames.back().baseSlot + slot] = m_stack.back();
                    break;
                }
                case OP_CALL:
                {
                    int argCount = ReadByte();
                    Value callee = m_stack[m_stack.size() - 1 - argCount];

                    if (std::holds_alternative<FunctionPtr>(callee))
                    {
                        FunctionPtr func = std::get<FunctionPtr>(callee);
                        if (argCount != func->arity)
                        {
                            throw std::runtime_error("Expected " + std::to_string(func->arity) + " args but got " + std::to_string(argCount));
                        }
                        CallFrame frame;
                        frame.function = func;
                        frame.ip = 0;
                        frame.baseSlot = m_stack.size() - argCount;
                        m_frames.push_back(frame);

                        for(int i = 0; i < 20; i++) Push(false);
                    }
                    else if (std::holds_alternative<NativeFnPtr>(callee))
                    {
                        NativeFnPtr native = std::get<NativeFnPtr>(callee);
                        std::vector<Value> args;
                        args.reserve(argCount);
                        for (int i = 0; i < argCount; ++i)
                        {
                            args.push_back(m_stack[m_stack.size() - argCount + i]);
                        }
                        Value result = native->func(args);
                        m_stack.erase(m_stack.end() - argCount - 1, m_stack.end());
                        Push(result);
                    }
                    else
                    {
                        throw std::runtime_error("Can only call functions and native functions.");
                    }
                    break;
                }
                case OP_RETURN:
                {
                    Value result = Pop();
                    size_t baseSlot = m_frames.back().baseSlot;
                    m_frames.pop_back();

                    if (m_frames.empty())
                    {
                        return;
                    }

                    m_stack.erase(m_stack.begin() + baseSlot - 1, m_stack.end());
                    Push(result);
                    break;
                }
                case OP_ADD:
                {
                    const Value r = Pop();
                    const Value l = Pop();
                    Push(l + r);
                    break;
                }
                case OP_SUB:
                {
                    const Value r = Pop();
                    const Value l = Pop();
                    Push(l - r);
                    break;
                }
                case OP_MUL:
                {
                    const Value r = Pop();
                    const Value l = Pop();
                    Push(l * r);
                    break;
                }
                case OP_DIV:
                {
                    const Value r = Pop();
                    const Value l = Pop();
                    Push(l / r);
                    break;
                }
                case OP_MOD:
                {
                    const Value r = Pop();
                    const Value l = Pop();
                    Push(l % r);
                    break;
                }
                case OP_LESS:
                {
                    const Value r = Pop();
                    const Value l = Pop();
                    Push(l < r);
                    break;
                }
                case OP_GREATER:
                {
                    const Value r = Pop();
                    const Value l = Pop();
                    Push(l > r);
                    break;
                }
                case OP_EQUAL:
                {
                    const Value r = Pop();
                    const Value l = Pop();
                    Push(l == r);
                    break;
                }
                case OP_NOT:
                {
                    Value val = Pop();
                    if (std::holds_alternative<bool>(val))
                    {
                        Push(!std::get<bool>(val));
                    }
                    else
                    {
                        throw std::runtime_error("OP_NOT expects a boolean value");
                    }
                    break;
                }
                case OP_JUMP_IF_FALSE:
                {
                    uint16_t target = ReadShort();
                    Value cond = Pop();
                    if (std::holds_alternative<bool>(cond) && !std::get<bool>(cond))
                    {
                        m_frames.back().ip = target;
                    }
                    break;
                }
                case OP_JUMP:
                {
                    m_frames.back().ip = ReadShort();
                    break;
                }
                case OP_BUILD_ARRAY:
                {
                    Push(std::make_shared<Array>());
                    break;
                }
                case OP_ARRAY_PUSH:
                {
                    Value val = Pop();
                    Value arr = Pop();
                    auto arrayPtr = Expect<ArrayPtr>(arr, "OP_ARRAY_PUSH expects array");
                    arrayPtr->values.push_back(val);
                    Push(val);
                    break;
                }
                case OP_ARRAY_LEN:
                {
                    Value arr = Pop();
                    auto arrayPtr = Expect<ArrayPtr>(arr, "OP_ARRAY_LEN expects array");
                    Push(static_cast<int64_t>(arrayPtr->values.size()));
                    break;
                }
                case OP_GET_INDEX:
                {
                    auto idx = Expect<int64_t>(Pop(), "OP_GET_INDEX expects int64_t as index");
                    auto arr = Expect<ArrayPtr>(Pop(), "OP_GET_INDEX expects array");

                    if (idx < 0 || idx >= arr->values.size())
                    {
                        throw std::runtime_error("Array index out of bounds: " + std::to_string(idx));
                    }
                    Push(arr->values[idx]);
                    break;
                }
                case OP_SET_INDEX:
                {
                    Value val = Pop();
                    auto idx = Expect<int64_t>(Pop(), "OP_SET_INDEX expects int64_t as index");
                    auto arr = Expect<ArrayPtr>(Pop(), "OP_SET_INDEX expects array");

                    if (idx < 0 || idx >= arr->values.size())
                    {
                        throw std::runtime_error("Array index out of bounds: " + std::to_string(idx));
                    }
                    arr->values[idx] = val;
                    Push(val);
                    break;
                }
                case OP_POP:
                {
                    Pop();
                    break;
                }
                default:
                    throw std::runtime_error("Unknown opcode: " + std::to_string(opcode));
            }
        }
    }

    void DefineNativeFunctions()
    {
        DefineNativePrint();
        DefineNativeRandom();
    }

    void DefineNative(const std::string& name, std::function<Value(const std::vector<Value>&)> fn)
    {
        auto native = std::make_shared<NativeFunction>();
        native->func = std::move(fn);
        m_globals[name] = native;
    }

    void DefineNativeRandom()
    {
        DefineNative("rand", [rng = std::mt19937(std::random_device{}()), this](const std::vector<Value>& args) mutable -> Value
        {
            auto max = Expect<int64_t>(args[0], "rand expects int64_t");
            std::uniform_int_distribution<int64_t> dist(0, max);
            return dist(rng);
        });
    }

    void DefineNativePrint()
    {
        DefineNative("print", [](const std::vector<Value>& args) -> Value
        {
             PrintValue(args[0]);
             std::cout << "\n" << std::flush;
             return false;
        });
    }
};