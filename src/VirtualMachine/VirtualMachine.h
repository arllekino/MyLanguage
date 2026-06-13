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
#include "../Graphics/UIRenderer.h"

struct CallFrame
{
    ClosurePtr closure;
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
        DefineNativeUIFunctions();
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
    static constexpr int STACK_MAX = 2048 * 32; // 1 MB
    static constexpr int FRAMES_MAX = 64; // убрать?

    std::vector<Value> m_stack;
    std::vector<CallFrame> m_frames;
    std::unordered_map<std::string, Value> m_globals;
    std::vector<UpvaluePtr> m_openUpvalues;

    UIRenderer m_uiRenderer;
    bool m_uiInitialized = false;

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

    UpvaluePtr CaptureUpvalue(size_t localIndex)
    {
        for (const auto& uv : m_openUpvalues)
        {
            if (uv->location == localIndex)
            {
                return uv;
            }
        }
        auto createdUpvalue = std::make_shared<UpvalueObj>();
        createdUpvalue->location = localIndex;
        createdUpvalue->isClosed = false;
        m_openUpvalues.push_back(createdUpvalue);
        return createdUpvalue;
    }

    void CloseUpvalues(size_t lastSlot)
    {
        for (auto it = m_openUpvalues.begin(); it != m_openUpvalues.end(); )
        {
            if ((*it)->location >= lastSlot)
            {
                (*it)->closedValue = m_stack[(*it)->location];
                (*it)->isClosed = true;
                it = m_openUpvalues.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    Value CloneIfStruct(const Value& val)
    {
        if (std::holds_alternative<InstancePtr>(val))
        {
            auto instance = std::get<InstancePtr>(val);
            if (instance && instance->klass->isStruct)
            {
                auto cloned = std::make_shared<Instance>();
                cloned->klass = instance->klass;

                for (const auto& [name, fieldVal] : instance->fields)
                {
                    cloned->fields[name] = CloneIfStruct(fieldVal);
                }
                return cloned;
            }
        }
        return val;
    }

    double AsDouble(const Value& val, const std::string& errMsg)
    {
        if (std::holds_alternative<int64_t>(val))
        {
            return static_cast<double>(std::get<int64_t>(val));
        }
        if (std::holds_alternative<double>(val))
        {
            return std::get<double>(val);
        }
        throw std::runtime_error(errMsg);
    }

    Value CallClosure(ClosurePtr closure, const std::vector<Value>& args)
    {
        size_t targetDepth = m_frames.size();

        CallFrame frame;
        frame.closure = closure;
        frame.function = closure->function;
        frame.ip = 0;
        frame.baseSlot = m_stack.size();

        Push(closure);
        for (const auto& arg : args)
        {
            Push(arg);
        }

        m_frames.push_back(frame);

        int localsToAllocate = closure->function->maxLocals - args.size() - 1;
        for(int i = 0; i < localsToAllocate; i++) Push(Null{});

        Run(targetDepth);

        return Pop();
    }

    void Run(size_t targetDepth = 0)
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
                    m_globals[name] = CloneIfStruct(Pop());
                    break;
                }
                case OP_SET_GLOBAL:
                {
                    Value nameVal = ReadConstant();
                    std::string name = *Expect<StringPtr>(nameVal, "Global name must be a string");
                    if (m_globals.contains(name))
                    {
                        m_globals[name] = CloneIfStruct(m_stack.back());
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
                    m_stack[m_frames.back().baseSlot + slot] = CloneIfStruct(m_stack.back());
                    break;
                }
                case OP_CALL:
                {
                    int argCount = ReadByte();
                    size_t calleeIndex = m_stack.size() - 1 - argCount;
                    Value callee = m_stack[calleeIndex];

                    for (int i = 0; i < argCount; ++i)
                    {
                        size_t idx = calleeIndex + 1 + i;
                        m_stack[idx] = CloneIfStruct(m_stack[idx]);
                    }

                    if (m_frames.size() >= FRAMES_MAX)
                    {
                        throw std::runtime_error("Stack overflow: maximum call frame depth exceeded.");
                    }
                    if (std::holds_alternative<ClosurePtr>(callee))
                    {
                        ClosurePtr closure = std::get<ClosurePtr>(callee);
                        int expected = closure->function->arity;
                        while (argCount < expected) { Push(Null{}); argCount++; }
                        if (argCount > expected)
                        {
                            m_stack.erase(m_stack.end() - (argCount - expected), m_stack.end());
                            argCount = expected;
                        }
                        CallFrame frame;
                        frame.closure = closure;
                        frame.function = closure->function;
                        frame.ip = 0;
                        frame.baseSlot = m_stack.size() - 1 - argCount;
                        m_frames.push_back(frame);

                        int localsToAllocate = closure->function->maxLocals - argCount - 1;
                        for(int i = 0; i < localsToAllocate; i++) Push(Null{});
                    }
                    else if (std::holds_alternative<FunctionPtr>(callee))
                    {
                        FunctionPtr func = std::get<FunctionPtr>(callee);

                        int expected = func->arity;
                        while (argCount < expected) { Push(Null{}); argCount++; }
                        if (argCount > expected) {
                            m_stack.erase(m_stack.end() - (argCount - expected), m_stack.end());
                            argCount = expected;
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

                        int expected = bound->method->arity;
                        while (argCount < expected) { Push(Null{}); argCount++; }
                        if (argCount > expected) {
                            m_stack.erase(m_stack.end() - (argCount - expected), m_stack.end());
                            argCount = expected;
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

                        size_t stackSizeBeforeCall = m_stack.size() - argCount - 1;

                        Value result = native->func(args);

                        m_stack.erase(m_stack.begin() + stackSizeBeforeCall, m_stack.end());
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
                    else if (std::holds_alternative<NativeBoundMethodPtr>(callee))
                    {
                        NativeBoundMethodPtr bound = std::get<NativeBoundMethodPtr>(callee);
                        std::vector<Value> args;
                        args.reserve(argCount);
                        for (int i = 0; i < argCount; ++i)
                        {
                            args.push_back(m_stack[m_stack.size() - argCount + i]);
                        }

                        size_t stackSizeBeforeCall = m_stack.size() - argCount - 1;

                        Value result = bound->func(bound->receiver, args);

                        m_stack.erase(m_stack.begin() + stackSizeBeforeCall, m_stack.end());
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
                    CloseUpvalues(baseSlot);
                    m_frames.pop_back();

                    m_stack.erase(m_stack.begin() + baseSlot, m_stack.end());
                    Push(result);

                    if (m_frames.size() == targetDepth)
                    {
                        return;
                    }
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
                    if (std::holds_alternative<Null>(l) && std::holds_alternative<Null>(r))
                    {
                        Push(true);
                    }
                    else if (std::holds_alternative<Null>(l) || std::holds_alternative<Null>(r))
                    {
                        Push(false);
                    }
                    else
                    {
                        Push(l == r);
                    }
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
                case OP_STRUCT:
                {
                    Value nameVal = ReadConstant();
                    std::string structName = *std::get<StringPtr>(nameVal);

                    auto klass = std::make_shared<Klass>();
                    klass->name = structName;
                    klass->isStruct = true;

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

                    if (std::holds_alternative<ArrayPtr>(instanceVal))
                    {
                        Pop();
                        auto arr = std::get<ArrayPtr>(instanceVal);

                        if (propName == "count") {
                            Push(static_cast<int64_t>(arr->values.size()));
                            break;
                        }

                        auto bound = std::make_shared<NativeBoundMethod>();
                        bound->receiver = instanceVal;

                        if (propName == "forEach") {
                            bound->func = [this](Value rec, const std::vector<Value>& args) -> Value {
                                auto array = std::get<ArrayPtr>(rec);
                                auto closure = Expect<ClosurePtr>(args[0], "forEach expects a closure");
                                for (const auto& val : array->values) {
                                    Value nestedResult = CallClosure(closure, {val});
                                }
                                return Null{};
                            };
                        } else if (propName == "map") {
                            bound->func = [this](Value rec, const std::vector<Value>& args) -> Value {
                                auto array = std::get<ArrayPtr>(rec);
                                auto closure = Expect<ClosurePtr>(args[0], "map expects a closure");
                                auto newArr = std::make_shared<Array>();
                                for (const auto& val : array->values) {
                                    newArr->values.push_back(CallClosure(closure, {val}));
                                }
                                return newArr;
                            };
                        } else if (propName == "filter") {
                            bound->func = [this](Value rec, const std::vector<Value>& args) -> Value {
                                auto array = std::get<ArrayPtr>(rec);
                                auto closure = Expect<ClosurePtr>(args[0], "filter expects a closure");
                                auto newArr = std::make_shared<Array>();
                                for (const auto& val : array->values) {
                                    Value res = CallClosure(closure, {val});
                                    if (std::holds_alternative<bool>(res) && std::get<bool>(res)) {
                                        newArr->values.push_back(val);
                                    }
                                }
                                return newArr;
                            };
                        } else {
                            throw std::runtime_error("Unknown array method: " + propName);
                        }

                        Push(bound);
                        break;
                    }

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
                            throw std::runtime_error("Undefined property '" + propName + "' on instance of '" + instance->klass->name + "'.");
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

                    instance->fields[propName] = CloneIfStruct(valueToSet);
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
                case OP_CLOSURE:
                {
                    Value functionVal = ReadConstant();
                    auto function = Expect<FunctionPtr>(functionVal, "OP_CLOSURE expects a function");
                    uint8_t upvalueCount = ReadByte();

                    auto closure = std::make_shared<Closure>();
                    closure->function = function;

                    for (int i = 0; i < upvalueCount; i++)
                    {
                        uint8_t isLocal = ReadByte();
                        uint8_t index = ReadByte();

                        if (isLocal == 1) {
                            closure->upvalues.push_back(CaptureUpvalue(m_frames.back().baseSlot + index));
                        } else {
                            closure->upvalues.push_back(m_frames.back().closure->upvalues[index]);
                        }
                    }
                    Push(closure);
                    break;
                }
                case OP_GET_UPVALUE:
                {
                    uint8_t slot = ReadByte();
                    UpvaluePtr uv = m_frames.back().closure->upvalues[slot];
                    if (uv->isClosed)
                    {
                        Push(uv->closedValue);
                    }
                    else
                    {
                        Push(m_stack[uv->location]);
                    }
                    break;
                }
                case OP_SET_UPVALUE:
                {
                    uint8_t slot = ReadByte();
                    UpvaluePtr uv = m_frames.back().closure->upvalues[slot];
                    Value val = m_stack.back();
                    if (uv->isClosed) {
                        uv->closedValue = val;
                    } else {
                        m_stack[uv->location] = val;
                    }
                    break;
                }
                case OP_CLOSE_UPVALUE:
                {
                    CloseUpvalues(m_stack.size() - 1);
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
        DefineNativeRead();
        DefineNativeConversions();
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

    void DefineNativeConversions()
    {
        DefineNative("Int", [](const std::vector<Value>& args) -> Value {
            if (std::holds_alternative<double>(args[0]))
            {
                return static_cast<int64_t>(std::get<double>(args[0]));
            }
            if (std::holds_alternative<int64_t>(args[0]))
            {
                return std::get<int64_t>(args[0]);
            }
            throw std::runtime_error("Cannot convert to Int");
        });

        DefineNative("Double", [](const std::vector<Value>& args) -> Value {
            if (std::holds_alternative<int64_t>(args[0]))
            {
                return static_cast<double>(std::get<int64_t>(args[0]));
            }
            if (std::holds_alternative<double>(args[0]))
            {
                return std::get<double>(args[0]);
            }
            throw std::runtime_error("Cannot convert to Double");
        });
    }

    void DefineNativeUIFunctions()
    {
        DefineNative("UIInitWindow", [this](const std::vector<Value>& args) -> Value {
            auto width = Expect<int64_t>(args[0], "width must be Int");
            auto height = Expect<int64_t>(args[1], "height must be Int");
            auto titlePtr = Expect<StringPtr>(args[2], "title must be String");

            m_uiRenderer.Init(static_cast<int>(width), static_cast<int>(height), *titlePtr);
            m_uiInitialized = true;
            return Null{};
        });

        DefineNative("UIShouldClose", [this](const std::vector<Value>& args) -> Value {
            if (!m_uiInitialized)
            {
                return true;
            }
            return glfwWindowShouldClose(m_uiRenderer.window) != 0;
        });

        DefineNative("UIBeginFrame", [this](const std::vector<Value>& args) -> Value {
            int64_t width = Expect<int64_t>(args[0], "width must be Int");
            int64_t height = Expect<int64_t>(args[1], "height must be Int");
            if (m_uiInitialized) {
                m_uiRenderer.BeginFrame(static_cast<int>(width), static_cast<int>(height));
            }
            return Null{};
        });

        DefineNative("UIDrawRect", [this](const std::vector<Value>& args) -> Value {
            if (!m_uiInitialized) return Null{};

            double x = AsDouble(args[0], "x must be Int or Double");
            double y = AsDouble(args[1], "y must be Int or Double");
            double w = AsDouble(args[2], "w must be Int or Double");
            double h = AsDouble(args[3], "h must be Int or Double");

            double r = AsDouble(args[4], "r must be Int or Double");
            double g = AsDouble(args[5], "g must be Int or Double");
            double b = AsDouble(args[6], "b must be Int or Double");
            double a = AsDouble(args[7], "a must be Int or Double");

            m_uiRenderer.DrawRect(
                static_cast<float>(x), static_cast<float>(y),
                static_cast<float>(w), static_cast<float>(h),
                glm::vec4(r, g, b, a)
            );
            return Null{};
        });

        DefineNative("UIEndFrame", [this](const std::vector<Value>& args) -> Value
        {
            if (m_uiInitialized) {
                m_uiRenderer.EndFrame();
            }
            return Null{};
        });
    }
};