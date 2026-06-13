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
        if (MatchKeyword("import"))
        {
            return ParseImport();
        }
        if (MatchKeyword("interface"))
        {
            return ParseInterfaceDeclaration();
        }
        if (MatchKeyword("class"))
        {
            return ParseClassDeclaration();
        }
        if (MatchKeyword("struct"))
        {
            return ParseStructDeclaration();
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

    std::unique_ptr<Stmt> ParseInterfaceDeclaration()
    {
        Token startToken = Previous();
        auto nameToken = Consume(TokenType::IDENTIFIER, "Expected interface name");
        Consume(TokenType::SEPARATOR, "Expected '{' before interface body");

        auto interfaceDecl = std::make_unique<InterfaceDeclStmt>();
        interfaceDecl->line = startToken.line;
        interfaceDecl->column = startToken.column;
        interfaceDecl->name = nameToken.value;

        while (!Check(TokenType::SEPARATOR) || Peek().value != "}")
        {
            if (IsAtEnd()) throw std::runtime_error("Expected '}' after interface body");
            if (Peek().type == TokenType::COMMENT) { Advance(); continue; }

            if (MatchKeyword("func"))
            {
                auto funcNode = ParseFuncDeclaration(false, false);
                interfaceDecl->methods.push_back(
                    std::unique_ptr<FuncDeclStmt>(dynamic_cast<FuncDeclStmt*>(funcNode.release()))
                );
            }
            else if (MatchKeyword("let"))
            {
                Token varStartToken = Previous();
                auto anotherNameToken = Consume(TokenType::IDENTIFIER, "Expected property name in interface");
                Consume(TokenType::SEPARATOR, "Expected ':' after property name");
                std::string typeName = ParseType();

                auto varDecl = std::make_unique<VarDeclStmt>();
                varDecl->line = varStartToken.line;
                varDecl->column = varStartToken.column;
                varDecl->name = anotherNameToken.value;
                varDecl->isConst = true;
                varDecl->typeName = typeName;

                interfaceDecl->properties.push_back(std::move(varDecl));
            }
            else
            {
                throw std::runtime_error("Parse Error: Only 'func' and 'let' are allowed in interfaces.");
            }
        }
        Consume(TokenType::SEPARATOR, "Expected '}' after interface body");

        return interfaceDecl;
    }

    std::unique_ptr<Stmt> ParseClassDeclaration()
    {
        Token startToken = Previous();
        auto nameToken = Consume(TokenType::IDENTIFIER, "Expected class name");
        auto classDecl = std::make_unique<ClassDeclStmt>();

        if (Check(TokenType::SEPARATOR) && Peek().value == ":")
        {
            std::vector<std::string> implementedInterfaces;
            Advance();
            do
            {
                auto idToken = Consume(TokenType::IDENTIFIER, "Expected interface name after ':'");
                implementedInterfaces.push_back(idToken.value);
            }
            while (Check(TokenType::SEPARATOR) && Peek().value == "," && (Advance(), true));
            classDecl->implementedInterfaces = implementedInterfaces;
        }
        Consume(TokenType::SEPARATOR, "Expected '{' before class body");

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

            const auto accessLevel = ParseAccessModifier();

            if (MatchKeyword("init"))
            {
                auto funcNode = ParseFuncDeclaration(true);
                dynamic_cast<FuncDeclStmt*>(funcNode.get())->accessLevel = accessLevel;
                classDecl->members.push_back(std::move(funcNode));
            }
            else if (MatchKeyword("func"))
            {
                auto funcNode = ParseFuncDeclaration(false);
                dynamic_cast<FuncDeclStmt*>(funcNode.get())->accessLevel = accessLevel;
                classDecl->members.push_back(std::move(funcNode));
            }
            else if (MatchKeyword("let") || MatchKeyword("const"))
            {
                auto varNode = ParseVarDeclaration(Previous().value == "const");
                dynamic_cast<VarDeclStmt*>(varNode.get())->accessLevel = accessLevel;
                classDecl->members.push_back(std::move(varNode));
            }
            else
            {
                throw std::runtime_error("Parse Error: Only functions and variables are allowed inside a class body.");
            }
        }
        Consume(TokenType::SEPARATOR, "Expected '}' after class body");

        return classDecl;
    }

    std::unique_ptr<Stmt> ParseStructDeclaration()
    {
        Token startToken = Previous();
        auto nameToken = Consume(TokenType::IDENTIFIER, "Expected struct name");
        auto structDecl = std::make_unique<StructDeclStmt>();

        if (MatchValue(TokenType::SEPARATOR, ":"))
        {
            do
            {
                structDecl->implementedInterfaces.push_back(Consume(TokenType::IDENTIFIER, "Expected interface name").value);
            }
            while (MatchValue(TokenType::SEPARATOR, ","));
        }

        Consume(TokenType::SEPARATOR, "Expected '{' before struct body");

        structDecl->line = startToken.line;
        structDecl->column = startToken.column;
        structDecl->name = nameToken.value;

        while (!Check(TokenType::SEPARATOR) || Peek().value != "}")
        {
            if (IsAtEnd())
            {
                throw std::runtime_error("Expected '}' after struct body");
            }
            if (Peek().type == TokenType::COMMENT)
            {
                Advance();
                continue;
            }

            AccessLevel accessLevel = ParseAccessModifier();

            if (MatchKeyword("static"))
            {
                if (!(MatchKeyword("let") || MatchKeyword("const")))
                    throw std::runtime_error("Expected 'let' or 'const' after 'static'");
                auto varNode = ParseVarDeclaration(Previous().value == "const");
                auto* v = dynamic_cast<VarDeclStmt*>(varNode.get());
                v->accessLevel = accessLevel;
                v->isStatic = true;
                structDecl->members.push_back(std::move(varNode));
            }
            else if (MatchKeyword("let") || MatchKeyword("const"))
            {
                auto varNode = ParseVarDeclaration(Previous().value == "const");
                dynamic_cast<VarDeclStmt*>(varNode.get())->accessLevel = accessLevel;
                structDecl->members.push_back(std::move(varNode));
            }
            else if (MatchKeyword("init"))
            {
                auto funcNode = ParseFuncDeclaration(true);
                dynamic_cast<FuncDeclStmt*>(funcNode.get())->accessLevel = accessLevel;
                structDecl->members.push_back(std::move(funcNode));
            }
            else if (MatchKeyword("func"))
            {
                auto funcNode = ParseFuncDeclaration(false);
                dynamic_cast<FuncDeclStmt*>(funcNode.get())->accessLevel = accessLevel;
                structDecl->members.push_back(std::move(funcNode));
            }
            else
            {
                throw std::runtime_error("Parse Error: Only variables and functions are allowed inside a struct.");
            }
        }
        Consume(TokenType::SEPARATOR, "Expected '}' after struct body");

        return structDecl;
    }

    std::unique_ptr<Stmt> ParseFuncDeclaration(const bool isInit, bool requireBody = true)
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
                std::string paramType = ParseType();

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

        std::unique_ptr<BlockStmt> body = nullptr;
        if (requireBody)
        {
            Consume(TokenType::SEPARATOR, "Expected '{' before function body");
            body = ParseBlock();
        }

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
        else if (MatchValue(TokenType::SEPARATOR, "{"))
        {
            varDecl->computedBody = ParseBlock();
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

    std::unique_ptr<Stmt> ParseIfLetStatement(bool isConst)
    {
        Token startToken = Previous();
        std::string varName = Consume(TokenType::IDENTIFIER, "Expected variable name after 'if let'").value;
        if (!MatchValue(TokenType::OPERATOR, "="))
            throw std::runtime_error("Expected '=' after variable name in 'if let'");
        if (!MatchValue(TokenType::SEPARATOR, "("))
            throw std::runtime_error("Expected '(' after '='");
        auto initExpr = ParseExpression();
        if (!MatchValue(TokenType::SEPARATOR, ")"))
            throw std::runtime_error("Expected ')' after expression in 'if let'");
        if (!MatchValue(TokenType::SEPARATOR, "{"))
            throw std::runtime_error("Expected '{' after 'if let ...'");
        auto trueBlock = ParseBlock();

        auto stmt = std::make_unique<IfLetStmt>();
        stmt->line = startToken.line;
        stmt->column = startToken.column;
        stmt->isConst = isConst;
        stmt->varName = varName;
        stmt->initExpr = std::move(initExpr);
        stmt->trueBlock = std::move(trueBlock);

        if (MatchKeyword("else"))
        {
            if (MatchKeyword("if"))
            {
                stmt->falseBlock = ParseIfStatement();
            }
            else
            {
                if (!MatchValue(TokenType::SEPARATOR, "{")) throw std::runtime_error("Expected '{' before else block");
                stmt->falseBlock = ParseBlock();
            }
        }
        return stmt;
    }

    std::unique_ptr<Stmt> ParseIfStatement()
    {
        Token startToken = Previous();

        if (MatchKeyword("let")) return ParseIfLetStatement(false);
        if (MatchKeyword("const")) return ParseIfLetStatement(true);

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
        auto expr = ParseNilCoalescing();

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

    std::unique_ptr<Expr> ParseNilCoalescing()
    {
        auto expr = ParseLogicOr();

        while (MatchValue(TokenType::OPERATOR, "??"))
        {
            Token opToken = Previous();
            auto right = ParseLogicOr();
            auto newExpr = std::make_unique<BinaryExpr>("??", std::move(expr), std::move(right));
            newExpr->line = opToken.line;
            newExpr->column = opToken.column;
            expr = std::move(newExpr);
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
            if (Check(TokenType::SEPARATOR) && Peek().value == "(")
            {
                Token opToken = Advance();
                std::vector<std::unique_ptr<Expr>> args;
                if (!Check(TokenType::SEPARATOR) || Peek().value != ")")
                {
                    do { args.push_back(ParseExpression()); }
                    while (MatchValue(TokenType::SEPARATOR, ","));
                }
                Consume(TokenType::SEPARATOR, "Expected ')' after arguments");

                auto callExpr = std::make_unique<CallExpr>(std::move(expr), std::move(args));
                callExpr->line = opToken.line;
                callExpr->column = opToken.column;
                expr = std::move(callExpr);
            }
            else if (Check(TokenType::SEPARATOR) && Peek().value == "{")
            {
                auto lambda = ParseShortLambdaExpression();

                if (auto* call = dynamic_cast<CallExpr*>(expr.get()))
                {
                    call->arguments.push_back(std::move(lambda));
                }
                else
                {
                    unsigned line = expr->line;
                    unsigned column = expr->column;

                    std::vector<std::unique_ptr<Expr>> args;
                    args.push_back(std::move(lambda));
                    auto callExpr = std::make_unique<CallExpr>(std::move(expr), std::move(args));

                    callExpr->line = line;
                    callExpr->column = column;
                    expr = std::move(callExpr);
                }
            }
            else if (MatchValue(TokenType::SEPARATOR, "["))
            {
                Token opToken = Previous();
                auto index = ParseExpression();
                if (!MatchValue(TokenType::SEPARATOR, "]")) throw std::runtime_error("Expected ']' after index");
                auto indexExpr = std::make_unique<IndexExpr>(std::move(expr), std::move(index));
                indexExpr->line = opToken.line; indexExpr->column = opToken.column;
                expr = std::move(indexExpr);
            }
            else if (MatchValue(TokenType::SEPARATOR, "."))
            {
                Token opToken = Previous();
                std::string propName = Consume(TokenType::IDENTIFIER, "Expected property name after '.'").value;
                auto getExpr = std::make_unique<GetExpr>(std::move(expr), propName, false);
                getExpr->line = opToken.line; getExpr->column = opToken.column;
                expr = std::move(getExpr);
            }
            else if (Check(TokenType::OPTIONAL) &&
                     m_current + 1 < (int)m_tokens.size() && m_tokens[m_current + 1].type == TokenType::SEPARATOR &&
                     m_tokens[m_current + 1].value == ".")
            {
                Token opToken = Advance();
                Advance();
                std::string propName = Consume(TokenType::IDENTIFIER, "Expected property name after '?.'").value;
                auto getExpr = std::make_unique<GetExpr>(std::move(expr), propName, true);
                getExpr->line = opToken.line; getExpr->column = opToken.column;
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
        else if (!IsAtEnd() && (Peek().value == "null"))
        {
            Advance();
            expr = std::make_unique<NullExpr>();
        }
        else if (!IsAtEnd() && Peek().value == "true")
        {
            Advance();
            expr = std::make_unique<BoolExpr>(true);
        }
        else if (!IsAtEnd() && Peek().value == "false")
        {
            Advance();
            expr = std::make_unique<BoolExpr>(false);
        }
        else if (!IsAtEnd() && Peek().value == "self")
        {
            Advance();
            expr = std::make_unique<IdentifierExpr>("self");
        }
        else if (MatchKeyword("await"))
        {
            auto operand = ParseCall();
            auto awaitExpr = std::make_unique<AwaitExpr>(std::move(operand));
            awaitExpr->line = startToken.line;
            awaitExpr->column = startToken.column;
            return awaitExpr;
        }
        else if (Match(TokenType::IDENTIFIER))
        {
            expr = std::make_unique<IdentifierExpr>(Previous().value);
        }
        else if (Match(TokenType::STRING))
        {
            expr = std::make_unique<StringExpr>(Previous().value);
        }
        else if (Check(TokenType::SEPARATOR) && Peek().value == "(")
        {
            if (CheckIsLambda())
            {
                expr = ParseLambdaExpression();
            }
            else
            {
                Advance();
                expr = ParseExpression();
                if (!MatchValue(TokenType::SEPARATOR, ")"))
                {
                    throw std::runtime_error("Expected ')' after expression");
                }
            }
        }
        else if (Check(TokenType::SEPARATOR) && Peek().value == "{")
        {
            expr = ParseShortLambdaExpression();
        }
        else if (MatchValue(TokenType::SEPARATOR, "["))
        {
            std::vector<std::unique_ptr<Expr>> elements;
            if (!Check(TokenType::SEPARATOR) || Peek().value != "]")
            {
                do { elements.push_back(ParseExpression()); }
                while (MatchValue(TokenType::SEPARATOR, ","));
            }
            if (!MatchValue(TokenType::SEPARATOR, "]")) throw std::runtime_error("Expected ']' after array elements");
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
        std::string typeStr;

        if (MatchValue(TokenType::SEPARATOR, "["))
        {
            std::string elementType = ParseType();
            Consume(TokenType::SEPARATOR, "Expected ']' after array type");
            typeStr = "[" + elementType + "]";
        }
        else if (MatchValue(TokenType::SEPARATOR, "("))
        {
            std::string funcType = "(";
            if (!Check(TokenType::SEPARATOR) || Peek().value != ")")
            {
                do {
                    funcType += ParseType();
                    if (Check(TokenType::SEPARATOR) && Peek().value == ",") {
                        funcType += ", ";
                    }
                } while (MatchValue(TokenType::SEPARATOR, ","));
            }
            Consume(TokenType::SEPARATOR, "Expected ')' after function types");
            funcType += ")";

            if (MatchValue(TokenType::SEPARATOR, ":"))
            {
                funcType += ": " + ParseType();
            }
            else
            {
                funcType += ": Void";
            }

            typeStr = funcType;
        }
        else
        {
            if (MatchKeyword("any"))
            {
                typeStr = "any " + Consume(TokenType::IDENTIFIER, "Expected interface name").value;
            }
            else
            {
                typeStr = Consume(TokenType::IDENTIFIER, "Expected type name").value;
                if (typeStr == "Array" && MatchValue(TokenType::OPERATOR, "<"))
                {
                    std::string elementType = ParseType();
                    if (!MatchValue(TokenType::OPERATOR, ">"))
                    {
                        throw std::runtime_error("Expected '>' after Array type");
                    }
                    typeStr = "Array<" + elementType + ">";
                }
            }
        }

        if (!IsAtEnd() && Peek().value == "?")
        {
            Advance();
            typeStr += "?";
        }

        return typeStr;
    }

    AccessLevel ParseAccessModifier()
    {
        if (MatchKeyword("private"))
        {
            return AccessLevel::Private;
        }
        if (MatchKeyword("internal"))
        {
            return AccessLevel::Internal;
        }
        return AccessLevel::Internal;
    }

    std::unique_ptr<Expr> ParseLambdaExpression()
    {
        Token startToken = Consume(TokenType::SEPARATOR, "Expected '(' for lambda");
        auto closure = std::make_unique<ClosureExpr>();
        closure->line = startToken.line;
        closure->column = startToken.column;

        if (!Check(TokenType::SEPARATOR) || Peek().value != ")")
        {
            do
            {
                std::string paramName = Consume(TokenType::IDENTIFIER, "Expected parameter name in lambda").value;
                Consume(TokenType::SEPARATOR, "Expected ':' after parameter name");
                std::string paramType = ParseType();
                closure->parameters.push_back({paramName, paramType});
            }
            while (MatchValue(TokenType::SEPARATOR, ","));
        }
        Consume(TokenType::SEPARATOR, "Expected ')' after lambda parameters");

        if (MatchValue(TokenType::SEPARATOR, ":"))
        {
            closure->returnType = ParseType();
        }
        else
        {
            closure->returnType = "Any";
        }

        Consume(TokenType::SEPARATOR, "Expected '{' before lambda body");

        auto body = std::make_unique<BlockStmt>();
        while (!Check(TokenType::SEPARATOR) || Peek().value != "}")
        {
            if (IsAtEnd()) throw std::runtime_error("Expected '}' after lambda body");
            if (Peek().type == TokenType::COMMENT) { Advance(); continue; }
            body->statements.push_back(ParseDeclaration());
        }
        Consume(TokenType::SEPARATOR, "Expected '}' after lambda body");

        if (body->statements.size() == 1)
        {
            if (auto* exprStmt = dynamic_cast<ExprStmt*>(body->statements.front().get()))
            {
                auto retStmt = std::make_unique<ReturnStmt>(std::move(exprStmt->expr));
                retStmt->line = exprStmt->line;
                retStmt->column = exprStmt->column;
                body->statements.front() = std::move(retStmt);
            }
        }

        closure->body = std::move(body);
        return closure;
    }

    std::unique_ptr<Expr> ParseShortLambdaExpression()
    {
        Token startToken = Consume(TokenType::SEPARATOR, "Expected '{' for closure");
        auto closure = std::make_unique<ClosureExpr>();
        closure->line = startToken.line;
        closure->column = startToken.column;
        closure->returnType = "Any";

        closure->parameters.push_back({"it", "Any"});

        auto body = std::make_unique<BlockStmt>();
        while (!Check(TokenType::SEPARATOR) || Peek().value != "}")
        {
            if (IsAtEnd()) throw std::runtime_error("Expected '}' after closure body");
            if (Peek().type == TokenType::COMMENT) { Advance(); continue; }
            body->statements.push_back(ParseDeclaration());
        }
        Consume(TokenType::SEPARATOR, "Expected '}' after closure body");

        if (body->statements.size() == 1)
        {
            if (auto* exprStmt = dynamic_cast<ExprStmt*>(body->statements.front().get()))
            {
                auto retStmt = std::make_unique<ReturnStmt>(std::move(exprStmt->expr));
                retStmt->line = exprStmt->line;
                retStmt->column = exprStmt->column;
                body->statements.front() = std::move(retStmt);
            }
        }

        closure->body = std::move(body);
        return closure;
    }

    [[nodiscard]] bool CheckIsLambda() const
    {
        int temp = m_current + 1;

        if (temp < m_tokens.size() && m_tokens[temp].value == ")")
        {
            temp++;
            if (temp < m_tokens.size() && (m_tokens[temp].value == "{" || m_tokens[temp].value == ":"))
            {
                return true;
            }
        }

        if (temp < m_tokens.size() && m_tokens[temp].type == TokenType::IDENTIFIER)
        {
            temp++;
            if (temp < m_tokens.size() && m_tokens[temp].value == ":")
            {
                return true;
            }
        }

        return false;
    }

    std::unique_ptr<Stmt> ParseImport()
    {
        Token moduleToken = Consume(TokenType::IDENTIFIER, "Expected module name after 'import'.");
        return std::make_unique<ImportStmt>(moduleToken.value);
    }
};