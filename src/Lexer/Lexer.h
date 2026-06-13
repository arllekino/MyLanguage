#pragma once
#include <string>
#include <unordered_map>
#include <vector>

#include "Token.h"
#include "TokenType.h"

class Lexer
{
public:
    explicit Lexer(std::string source)
        : m_source(std::move(source))
    {
    }

    std::vector<Token> Tokenize()
    {
        std::vector<Token> tokens;

        while (m_position < m_source.length())
        {
            SkipWhiteSpace();
            const char current = Peek();

            if (current == '\0') break;

            Token token;

            if (isalpha(current) || current == '_')
            {
                token = ReadIdentifierOrKeyword();
            }
            else if (isdigit(current))
            {
                token = ReadNumber();
            }
            else if (current == '"')
            {
                token = ReadString();
            }
            else if (current == '/')
            {
                if (PeekNext() == '/' || PeekNext() == '*')
                {
                    token = ReadComment();
                }
                else
                {
                    token = ReadOperator();
                }
            }
            else if (strchr("+-*=!<>?&|^%~.", current))
            {
                if (current == '.' && PeekNext() != '.')
                {
                    std::string value(1, Advance());
                    token = Token(TokenType::SEPARATOR, value, m_line, m_column);
                }
                else
                {
                    token = ReadOperator();
                }
            }
            else if (strchr("(){},;:", current) || current == '[' || current == ']')
            {
                std::string value(1, Advance());
                token = Token(TokenType::SEPARATOR, value, m_line, m_column);
            }
            else
            {
                std::string value(1, Advance());
                token = Token(TokenType::ERROR, "Unexpected character: " + value, m_line, m_column);
            }

            tokens.push_back(token);
        }

        tokens.push_back(Token(TokenType::END_OF_FILE, "", m_line, m_column));
        return tokens;
    }

private:
    std::string m_source;
    unsigned m_position = 0;
    unsigned m_line = 1;
    unsigned m_column = 1;

    std::unordered_map<std::string, TokenType> m_keywords = {
        {"const", TokenType::KEYWORD}, {"let", TokenType::KEYWORD},
        {"func", TokenType::KEYWORD}, {"class", TokenType::KEYWORD},
        {"async", TokenType::KEYWORD}, {"throws", TokenType::KEYWORD},
        {"typealias", TokenType::KEYWORD},
        {"init", TokenType::KEYWORD}, {"deinit", TokenType::KEYWORD},
        {"static", TokenType::KEYWORD},
        {"private", TokenType::KEYWORD}, {"public", TokenType::KEYWORD},
        {"internal", TokenType::KEYWORD}, {"domain", TokenType::KEYWORD},
        {"struct", TokenType::KEYWORD}, {"interface", TokenType::KEYWORD},
        {"override", TokenType::KEYWORD},
        {"enum", TokenType::KEYWORD}, {"case", TokenType::KEYWORD},
        {"default", TokenType::KEYWORD},
        {"if", TokenType::KEYWORD}, {"else", TokenType::KEYWORD},
        {"switch", TokenType::KEYWORD}, {"fallthrough", TokenType::KEYWORD},
        {"while", TokenType::KEYWORD}, {"for", TokenType::KEYWORD},
        {"in", TokenType::KEYWORD},
        {"try", TokenType::KEYWORD}, {"catch", TokenType::KEYWORD},
        {"await", TokenType::KEYWORD},
        {"import", TokenType::KEYWORD},
        {"expansion", TokenType::KEYWORD},
        {"any", TokenType::KEYWORD},
        {"break", TokenType::KEYWORD}, {"continue", TokenType::KEYWORD},
        {"self", TokenType::KEYWORD}, {"Self", TokenType::KEYWORD},
        {"throw", TokenType::KEYWORD},
        {"super", TokenType::KEYWORD},
        {"return", TokenType::KEYWORD},
        {"weak", TokenType::KEYWORD},
        {"true", TokenType::KEYWORD}, {"false", TokenType::KEYWORD}
    };

    [[nodiscard]] char Peek() const
    {
        return m_position < m_source.length() ? m_source[m_position] : '\0';
    }

