#pragma once
#include <variant>
#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <future>

struct Chunk;
struct Array;
struct Function;
struct NativeFunction;
struct Klass;
struct Instance;
struct BoundMethod;
struct Null{};
struct UpvalueObj;
struct Closure;
struct NativeBoundMethod;
struct AsyncFuture;

class ViewNode;

using StringPtr = std::shared_ptr<const std::string>;
using ArrayPtr = std::shared_ptr<Array>;
using FunctionPtr = std::shared_ptr<Function>;
using NativeFnPtr = std::shared_ptr<NativeFunction>;
using KlassPtr = std::shared_ptr<Klass>;
using InstancePtr = std::shared_ptr<Instance>;
using BoundMethodPtr = std::shared_ptr<BoundMethod>;
using WeakInstancePtr = std::weak_ptr<Instance>;
using UpvaluePtr = std::shared_ptr<UpvalueObj>;
using ClosurePtr = std::shared_ptr<Closure>;
using NativeBoundMethodPtr = std::shared_ptr<NativeBoundMethod>;
using AsyncFuturePtr = std::shared_ptr<AsyncFuture>;

using ViewNodePtr = std::shared_ptr<ViewNode>;

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
    WeakInstancePtr,
    ClosurePtr,
    ViewNodePtr,
    NativeBoundMethodPtr,
    AsyncFuturePtr
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
    bool isStruct = false;
    std::vector<std::string> fields;
    std::unordered_map<std::string, FunctionPtr> methods;
    std::unordered_map<std::string, Value> staticFields;
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

struct UpvalueObj
{
    bool isClosed = false;
    size_t location = 0;
    Value closedValue = Null{};
};

struct Closure
{
    FunctionPtr function;
    std::vector<UpvaluePtr> upvalues;
};

struct NativeBoundMethod
{
    Value receiver;
    std::function<Value(Value, const std::vector<Value>&)> func;
};

struct AsyncFuture
{
    std::shared_future<Value> result;
};