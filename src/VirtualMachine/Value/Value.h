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

using StringPtr = std::shared_ptr<const std::string>;
using ArrayPtr = std::shared_ptr<Array>;
using FunctionPtr = std::shared_ptr<Function>;
using NativeFnPtr = std::shared_ptr<NativeFunction>;
using KlassPtr = std::shared_ptr<Klass>;

using Value = std::variant<
    int64_t,
    double,
    bool,
    StringPtr,
    ArrayPtr,
    FunctionPtr,
    NativeFnPtr
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
};

struct NativeFunction
{
    std::function<Value(const std::vector<Value>&)> func;
};

struct Klass
{
    std::string name;
    
};