#pragma once
#include <variant>
#include <vector>
#include "../Chunk.h"

struct Chunk;
struct Array;
struct Function;

using StringPtr = std::shared_ptr<const std::string>;
using ArrayPtr = std::shared_ptr<Array>;
using FunctionPtr = std::shared_ptr<Function>;

using Value = std::variant<
    int64_t,
    double,
    bool,
    StringPtr,
    ArrayPtr,
    FunctionPtr
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