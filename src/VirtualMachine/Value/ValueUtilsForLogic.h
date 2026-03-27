#pragma once
#include "./Value.h"

template<typename T>
concept isBool = std::is_same_v<T, bool>;

inline Value operator+(const Value& lhs, const Value& rhs)
{
    return std::visit([&](auto&& l, auto&& r) -> Value
    {
        using L = std::decay_t<decltype(l)>;
        using R = std::decay_t<decltype(r)>;

        if constexpr (isBool<L> || isBool<R>)
        {
            throw std::invalid_argument("Operations with bool are not supported");
        }
        else if constexpr (std::is_arithmetic_v<L> && std::is_arithmetic_v<R>)
        {
            using ResultType = std::common_type_t<L, R>;
            return static_cast<ResultType>(l + r);
        }
        else
        {
            throw std::invalid_argument("Cannot add values of different types");
        }
    }, lhs, rhs);
}

inline Value operator-(const Value& lhs, const Value& rhs)
{
    return std::visit([&](auto&& l, auto&& r) -> Value
    {
        using L = std::decay_t<decltype(l)>;
        using R = std::decay_t<decltype(r)>;

        if constexpr (isBool<L> || isBool<R>)
        {
            throw std::invalid_argument("Operations with bool are not supported");
        }
        else if constexpr (std::is_arithmetic_v<L> && std::is_arithmetic_v<R>)
        {
            using ResultType = std::common_type_t<L, R>;
            return static_cast<ResultType>(l - r);
        }
        else
        {
            throw std::invalid_argument("Cannot subtract values of different types");
        }
    }, lhs, rhs);
}

inline Value operator*(const Value& lhs, const Value& rhs)
{
    return std::visit([&](auto&& l, auto&& r) -> Value
    {
        using L = std::decay_t<decltype(l)>;
        using R = std::decay_t<decltype(r)>;

        if constexpr (isBool<L> || isBool<R>)
        {
            throw std::invalid_argument("Operations with bool are not supported");
        }
        else if constexpr (std::is_arithmetic_v<L> && std::is_arithmetic_v<R>)
        {
            using ResultType = std::common_type_t<L, R>;
            return static_cast<ResultType>(l * r);
        }
        else
        {
            throw std::invalid_argument("Cannot multiply values of different types");
        }
    }, lhs, rhs);
}

inline Value operator/(const Value& lhs, const Value& rhs)
{
    return std::visit([&](auto&& l, auto&& r) -> Value
    {
        using L = std::decay_t<decltype(l)>;
        using R = std::decay_t<decltype(r)>;

        if constexpr (isBool<L> || isBool<R>)
        {
            throw std::invalid_argument("Operations with bool are not supported");
        }
        else if constexpr (std::is_arithmetic_v<L> && std::is_arithmetic_v<R>)
        {
            if (r == 0)
            {
                throw std::invalid_argument("Division by zero");
            }
            using ResultType = std::common_type_t<L, R>;
            return static_cast<ResultType>(l / r);
        }
        else
        {
            throw std::invalid_argument("Cannot divide values of different types");
        }
    }, lhs, rhs);
}

inline Value operator%(const Value& lhs, const Value& rhs)
{
    return std::visit([&](auto&& l, auto&& r) -> Value
    {
        using L = std::decay_t<decltype(l)>;
        using R = std::decay_t<decltype(r)>;

        if constexpr (isBool<L> || isBool<R>)
        {
            throw std::invalid_argument("Operations with bool are not supported");
        }
        else if constexpr (std::is_arithmetic_v<L> && std::is_arithmetic_v<R>)
        {
            if (r == 0)
            {
                throw std::invalid_argument("Division by zero");
            }

            if constexpr (std::is_integral_v<L> && std::is_integral_v<R>)
            {
                return static_cast<int64_t>(l % r);
            }
            else
            {
                return std::fmod(static_cast<double>(l), static_cast<double>(r));
            }
        }
        else
        {
            throw std::invalid_argument("Modulo operation not supported for these types");
        }
    }, lhs, rhs);
}

inline Value operator-(const Value& val)
{
    return std::visit([](auto&& v) -> Value
    {
        using T = std::decay_t<decltype(v)>;

        if constexpr (isBool<T>)
        {
            throw std::invalid_argument("Unary minus not supported for bool");
        }
        else if constexpr (std::is_arithmetic_v<T>)
        {
            return -v;
        }
        else
        {
            throw std::invalid_argument("Unary minus not supported for this type");
        }
    }, val);
}

inline bool operator==(const Value& lhs, const Value& rhs)
{
    return std::visit([](auto&& l, auto&& r) -> bool
    {
        using L = std::decay_t<decltype(l)>;
        using R = std::decay_t<decltype(r)>;

        if constexpr (std::is_same_v<L, R>)
        {
            return l == r;
        }
        else
        {
            throw std::invalid_argument("Cannot compare values of different types");
        }
    }, lhs, rhs);
}

inline bool operator!=(const Value& lhs, const Value& rhs)
{
    return !(lhs == rhs);
}

inline bool operator<(const Value& lhs, const Value& rhs)
{
    return std::visit([](auto&& l, auto&& r) -> bool
    {
        using L = std::decay_t<decltype(l)>;
        using R = std::decay_t<decltype(r)>;

        if constexpr (isBool<L> || isBool<R>)
        {
            throw std::invalid_argument("Cannot compare bool with other types");
        }

        if constexpr (std::is_same_v<L, R>)
        {
            return l < r;
        }
        else
        {
            throw std::invalid_argument("Cannot compare values of different types");
        }
    }, lhs, rhs);
}

inline bool operator>(const Value& lhs, const Value& rhs)
{
    return rhs < lhs;
}

inline bool operator<=(const Value& lhs, const Value& rhs)
{
    return !(rhs < lhs);
}

inline bool operator>=(const Value& lhs, const Value& rhs)
{
    return !(lhs < rhs);
}