    [[nodiscard]] char PeekNext() const
    {
        return m_position + 1 < m_source.length() ? m_source[m_position + 1] : '\0';
    }

    char Advance()
    {
        if (m_position >= m_source.length()) return '\0';

        const char ch = m_source[m_position++];
        if (ch == '\n')
        {
            m_line++;
            m_column = 1;
        }
        else
        {
            m_column++;
        }
        return ch;
    }

    void SkipWhiteSpace()
    {
        while (isspace(Peek())) Advance();
    }

    Token ReadIdentifierOrKeyword()
    {
        const unsigned start = m_position;
        const unsigned startLine = m_line;
        const unsigned startCol = m_column;

        while (isalnum(Peek()) || Peek() == '_')
        {
            Advance();
        }

        std::string value = m_source.substr(start, m_position - start);
        TokenType type = m_keywords.contains(value) ? TokenType::KEYWORD : TokenType::IDENTIFIER;

        return {type, value, startLine, startCol};
    }

    Token ReadNumber()
    {
        const unsigned start = m_position;
        const unsigned startLine = m_line;
        const unsigned startCol = m_column;

        while (isdigit(Peek())) Advance();

        if (Peek() == '.')
        {
            Advance();
            while (isdigit(Peek())) Advance();
            return {
                TokenType::DOUBLE,
                m_source.substr(start, m_position - start),
                startLine,
                startCol
            };
        }
        return {
            TokenType::INTEGER,
            m_source.substr(start, m_position - start),
            startLine,
            startCol
        };
    }

    Token ReadString()
    {
        const unsigned startLine = m_line;
        const unsigned startColumn = m_column;
        Advance();

        std::string value;
        while (Peek() != '"' && Peek() != '\0')
        {
            if (Peek() == '\\')
            {
                Advance();
            }
            value += Advance();
        }

        if (Peek() == '"')
        {
            Advance();
            return {
                TokenType::STRING,
                value,
                startLine,
                startColumn
            };
        }
        return {
            TokenType::ERROR,
            "Unterminated string",
            startLine,
            startColumn
        };
    }

    Token ReadComment()
    {
        const unsigned startLine = m_line;
        const unsigned startCol = m_column;

        Advance();

        if (Peek() == '/')
        {
            Advance();
            std::string value;
            while (Peek() != '\n' && Peek() != '\0') value += Advance();
            return {
                TokenType::COMMENT,
                value,
                startLine,
                startCol
            };
        }
        if (Peek() == '*')
        {
            Advance();
            std::string value;
            while (true)
            {
                if (Peek() == '\0')
                {
                    return {
                        TokenType::ERROR,
                        "Unterminated comment",
                        startLine,
                        startCol
                    };
                }
                if (Peek() == '*' && PeekNext() == '/')
                {
                    Advance();
                    Advance();
                    break;
                }
                value += Advance();
            }
            return {
                TokenType::COMMENT,
                value,
                startLine,
                startCol
            };
        }
        return {
            TokenType::ERROR,
            "Invalid comment",
            startLine,
            startCol
        };
    }

    Token ReadOperator()
    {
        const unsigned startLine = m_line;
        const unsigned startCol = m_column;

        std::string op(1, Advance());
        char next = Peek();

        if ((op == "=" && next == '=') || (op == "!" && next == '=') ||
            (op == "<" && next == '=') || (op == ">" && next == '=') ||
            (op == "&" && next == '&') || (op == "|" && next == '|') ||
            (op == "+" && next == '=') || (op == "-" && next == '=') ||
            (op == "*" && next == '=') || (op == "/" && next == '=') ||
            (op == "-" && next == '>') || (op == "?" && next == '?') ||
            (op == "." && next == '.'))
        {
            op += Advance();
            if (op == ".." && (Peek() == '<' || Peek() == '.'))
            {
                op += Advance();
            }
        }

        TokenType type = TokenType::OPERATOR;
        if (op == "?")
        {
            type = TokenType::OPTIONAL;
        }
        if (op == "!")
        {
            type = TokenType::FORCE_UNWRAP;
        }

        return {
            type,
            op,
            startLine,
            startCol
        };
    }
};