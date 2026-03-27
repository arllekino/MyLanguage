#pragma once
#include <vector>
#include "Value/Value.h"

struct Chunk
{
    std::vector<uint8_t> code;
    std::vector<Value> constants;
};