#pragma once
#include <vector>
#include <stdexcept>

#include "AST.h"
#include "../Lexer/Token.h"

class ASTBuilder
{
public:
    explicit ASTBuilder(std::vector<Token> tokens)
        : m_tokens(std::move(tokens))
    {}

    std::vector<std::unique_ptr<Stmt>> Parse()
    {
        std::vector<std::unique_ptr<Stmt>> result;
        while (!IsAtEnd())
        {
            if (Peek().type == TokenType::COMMENT)
            {
                Advance();
                continue;
            }
            result.push_back(ParseDeclaration());
        }
        return result;
    }

private:
    std::vector<Token> m_tokens;
    int m_current = 0;

    [[nodiscard]] Token Peek() const
    {
        return m_tokens[m_current];
    }

    [[nodiscard]] Token Previous() const
    {
        return m_tokens[m_current - 1];
    }

    [[nodiscard]] bool IsAtEnd() const
    {
        return Peek().type == TokenType::END_OF_FILE;
    }

    Token Advance()
    {
        if (!IsAtEnd())
        {
            m_current++;
        }
        return Previous();
    }

    [[nodiscard]] bool Check(const TokenType type) const
    {
        if (IsAtEnd()) return false;
        return Peek().type == type;
    }

    bool Match(const TokenType type)
    {
        if (Check(type))
        {
            Advance();
            return true;
        }
        return false;
    }

    bool MatchValue(const TokenType type, const std::string& value)
    {
        if (Check(type) && Peek().value == value)
        {
            Advance();
            return true;
        }
        return false;
    }

    bool MatchKeyword(const std::string& keyword)
    {
        if (Check(TokenType::KEYWORD) && Peek().value == keyword)
        {
            Advance();
            return true;
        }
        return false;
    }

    Token Consume(const TokenType type, const std::string& message)
    {
        if (Check(type)) return Advance();
        throw std::runtime_error("Parse at line " + std::to_string(Peek().line) + ": " + message + " (got: '" + Peek().value + "')");
    }

    std::unique_ptr<Stmt> ParseDeclaration()
    {
        if (MatchKeyword("let") || MatchKeyword("const"))
            return ParseVarDeclaration(Previous().value == "const");
        return ParseStatement();
    }

    std::unique_ptr<Stmt> ParseVarDeclaration(const bool isConst)
    {
        std::string message = isConst ? "Expected const name" : "Expected variable name";
        auto nameToken = Consume(TokenType::IDENTIFIER, message);
        auto varDecl = std::make_unique<VarDeclStmt>();
        varDecl->name = nameToken.value;
        varDecl->isConst = isConst;

        if (MatchValue(TokenType::SEPARATOR, ":"))
        {
            varDecl->typeName = Consume(TokenType::IDENTIFIER, "Expected type name after ':'").value;
        }

        if (MatchValue(TokenType::OPERATOR, "="))
        {
            varDecl->initExpr = ParseExpression();
        }
        else if (isConst)
        {
            throw std::runtime_error("Constant '" + varDecl->name + "' must be initialized");
        }

        return varDecl;
    }

    std::unique_ptr<Stmt> ParseStatement()
    {
        if (MatchKeyword("if"))
        {
            return ParseIfStatement();
        }
        if (MatchValue(TokenType::SEPARATOR, "{"))
        {
            return ParseBlock();
        }

        auto expr = ParseExpression();
        return std::make_unique<ExprStmt>(std::move(expr));
    }

    std::unique_ptr<Stmt> ParseIfStatement()
    {
        if (!MatchValue(TokenType::SEPARATOR, "("))
        {
            throw std::runtime_error("Expected '(' after 'if'");
        }
        auto condition = ParseExpression();
        if (!MatchValue(TokenType::SEPARATOR, ")"))
        {
            throw std::runtime_error("Expected ')' after condition");
        }

        if (!MatchValue(TokenType::SEPARATOR, "{"))
        {
            throw std::runtime_error("Expected '{' before if block");
        }
        auto trueBlock = ParseBlock();

        auto ifStmt = std::make_unique<IfStmt>();
        ifStmt->condition = std::move(condition);
        ifStmt->trueBlock = std::move(trueBlock);

        if (MatchKeyword("else"))
        {
            if (MatchKeyword("if"))
            {
                ifStmt->falseBlock = ParseIfStatement();
            }
            else
            {
                if (!MatchValue(TokenType::SEPARATOR, "{")) throw std::runtime_error("Expected '{' before else block");
                ifStmt->falseBlock = ParseBlock();
            }
        }
        return ifStmt;
    }

