#pragma once
#include <variant>
#include <vector>
#include <string>
#include <memory>
#include <functional>

struct Chunk;
struct Array;
struct Function;
struct NativeFunction;
struct Klass;
struct Instance;
struct BoundMethod;
struct Null{};

using StringPtr = std::shared_ptr<const std::string>;
using ArrayPtr = std::shared_ptr<Array>;
using FunctionPtr = std::shared_ptr<Function>;
using NativeFnPtr = std::shared_ptr<NativeFunction>;
using KlassPtr = std::shared_ptr<Klass>;
using InstancePtr = std::shared_ptr<Instance>;
using BoundMethodPtr = std::shared_ptr<BoundMethod>;
using WeakInstancePtr = std::weak_ptr<Instance>;

using Value = std::variant<
    Null,
    int64_t,
    double,
    bool,
    StringPtr,
    ArrayPtr,
    FunctionPtr,
    NativeFnPtr,
    KlassPtr,
    InstancePtr,
    BoundMethodPtr,
    WeakInstancePtr
>;

struct Array
{
    std::vector<Value> values;
};

struct Function
{
    std::string name;
    std::unique_ptr<Chunk> chunk;
    int arity = 0;
    int maxLocals = 0;
};

struct NativeFunction
{
    std::function<Value(const std::vector<Value>&)> func;
};

struct Klass
{
    std::string name;
    std::vector<std::string> fields;
    std::unordered_map<std::string, FunctionPtr> methods;
};

struct Instance
{
    KlassPtr klass;
    std::unordered_map<std::string, Value> fields;
};

struct BoundMethod
{
    InstancePtr receiver;
    FunctionPtr method;
};