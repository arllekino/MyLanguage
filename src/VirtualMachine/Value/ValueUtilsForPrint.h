#pragma once
#include <iostream>

#include "./Value.h"

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
            std::cout << (v ? *v : "nil");
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
            else std::cout << "<func " << v->name;
        }

    }, value);
}