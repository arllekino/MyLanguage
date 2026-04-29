#pragma once
#include <string>
#include <vector>

struct ASTNode
{
    virtual ~ASTNode() = default;
};

struct Expr : ASTNode {};
struct Stmt : ASTNode {};

struct NumberExpr : Expr
{
    double value;
    bool isDouble;

    NumberExpr(double val, bool isDouble) : value(val), isDouble(isDouble) {}
};

struct IdentifierExpr : Expr
{
    std::string name;

    explicit IdentifierExpr(std::string name) : name(std::move(name)) {}
};

struct BinaryExpr : Expr
{
    std::string op;
    std::unique_ptr<Expr> left;
    std::unique_ptr<Expr> right;
    BinaryExpr(std::string op, std::unique_ptr<Expr> left, std::unique_ptr<Expr> right)
        : op(std::move(op)), left(std::move(left)), right(std::move(right))
    {};
};

struct BlockStmt : Stmt
{
    std::vector<std::unique_ptr<Stmt>> statements;
};

struct VarDeclStmt: Stmt
{
    bool isConst;
    std::string name;
    std::string typeName; // можеть быть пустым, если тип выводится
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
    explicit ExprStmt(std::unique_ptr<Expr> expr) : expr(std::move(expr)) {}
};

struct StringExpr : Expr {
    std::string value;

    explicit StringExpr(std::string val)
        : value(std::move(val))
    {}
};

struct BoolExpr : Expr {
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