    std::unique_ptr<BlockStmt> ParseBlock()
    {
        auto block = std::make_unique<BlockStmt>();

        while (!Check(TokenType::SEPARATOR) || Peek().value != "}")
        {
            if (IsAtEnd()) throw std::runtime_error("Expected '}' after block");
            if (Peek().type == TokenType::COMMENT)
            {
                Advance(); continue;
            }
            block->statements.push_back(ParseDeclaration());
        }
        Consume(TokenType::SEPARATOR, "Expected '}' after block");
        return block;
    }

    std::unique_ptr<Expr> ParseExpression()
    {
        return ParseEquality();
    }

    std::unique_ptr<Expr> ParseEquality()
    {
        auto expr = ParseComparison();

        while (MatchValue(TokenType::OPERATOR, "==") || MatchValue(TokenType::OPERATOR, "!="))
        {
            std::string op = Previous().value;
            auto right = ParseComparison();
            expr = std::make_unique<BinaryExpr>(op, std::move(expr), std::move(right));
        }
        return expr;
    }

    std::unique_ptr<Expr> ParseComparison()
    {
        auto expr = ParseTerm();

        while (MatchValue(TokenType::OPERATOR, "<") || MatchValue(TokenType::OPERATOR, ">") ||
               MatchValue(TokenType::OPERATOR, "<=") || MatchValue(TokenType::OPERATOR, ">="))
        {
            std::string op = Previous().value;
            auto right = ParseTerm();
            expr = std::make_unique<BinaryExpr>(op, std::move(expr), std::move(right));
        }
        return expr;
    }

    std::unique_ptr<Expr> ParseTerm()
    {
        auto expr = ParseFactor();

        while (MatchValue(TokenType::OPERATOR, "+") || MatchValue(TokenType::OPERATOR, "-"))
        {
            std::string op = Previous().value;
            auto right = ParseFactor();
            expr = std::make_unique<BinaryExpr>(op, std::move(expr), std::move(right));
        }
        return expr;
    }

    std::unique_ptr<Expr> ParseFactor()
    {
        auto expr = ParseCall();

        while (MatchValue(TokenType::OPERATOR, "*") || MatchValue(TokenType::OPERATOR, "/") || MatchValue(TokenType::OPERATOR, "%"))
        {
            std::string op = Previous().value;
            auto right = ParseCall();
            expr = std::make_unique<BinaryExpr>(op, std::move(expr), std::move(right));
        }
        return expr;
    }

    std::unique_ptr<Expr> ParseCall()
    {
        auto expr = ParsePrimary();

        while (true)
        {
            if (MatchValue(TokenType::SEPARATOR, "("))
            {
                std::vector<std::unique_ptr<Expr>> args;
                if (!Check(TokenType::SEPARATOR) || Peek().value != ")")
                {
                    do
                    {
                        args.push_back(ParseExpression());
                    }
                    while (MatchValue(TokenType::SEPARATOR, ","));
                }

                if (!MatchValue(TokenType::SEPARATOR, ")"))
                {
                    throw std::runtime_error("Parse at line " + std::to_string(Peek().line) + ": Expected ')' after arguments");
                }
                expr = std::make_unique<CallExpr>(std::move(expr), std::move(args));
            }
            else
            {
                break;
            }
        }
        return expr;
    }

    std::unique_ptr<Expr> ParsePrimary()
    {
        if (Match(TokenType::INTEGER))
        {
            return std::make_unique<NumberExpr>(std::stod(Previous().value), false);
        }
        if (Match(TokenType::DOUBLE))
        {
            return std::make_unique<NumberExpr>(std::stod(Previous().value), true);
        }
        if (Match(TokenType::IDENTIFIER))
        {
            return std::make_unique<IdentifierExpr>(Previous().value);
        }
        if (Match(TokenType::STRING))
        {
            return std::make_unique<StringExpr>(Previous().value);
        }
        if (MatchKeyword("true"))
        {
            return std::make_unique<BoolExpr>(true);
        }
        if (MatchKeyword("false"))
        {
            return std::make_unique<BoolExpr>(false);
        }

        if (MatchValue(TokenType::SEPARATOR, "("))
        {
            auto expr = ParseExpression();
            if (!MatchValue(TokenType::SEPARATOR, ")")) throw std::runtime_error("Expected ')' after expression");
            return expr;
        }

        throw std::runtime_error("Expected expression at line " + std::to_string(Peek().line) + ", got: '" + Peek().value + "'");
    }
};