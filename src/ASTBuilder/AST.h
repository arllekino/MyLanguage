#pragma once
#include <string>
#include <vector>

#include "../Utils/Parameter.h"

struct ASTNode
{
    unsigned line = 0;
    unsigned column = 0;

    virtual ~ASTNode() = default;
};

struct Expr : ASTNode {};
struct Stmt : ASTNode {};

struct NumberExpr : Expr
{
    double value;
    bool isDouble;

    NumberExpr(double val, bool isDouble)
        : value(val),
        isDouble(isDouble)
    {}
};

struct IdentifierExpr : Expr
{
    std::string name;

    explicit IdentifierExpr(std::string name)
        : name(std::move(name))
    {}
};

struct BinaryExpr : Expr
{
    std::string op;
    std::unique_ptr<Expr> left;
    std::unique_ptr<Expr> right;

    BinaryExpr(std::string op, std::unique_ptr<Expr> left, std::unique_ptr<Expr> right)
        : op(std::move(op)),
        left(std::move(left)),
        right(std::move(right))
    {};
};

struct BlockStmt : Stmt
{
    std::vector<std::unique_ptr<Stmt>> statements;
};

struct VarDeclStmt: Stmt
{
    bool isConst;
    bool isWeak = false;
    std::string name;
    std::string typeName;
    std::unique_ptr<Expr> initExpr;
};

struct IfStmt : Stmt
{
    std::unique_ptr<Expr> condition;
    std::unique_ptr<BlockStmt> trueBlock;
    std::unique_ptr<Stmt> falseBlock;
};

struct ExprStmt : Stmt
{
    std::unique_ptr<Expr> expr;

    explicit ExprStmt(std::unique_ptr<Expr> expr)
        : expr(std::move(expr))
    {}
};

struct StringExpr : Expr
{
    std::string value;

    explicit StringExpr(std::string val)
        : value(std::move(val))
    {}
};

struct BoolExpr : Expr
{
    bool value;
    explicit BoolExpr(bool val) : value(val) {}
};

struct CallExpr : Expr
{
    std::unique_ptr<Expr> callee;
    std::vector<std::unique_ptr<Expr>> arguments;
    CallExpr(std::unique_ptr<Expr> callee, std::vector<std::unique_ptr<Expr>> args)
        : callee(std::move(callee)),
        arguments(std::move(args))
    {}
};

struct WhileStmt : Stmt
{
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> body;
};

struct ArrayExpr : Expr
{
    std::vector<std::unique_ptr<Expr>> elements;
    explicit ArrayExpr(std::vector<std::unique_ptr<Expr>> elements)
        : elements(std::move(elements))
    {}
};

struct IndexExpr : Expr
{
    std::unique_ptr<Expr> array;
    std::unique_ptr<Expr> index;
    IndexExpr(std::unique_ptr<Expr> array, std::unique_ptr<Expr> index)
        : array(std::move(array)),
        index(std::move(index))
    {}
};

struct AssignExpr : Expr
{
    std::unique_ptr<Expr> target;
    std::unique_ptr<Expr> value;

    AssignExpr(std::unique_ptr<Expr> target, std::unique_ptr<Expr> value)
        : target(std::move(target)),
        value(std::move(value))
    {}
};

struct ClassDeclStmt : Stmt
{
    std::string name;
    std::vector<std::unique_ptr<Stmt>> members;
};

struct GetExpr : Expr
{
    std::unique_ptr<Expr> object;
    std::string propertyName;

    GetExpr(std::unique_ptr<Expr> object, std::string propertyName)
        : object(std::move(object)),
        propertyName(std::move(propertyName))
    {}
};

struct SetExpr : Expr
{
    std::unique_ptr<Expr> object;
    std::string propertyName;
    std::unique_ptr<Expr> value;

    SetExpr(std::unique_ptr<Expr> object, std::string propertyName, std::unique_ptr<Expr> value)
        : object(std::move(object)),
        propertyName(std::move(propertyName)),
        value(std::move(value))
    {}
};

struct ReturnStmt : Stmt
{
    std::unique_ptr<Expr> value;

    explicit ReturnStmt(std::unique_ptr<Expr> value)
        : value(std::move(value))
    {}
};

struct FuncDeclStmt : Stmt
{
    std::string name;
    std::vector<Parameter> parameters;
    std::string returnType;
    std::unique_ptr<BlockStmt> body;
};