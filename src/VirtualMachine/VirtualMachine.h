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
        mainFrame.baseSlot = 0;
        m_frames.push_back(mainFrame);

        for(int i = 0; i < mainFunc->maxLocals - 1; i++)
        {
            Push(Null{});
        }

        Run();
    }

private:
    static constexpr int STACK_MAX = 2048;
    static constexpr int FRAMES_MAX = 64; // убрать?

    std::vector<Value> m_stack;
    std::vector<CallFrame> m_frames;
    std::unordered_map<std::string, Value> m_globals;

    uint8_t ReadByte()
    {
        return m_frames.back().function->chunk->code[m_frames.back().ip++];
    }

    uint16_t ReadShort()
    {
        uint8_t high = ReadByte();
        uint8_t low = ReadByte();
        return static_cast<uint16_t>((high << 8) | low);
    }

    Value ReadConstant()
    {
        return m_frames.back().function->chunk->constants[ReadByte()];
    }

    Value Pop()
    {
        Value val = m_stack.back();
        m_stack.pop_back();
        return val;
    }

    void Push(const Value& value)
    {
        if (m_stack.size() >= STACK_MAX)
        {
            throw std::runtime_error("Stack overflow: exceeded maximum stack size.");
        }
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

    static InstancePtr GetStrongInstance(const Value& value, const std::string& errorMsg)
    {
        if (std::holds_alternative<InstancePtr>(value))
        {
            return std::get<InstancePtr>(value);
        }
        if (std::holds_alternative<WeakInstancePtr>(value))
        {
            auto weakPtr = std::get<WeakInstancePtr>(value);
            if (auto shared = weakPtr.lock())
            {
                return shared;
            }
            throw std::runtime_error("Fatal Error: Attempted to access a deallocated weak reference.");
        }
        throw std::runtime_error("Type mismatch: " + errorMsg);
    }

    static void CheckOperandsNotNil(const Value& l, const Value& r, const std::string& opName)
    {
        if (std::holds_alternative<Null>(l) || std::holds_alternative<Null>(r))
        {
            throw std::runtime_error("Runtime Error: Cannot perform '" + opName + "' operation with a Nil value.");
        }
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
                case OP_DEFINE_GLOBAL:
                {
                    Value nameVal = ReadConstant();
                    std::string name = *Expect<StringPtr>(nameVal, "Global name must be a string");
                    m_globals[name] = Pop();
                    break;
                }
                case OP_SET_GLOBAL:
                {
                    Value nameVal = ReadConstant();
                    std::string name = *Expect<StringPtr>(nameVal, "Global name must be a string");
                    if (m_globals.contains(name))
                    {
                        m_globals[name] = m_stack.back();
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

                    if (m_frames.size() >= FRAMES_MAX)
                    {
                        throw std::runtime_error("Stack overflow: maximum call frame depth exceeded.");
                    }

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
                        frame.baseSlot = m_stack.size() - 1 - argCount;
                        m_frames.push_back(frame);

                        int localsToAllocate = func->maxLocals - argCount - 1;
                        for(int i = 0; i < localsToAllocate; i++)
                        {
                            Push(Null{});
                        }
                    }
                    else if (std::holds_alternative<BoundMethodPtr>(callee))
                    {
                        BoundMethodPtr bound = std::get<BoundMethodPtr>(callee);
                        if (argCount != bound->method->arity)
                        {
                            throw std::runtime_error("Expected " + std::to_string(bound->method->arity) + " args but got " + std::to_string(argCount));
                        }

                        m_stack[m_stack.size() - 1 - argCount] = bound->receiver;

                        CallFrame frame;
                        frame.function = bound->method;
                        frame.ip = 0;
                        frame.baseSlot = m_stack.size() - 1 - argCount;
                        m_frames.push_back(frame);

                        int localsToAllocate = bound->method->maxLocals - argCount - 1;
                        for(int i = 0; i < localsToAllocate; i++) Push(false);
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
                    else if (std::holds_alternative<KlassPtr>(callee))
                    {
                        KlassPtr klass = std::get<KlassPtr>(callee);
                        auto instance = std::make_shared<Instance>();
                        instance->klass = klass;

                        m_stack[m_stack.size() - 1 - argCount] = instance;

                        if (klass->methods.contains("init"))
                        {
                            FunctionPtr initMethod = klass->methods["init"];
                            if (argCount != initMethod->arity)
                            {
                                throw std::runtime_error("Expected " + std::to_string(initMethod->arity) + " arguments for init(), got " + std::to_string(argCount));
                            }

                            CallFrame frame;
                            frame.function = initMethod;
                            frame.ip = 0;
                            frame.baseSlot = m_stack.size() - 1 - argCount;
                            m_frames.push_back(frame);

                            int localsToAllocate = initMethod->maxLocals - argCount - 1;
                            for(int i = 0; i < localsToAllocate; i++) Push(false);
                        }
                        else
                        {
                            if (argCount != 0)
                            {
                                throw std::runtime_error("Class '" + klass->name + "' has no init() method, but arguments were provided.");
                            }
                        }
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

                    m_stack.erase(m_stack.begin() + baseSlot, m_stack.end());
                    Push(result);
                    break;
                }
                case OP_ADD:
                {
                    const Value r = Pop();
                    const Value l = Pop();
                    CheckOperandsNotNil(l, r, "+");
                    Push(l + r);
                    break;
                }
                case OP_SUB:
                {
                    const Value r = Pop();
                    const Value l = Pop();
                    CheckOperandsNotNil(l, r, "-");
                    Push(l - r);
                    break;
                }
                case OP_MUL:
                {
                    const Value r = Pop();
                    const Value l = Pop();
                    CheckOperandsNotNil(l, r, "*");
                    Push(l * r);
                    break;
                }
                case OP_DIV:
                {
                    const Value r = Pop();
                    const Value l = Pop();
                    CheckOperandsNotNil(l, r, "/");
                    Push(l / r);
                    break;
                }
                case OP_MOD:
                {
                    const Value r = Pop();
                    const Value l = Pop();
                    CheckOperandsNotNil(l, r, "%");
                    Push(l % r);
                    break;
                }
                case OP_LESS:
                {
                    const Value r = Pop();
                    const Value l = Pop();
                    CheckOperandsNotNil(l, r, "<");
                    Push(l < r);
                    break;
                }
                case OP_GREATER:
                {
                    const Value r = Pop();
                    const Value l = Pop();
                    CheckOperandsNotNil(l, r, ">");
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
                    Value arr = m_stack.back();

                    auto arrayPtr = Expect<ArrayPtr>(arr, "OP_ARRAY_PUSH expects array");
                    arrayPtr->values.push_back(val);
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
                case OP_CLASS:
                {
                    Value nameVal = ReadConstant();
                    std::string className = *std::get<StringPtr>(nameVal);

                    auto klass = std::make_shared<Klass>();
                    klass->name = className;

                    Push(klass);
                    break;
                }
                case OP_FIELD:
                {
                    Value fieldNameVal = ReadConstant();
                    std::string fieldName = *std::get<StringPtr>(fieldNameVal);

                    Value klassVal = m_stack.back();
                    auto klass = Expect<KlassPtr>(klassVal, "OP_FIELD expects a class");

                    klass->fields.push_back(fieldName);
                    break;
                }
                case OP_METHOD:
                {
                    Value methodNameVal = ReadConstant();
                    std::string methodName = *std::get<StringPtr>(methodNameVal);

                    Value klassVal = Pop();
                    Value methodVal = Pop();

                    auto klass = Expect<KlassPtr>(klassVal, "OP_METHOD expects a class");
                    auto method = Expect<FunctionPtr>(methodVal, "OP_METHOD expects a function");

                    klass->methods[methodName] = method;
                    break;
                }
                case OP_GET_PROPERTY:
                {
                    Value nameVal = ReadConstant();
                    std::string propName = *std::get<StringPtr>(nameVal);

                    Value instanceVal = m_stack.back();
                    auto instance = GetStrongInstance(instanceVal, "Only instances have properties.");

                    bool isField = false;
                    for (const auto& field : instance->klass->fields)
                    {
                        if (field == propName)
                        {
                            isField = true;
                            break;
                        }
                    }

                    if (isField)
                    {
                        Pop();
                        if (instance->fields.contains(propName))
                        {
                            Push(instance->fields[propName]);
                        }
                        else
                        {
                            Push(false);
                        }
                    }
                    else if (instance->klass->methods.contains(propName))
                    {
                        Pop();
                        auto boundMethod = std::make_shared<BoundMethod>();
                        boundMethod->receiver = instance;
                        boundMethod->method = instance->klass->methods[propName];
                        Push(boundMethod);
                    }
                    else
                    {
                        throw std::runtime_error("Undefined property or method '" + propName + "' on instance.");
                    }
                    break;
                }
                case OP_SET_PROPERTY:
                {
                    Value nameVal = ReadConstant();
                    std::string propName = *std::get<StringPtr>(nameVal);

                    Value valueToSet = Pop();
                    Value instanceVal = Pop();
                    auto instance = GetStrongInstance(instanceVal, "Only instances have properties.");

                    bool fieldExists = false;
                    for (const auto& field : instance->klass->fields)
                    {
                        if (field == propName)
                        {
                            fieldExists = true;
                            break;
                        }
                    }
                    if (!fieldExists)
                    {
                        throw std::runtime_error("Error: Class '" + instance->klass->name + "' has no property '" + propName + "'.");
                    }

                    instance->fields[propName] = valueToSet;
                    Push(valueToSet);
                    break;
                }
                case OP_SET_PROPERTY_WEAK:
                {
                    Value nameVal = ReadConstant();
                    std::string propName = *std::get<StringPtr>(nameVal);

                    Value valueToSet = Pop();
                    Value instanceVal = Pop();
                    auto instance = GetStrongInstance(instanceVal, "Only instances have properties.");

                    bool fieldExists = false;
                    for (const auto& field : instance->klass->fields)
                    {
                        if (field == propName)
                        {
                            fieldExists = true;
                            break;
                        }
                    }
                    if (!fieldExists)
                    {
                        throw std::runtime_error("Error: Class '" + instance->klass->name + "' has no property '" + propName + "'.");
                    }

                    if (std::holds_alternative<InstancePtr>(valueToSet))
                    {
                        instance->fields[propName] = WeakInstancePtr(std::get<InstancePtr>(valueToSet));
                    }
                    else if (std::holds_alternative<Null>(valueToSet))
                    {
                        instance->fields[propName] = valueToSet;
                    }
                    else
                    {
                        throw std::runtime_error("Fatal: Attempted to assign a non-instance value to a weak property.");
                    }

                    Push(valueToSet);
                    break;
                }
                case OP_AND:
                {
                    Value r = Pop();
                    Value l = Pop();

                    bool rightBool = std::holds_alternative<bool>(r) ? std::get<bool>(r) : false;
                    bool leftBool = std::holds_alternative<bool>(l) ? std::get<bool>(l) : false;

                    Push(leftBool && rightBool);
                    break;
                }
                case OP_OR:
                {
                    Value r = Pop();
                    Value l = Pop();

                    bool rightBool = std::holds_alternative<bool>(r) ? std::get<bool>(r) : false;
                    bool leftBool = std::holds_alternative<bool>(l) ? std::get<bool>(l) : false;

                    Push(leftBool || rightBool);
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
        DefineNativeRead();
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

    void DefineNativeRead()
    {
        DefineNative("read", [](const std::vector<Value>& args) -> Value
        {
            std::string input;
            std::getline(std::cin, input);
            return std::make_shared<std::string>(input);
        });
    }
};