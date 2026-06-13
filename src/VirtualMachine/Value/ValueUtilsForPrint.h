#pragma once
#include <iostream>
#include <sstream>

#include "./Value.h"

inline std::string ValueToString(const Value& value)
{
    return std::visit([] (auto&& v) -> std::string {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, int64_t>)   return std::to_string(v);
        if constexpr (std::is_same_v<T, double>)     { std::ostringstream s; s << v; return s.str(); }
        if constexpr (std::is_same_v<T, bool>)       return v ? "true" : "false";
        if constexpr (std::is_same_v<T, StringPtr>)  return v ? *v : "null";
        if constexpr (std::is_same_v<T, Null>)            return "null";
        if constexpr (std::is_same_v<T, AsyncFuturePtr>)  return "<async>";
        return "<value>";
    }, value);
}

inline void PrintValue(const Value& value)
{
    std::visit([] (auto&& v) {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, int64_t> || std::is_same_v<T, double>)
        {
            std::cout << v;
        }
        else if constexpr (std::is_same_v<T, bool>)
        {
            std::cout << (v ? "true" : "false");
        }
        else if constexpr (std::is_same_v<T, StringPtr>)
        {
            std::cout << (v ? *v : "null");
        }
        else if constexpr (std::is_same_v<T, ArrayPtr>)
        {
            if (!v)
            {
                std::cout << "[]";
                return;
            }
            std::cout << "[";
            for (size_t i = 0; i < v->values.size(); ++i)
            {
                PrintValue(v->values[i]);
                if (i + 1 < v->values.size())
                {
                    std::cout << ", ";
                }
            }
            std::cout << "]";
        }
        else if constexpr (std::is_same_v<T, FunctionPtr>)
        {
            if (!v) std::cout << "null func";
            else std::cout << "<func " << v->name << ">";
        }
        else if constexpr (std::is_same_v<T, Null>)
        {
            std::cout << "null";
        }
        else if constexpr (std::is_same_v<T, ViewNodePtr>)
        {
            std::cout << "<ViewNode " << v.get() << ">";
        }
        else if constexpr (std::is_same_v<T, InstancePtr>)
        {
            if (!v) std::cout << "null instance";
            else std::cout << "<instance " << v->klass->name << ">";
        }
        else if constexpr (std::is_same_v<T, KlassPtr>) { std::cout << "<class>"; }
        else if constexpr (std::is_same_v<T, ClosurePtr>) { std::cout << "<closure>"; }
        else if constexpr (std::is_same_v<T, BoundMethodPtr> || std::is_same_v<T, NativeBoundMethodPtr>) { std::cout << "<bound method>"; }
        else if constexpr (std::is_same_v<T, NativeFnPtr>) { std::cout << "<native fn>"; }
        else if constexpr (std::is_same_v<T, WeakInstancePtr>) { std::cout << "<weak instance>"; }
        else if constexpr (std::is_same_v<T, AsyncFuturePtr>) { std::cout << "<async>"; }
        else
        {
            std::cout << "<unknown type>";
        }
    }, value);
}