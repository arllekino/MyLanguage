#pragma once

enum class TokenType
{
    KEYWORD,
    IDENTIFIER,
    INTEGER,
    DOUBLE,
    STRING,
    OPERATOR,
    SEPARATOR,
    COMMENT,
    SPACE,
    ERROR,
    END_OF_FILE,
    OPTIONAL,
    FORCE_UNWRAP,

};