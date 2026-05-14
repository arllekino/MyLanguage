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
        if (MatchKeyword("class"))
        {
            return ParseClassDeclaration();
        }
        if (MatchKeyword("func"))
        {
            return ParseFuncDeclaration(false);
        }
        if (MatchKeyword("let") || MatchKeyword("const"))
        {
            return ParseVarDeclaration(Previous().value == "const");
        }
        return ParseStatement();
    }

    std::unique_ptr<Stmt> ParseClassDeclaration()
    {
        Token startToken = Previous();
        auto nameToken = Consume(TokenType::IDENTIFIER, "Expected class name");
        Consume(TokenType::SEPARATOR, "Expected '{' before class body");

        auto classDecl = std::make_unique<ClassDeclStmt>();
        classDecl->line = startToken.line;
        classDecl->column = startToken.column;
        classDecl->name = nameToken.value;

        while (!Check(TokenType::SEPARATOR) || Peek().value != "}")
        {
            if (IsAtEnd())
            {
                throw std::runtime_error("Expected '}' after class body");
            }
            if (Peek().type == TokenType::COMMENT)
            {
                Advance();
                continue;
            }

            if (MatchKeyword("init"))
            {
                classDecl->members.push_back(ParseFuncDeclaration(true));
            }
            else if (MatchKeyword("func"))
            {
                classDecl->members.push_back(ParseFuncDeclaration(false));
            }
            else if (MatchKeyword("let") || MatchKeyword("const"))
            {
                classDecl->members.push_back(ParseVarDeclaration(Previous().value == "const"));
            }
            else
            {
                throw std::runtime_error("Parse Error: Only functions and variables are allowed inside a class body.");
            }
        }
        Consume(TokenType::SEPARATOR, "Expected '}' after class body");

        return classDecl;
    }

    std::unique_ptr<Stmt> ParseFuncDeclaration(const bool isInit)
    {
        Token startToken = Previous();
        std::string funcName;

        if (isInit)
        {
            funcName = "init";
        }
        else
        {
            funcName = Consume(TokenType::IDENTIFIER, "Expected function name").value;
        }

        Consume(TokenType::SEPARATOR, "Expected '(' before function body");

        std::vector<Parameter> parameters;
        if (!Check(TokenType::SEPARATOR) || Peek().value != ")")
        {
            do
            {
                std::string paramName = Consume(TokenType::IDENTIFIER, "Expected parameter name").value;
                Consume(TokenType::SEPARATOR, "Expected ':' after parameter name");
                std::string paramType = Consume(TokenType::IDENTIFIER, "Expected parameter type").value;

                parameters.push_back({paramName, paramType});
            }
            while (MatchValue(TokenType::SEPARATOR, ","));
        }
        Consume(TokenType::SEPARATOR, "Expected ')' after parameters");

        std::string returnType = "Void";

        if (!isInit && MatchValue(TokenType::SEPARATOR, ":"))
        {
            returnType = ParseType();
        }

        Consume(TokenType::SEPARATOR, "Expected '{' before function body");
        auto body = ParseBlock();

        auto funcDecl = std::make_unique<FuncDeclStmt>();
        funcDecl->line = startToken.line;
        funcDecl->column = startToken.column;
        funcDecl->name = funcName;
        funcDecl->parameters = std::move(parameters);
        funcDecl->returnType = returnType;
        funcDecl->body = std::move(body);

        return funcDecl;
    }

    std::unique_ptr<Stmt> ParseVarDeclaration(const bool isConst)
    {
        Token startToken = Previous();

        bool isWeak = false;
        if (!isConst && MatchKeyword("weak"))
        {
            isWeak = true;
        }

        std::string message = isConst ? "Expected const name" : "Expected variable name";
        auto nameToken = Consume(TokenType::IDENTIFIER, message);
        auto varDecl = std::make_unique<VarDeclStmt>();
        varDecl->line = startToken.line;
        varDecl->column = startToken.column;
        varDecl->name = nameToken.value;
        varDecl->isConst = isConst;
        varDecl->isWeak = isWeak;

        if (MatchValue(TokenType::SEPARATOR, ":"))
        {
            varDecl->typeName = ParseType();
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
        if (MatchKeyword("while"))
        {
            return ParseWhileStatement();
        }
        if (MatchKeyword("for"))
        {
            return ParseForStatement();
        }
        if (MatchKeyword("return"))
        {
            return ParseReturnStatement();
        }
        if (MatchValue(TokenType::SEPARATOR, "{"))
        {
            return ParseBlock();
        }

        Token startToken = Peek();
        auto expr = ParseExpression();
        auto stmt = std::make_unique<ExprStmt>(std::move(expr));
        stmt->line = startToken.line;
        stmt->column = startToken.column;
        return stmt;
    }

    std::unique_ptr<Stmt> ParseIfStatement()
    {
        Token startToken = Previous();
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
        ifStmt->line = startToken.line;
        ifStmt->column = startToken.column;
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

    std::unique_ptr<Stmt> ParseWhileStatement()
    {
        Token startToken = Previous();
        if (!MatchValue(TokenType::SEPARATOR, "("))
        {
            throw std::runtime_error("Expected '(' after 'while'");
        }
        auto condition = ParseExpression();
        if (!MatchValue(TokenType::SEPARATOR, ")"))
        {
            throw std::runtime_error("Expected ')' after condition");
        }

        if (!MatchValue(TokenType::SEPARATOR, "{"))
        {
            throw std::runtime_error("Expected '{' before while block");
        }
        auto body = ParseBlock();

        auto whileStmt = std::make_unique<WhileStmt>();
        whileStmt->line = startToken.line;
        whileStmt->column = startToken.column;
        whileStmt->condition = std::move(condition);
        whileStmt->body = std::move(body);
        return whileStmt;
    }

    std::unique_ptr<Stmt> ParseForStatement()
    {
        Token startToken = Previous();
        Consume(TokenType::SEPARATOR, "Expected '(' after 'for'");

        std::unique_ptr<Stmt> initializer = nullptr;
        if (MatchValue(TokenType::SEPARATOR, ";"))
        {
            initializer = nullptr;
        }
        else if (MatchKeyword("let") || MatchKeyword("const"))
        {
            initializer = ParseVarDeclaration(Previous().value == "const");
            Consume(TokenType::SEPARATOR, "Expected ';' after loop initializer");
        }
        else
        {
            initializer = std::make_unique<ExprStmt>(ParseExpression());
            Consume(TokenType::SEPARATOR, "Expected ';' after loop initializer");
        }

        std::unique_ptr<Expr> condition = nullptr;
        if (!Check(TokenType::SEPARATOR) || Peek().value != ";")
        {
            condition = ParseExpression();
        }
        Consume(TokenType::SEPARATOR, "Expected ';' after loop condition");

        std::unique_ptr<Expr> increment = nullptr;
        if (!Check(TokenType::SEPARATOR) || Peek().value != ")")
        {
            increment = ParseExpression();
        }
        Consume(TokenType::SEPARATOR, "Expected ')' after for clauses");

        auto body = ParseStatement();

        if (increment)
        {
            auto incrementStmt = std::make_unique<ExprStmt>(std::move(increment));
            auto bodyBlock = std::make_unique<BlockStmt>();

            if (auto* block = dynamic_cast<BlockStmt*>(body.get()))
            {
                bodyBlock->statements = std::move(block->statements);
            }
            else
            {
                bodyBlock->statements.push_back(std::move(body));
            }
            bodyBlock->statements.push_back(std::move(incrementStmt));
            body = std::move(bodyBlock);
        }

        if (!condition)
        {
            condition = std::make_unique<BoolExpr>(true);
        }

        auto whileStmt = std::make_unique<WhileStmt>();
        whileStmt->line = startToken.line;
        whileStmt->column = startToken.column;
        whileStmt->condition = std::move(condition);
        whileStmt->body = std::move(body);

        if (initializer)
        {
            auto outerBlock = std::make_unique<BlockStmt>();
            outerBlock->line = startToken.line;
            outerBlock->column = startToken.column;
            outerBlock->statements.push_back(std::move(initializer));
            outerBlock->statements.push_back(std::move(whileStmt));
            return outerBlock;
        }

        return whileStmt;
    }

    std::unique_ptr<Stmt> ParseReturnStatement()
    {
        Token startToken = Previous();
        std::unique_ptr<Expr> value = nullptr;

        if (!Check(TokenType::SEPARATOR) || Peek().value != "}")
        {
            value = ParseExpression();
        }
        auto retStmt = std::make_unique<ReturnStmt>(std::move(value));
        retStmt->line = startToken.line;
        retStmt->column = startToken.column;
        return retStmt;
    }

    std::unique_ptr<BlockStmt> ParseBlock()
    {
        Token startToken = Previous();
        auto block = std::make_unique<BlockStmt>();
        block->line = startToken.line;
        block->column = startToken.column;

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
        return ParseAssignment();
    }

    std::unique_ptr<Expr> ParseAssignment()
    {
        auto expr = ParseLogicOr();

        if (MatchValue(TokenType::OPERATOR, "="))
        {
            Token opToken = Previous();
            auto value = ParseAssignment();

            if (dynamic_cast<IdentifierExpr*>(expr.get()) != nullptr ||
                dynamic_cast<IndexExpr*>(expr.get()) != nullptr)
            {
                auto assignExpr = std::make_unique<AssignExpr>(std::move(expr), std::move(value));
                assignExpr->line = opToken.line;
                assignExpr->column = opToken.column;
                return assignExpr;
            }
            if (auto* getExpr = dynamic_cast<GetExpr*>(expr.get()))
            {
                auto setExpr = std::make_unique<SetExpr>(
                    std::move(getExpr->object),
                    getExpr->propertyName,
                    std::move(value)
                );
                setExpr->line = opToken.line;
                setExpr->column = opToken.column;
                return setExpr;
            }

            throw std::runtime_error("Parse Error: Invalid assignment target at line " + std::to_string(Previous().line));
        }

        return expr;
    }

    std::unique_ptr<Expr> ParseLogicOr()
    {
        auto expr = ParseLogicAnd();

        while (MatchValue(TokenType::OPERATOR, "||"))
        {
            Token opToken = Previous();
            std::string op = opToken.value;
            auto right = ParseLogicAnd();
            auto newExpr = std::make_unique<BinaryExpr>(op, std::move(expr), std::move(right));
            newExpr->line = opToken.line;
            newExpr->column = opToken.column;
            expr = std::move(newExpr);
        }
        return expr;
    }

    std::unique_ptr<Expr> ParseLogicAnd()
    {
        auto expr = ParseEquality();

        while (MatchValue(TokenType::OPERATOR, "&&"))
        {
            Token opToken = Previous();
            std::string op = opToken.value;
            auto right = ParseEquality();
            auto newExpr = std::make_unique<BinaryExpr>(op, std::move(expr), std::move(right));
            newExpr->line = opToken.line;
            newExpr->column = opToken.column;
            expr = std::move(newExpr);
        }
        return expr;
    }

    std::unique_ptr<Expr> ParseEquality()
    {
        auto expr = ParseComparison();

        while (MatchValue(TokenType::OPERATOR, "==") || MatchValue(TokenType::OPERATOR, "!="))
        {
            Token opToken = Previous();
            std::string op = opToken.value;
            auto right = ParseComparison();
            auto newExpr = std::make_unique<BinaryExpr>(op, std::move(expr), std::move(right));
            newExpr->line = opToken.line;
            newExpr->column = opToken.column;
            expr = std::move(newExpr);
        }
        return expr;
    }

    std::unique_ptr<Expr> ParseComparison()
    {
        auto expr = ParseTerm();

        while (MatchValue(TokenType::OPERATOR, "<") || MatchValue(TokenType::OPERATOR, ">") ||
               MatchValue(TokenType::OPERATOR, "<=") || MatchValue(TokenType::OPERATOR, ">="))
        {
            Token opToken = Previous();
            std::string op = opToken.value;
            auto right = ParseTerm();
            auto newExpr = std::make_unique<BinaryExpr>(op, std::move(expr), std::move(right));
            newExpr->line = opToken.line;
            newExpr->column = opToken.column;
            expr = std::move(newExpr);
        }
        return expr;
    }

    std::unique_ptr<Expr> ParseTerm()
    {
        auto expr = ParseFactor();

        while (MatchValue(TokenType::OPERATOR, "+") || MatchValue(TokenType::OPERATOR, "-"))
        {
            Token opToken = Previous();
            std::string op = opToken.value;
            auto right = ParseFactor();
            auto newExpr = std::make_unique<BinaryExpr>(op, std::move(expr), std::move(right));
            newExpr->line = opToken.line;
            newExpr->column = opToken.column;
            expr = std::move(newExpr);
        }
        return expr;
    }

    std::unique_ptr<Expr> ParseFactor()
    {
        auto expr = ParseCall();

        while (MatchValue(TokenType::OPERATOR, "*") || MatchValue(TokenType::OPERATOR, "/") || MatchValue(TokenType::OPERATOR, "%"))
        {
            Token opToken = Previous();
            std::string op = opToken.value;
            auto right = ParseCall();
            auto newExpr = std::make_unique<BinaryExpr>(op, std::move(expr), std::move(right));
            newExpr->line = opToken.line;
            newExpr->column = opToken.column;
            expr = std::move(newExpr);
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
                Token opToken = Previous();
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
                auto callExpr = std::make_unique<CallExpr>(std::move(expr), std::move(args));
                callExpr->line = opToken.line;
                callExpr->column = opToken.column;
                expr = std::move(callExpr);
            }
            else if (MatchValue(TokenType::SEPARATOR, "["))
            {
                Token opToken = Previous();
                auto index = ParseExpression();
                if (!MatchValue(TokenType::SEPARATOR, "]"))
                {
                    throw std::runtime_error("Expected ']' after index");
                }
                auto indexExpr = std::make_unique<IndexExpr>(std::move(expr), std::move(index));
                indexExpr->line = opToken.line;
                indexExpr->column = opToken.column;
                expr = std::move(indexExpr);
            }
            else if (MatchValue(TokenType::SEPARATOR, "."))
            {
                Token opToken = Previous();
                std::string propName = Consume(TokenType::IDENTIFIER, "Expected property name after '.'").value;
                auto getExpr = std::make_unique<GetExpr>(std::move(expr), propName);
                getExpr->line = opToken.line;
                getExpr->column = opToken.column;
                expr = std::move(getExpr);
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
        Token startToken = Peek();
        std::unique_ptr<Expr> expr = nullptr;

        if (Match(TokenType::INTEGER))
        {
            expr = std::make_unique<NumberExpr>(std::stod(Previous().value), false);
        }
        else if (Match(TokenType::DOUBLE))
        {
            expr = std::make_unique<NumberExpr>(std::stod(Previous().value), true);
        }
        else if (Match(TokenType::IDENTIFIER))
        {
            expr = std::make_unique<IdentifierExpr>(Previous().value);
        }
        else if (Match(TokenType::STRING))
        {
            expr = std::make_unique<StringExpr>(Previous().value);
        }
        else if (MatchKeyword("true"))
        {
            expr = std::make_unique<BoolExpr>(true);
        }
        else if (MatchKeyword("false"))
        {
            expr = std::make_unique<BoolExpr>(false);
        }
        else if (MatchKeyword("self"))
        {
            expr = std::make_unique<IdentifierExpr>("self");
        }
        else if (MatchValue(TokenType::SEPARATOR, "("))
        {
            expr = ParseExpression();
            if (!MatchValue(TokenType::SEPARATOR, ")"))
            {
                throw std::runtime_error("Expected ')' after expression");
            }
        }
        else if (MatchValue(TokenType::SEPARATOR, "["))
        {
            std::vector<std::unique_ptr<Expr>> elements;
            if (!Check(TokenType::SEPARATOR) || Peek().value != "]")
            {
                do
                {
                    elements.push_back(ParseExpression());
                }
                while (MatchValue(TokenType::SEPARATOR, ","));
            }
            if (!MatchValue(TokenType::SEPARATOR, "]"))
            {
                throw std::runtime_error("Expected ']' after array elements");
            }
            expr = std::make_unique<ArrayExpr>(std::move(elements));
        }
        else
        {
            throw std::runtime_error("Expected expression at line " + std::to_string(Peek().line) + ", got: '" + Peek().value + "'");
        }

        expr->line = startToken.line;
        expr->column = startToken.column;
        return expr;
    }

    std::string ParseType()
    {
        if (MatchValue(TokenType::SEPARATOR, "["))
        {
            std::string elementType = ParseType();
            Consume(TokenType::SEPARATOR, "Expected ']' after array type");
            return "[" + elementType + "]";
        }
        return Consume(TokenType::IDENTIFIER, "Expected type name").value;
    }
};