#pragma once
#include <variant>

using Value = std::variant<
    int64_t,
    double,
    bool
>;