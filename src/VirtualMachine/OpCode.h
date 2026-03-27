#pragma once
#include <cstdint>

enum OpCode : uint8_t
{
    OP_CONSTANT = 0,
    OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD, OP_NEG,

    OP_GREATER, OP_LESS, OP_EQUAL, OP_NOT,

    OP_JUMP,
    OP_JUMP_IF_FALSE,
    OP_LOOP,

    OP_GET_LOCAL,
    OP_SET_LOCAL,

    OP_BUILD_ARRAY,
    OP_GET_INDEX,
    OP_SET_INDEX,
    OP_ARRAY_LEN,
    OP_ARRAY_PUSH,
    OP_POP,

    OP_PRINT,

    OP_RETURN = 255
};