#pragma once
#include <string>
#include <vector>
#include <memory>

#include "AccessLevel.h"
#include "../Utils/Parameter.h"

struct NumberExpr;
struct IdentifierExpr;
struct BinaryExpr;
struct StringExpr;
struct BoolExpr;
struct CallExpr;
struct ArrayExpr;
struct IndexExpr;
struct AssignExpr;
struct GetExpr;
struct SetExpr;
struct ClosureExpr;
struct BlockStmt;
struct VarDeclStmt;
struct IfStmt;
struct ExprStmt;
struct WhileStmt;
struct StructDeclStmt;
struct ClassDeclStmt;
struct ReturnStmt;
struct FuncDeclStmt;
struct NullExpr;
struct InterfaceDeclStmt;
struct ImportStmt;

class Visitor
{
public:
    virtual ~Visitor() = default;
    
    virtual void Visit(NumberExpr* node) = 0;
    virtual void Visit(IdentifierExpr* node) = 0;
    virtual void Visit(BinaryExpr* node) = 0;
    virtual void Visit(StringExpr* node) = 0;
    virtual void Visit(BoolExpr* node) = 0;
    virtual void Visit(CallExpr* node) = 0;
    virtual void Visit(ArrayExpr* node) = 0;
    virtual void Visit(IndexExpr* node) = 0;
    virtual void Visit(AssignExpr* node) = 0;
    virtual void Visit(GetExpr* node) = 0;
    virtual void Visit(SetExpr* node) = 0;
    virtual void Visit(ClosureExpr* node) = 0;
    virtual void Visit(NullExpr* node) = 0;

    virtual void Visit(BlockStmt* node) = 0;
    virtual void Visit(VarDeclStmt* node) = 0;
    virtual void Visit(IfStmt* node) = 0;
    virtual void Visit(ExprStmt* node) = 0;
    virtual void Visit(WhileStmt* node) = 0;
    virtual void Visit(StructDeclStmt* node) = 0;
    virtual void Visit(ClassDeclStmt* node) = 0;
    virtual void Visit(ReturnStmt* node) = 0;
    virtual void Visit(FuncDeclStmt* node) = 0;
    virtual void Visit(InterfaceDeclStmt* node) = 0;
    virtual void Visit(ImportStmt* node) = 0;
};

struct ASTNode
{
    unsigned line = 0;
    unsigned column = 0;

    virtual ~ASTNode() = default;
    virtual void Accept(Visitor* v) = 0;
};

struct Expr : ASTNode {};
struct Stmt : ASTNode {};

struct NumberExpr : Expr
{
    double value;
    bool isDouble;

    NumberExpr(double val, bool isDouble) : value(val), isDouble(isDouble) {}
    void Accept(Visitor* v) override { v->Visit(this); }
};

struct IdentifierExpr : Expr
{
    std::string name;

    explicit IdentifierExpr(std::string name) : name(std::move(name)) {}
    void Accept(Visitor* v) override { v->Visit(this); }
};

struct BinaryExpr : Expr
{
    std::string op;
    std::unique_ptr<Expr> left;
    std::unique_ptr<Expr> right;

    BinaryExpr(std::string op, std::unique_ptr<Expr> left, std::unique_ptr<Expr> right)
        : op(std::move(op)), left(std::move(left)), right(std::move(right)) {}
    void Accept(Visitor* v) override { v->Visit(this); }
};

struct BlockStmt : Stmt
{
    std::vector<std::unique_ptr<Stmt>> statements;
    void Accept(Visitor* v) override { v->Visit(this); }
};

struct VarDeclStmt : Stmt
{
    AccessLevel accessLevel = AccessLevel::Internal;
    bool isConst;
    bool isWeak = false;
    std::string name;
    std::string typeName;
    std::unique_ptr<Expr> initExpr;
    std::unique_ptr<BlockStmt> computedBody = nullptr;

    void Accept(Visitor* v) override { v->Visit(this); }
};

struct IfStmt : Stmt
{
    std::unique_ptr<Expr> condition;
    std::unique_ptr<BlockStmt> trueBlock;
    std::unique_ptr<Stmt> falseBlock;
    void Accept(Visitor* v) override { v->Visit(this); }
};

struct ExprStmt : Stmt
{
    std::unique_ptr<Expr> expr;

    explicit ExprStmt(std::unique_ptr<Expr> expr) : expr(std::move(expr)) {}
    void Accept(Visitor* v) override { v->Visit(this); }
};

struct StringExpr : Expr
{
    std::string value;

    explicit StringExpr(std::string val) : value(std::move(val)) {}
    void Accept(Visitor* v) override { v->Visit(this); }
};

struct BoolExpr : Expr
{
    bool value;
    explicit BoolExpr(bool val) : value(val) {}
    void Accept(Visitor* v) override { v->Visit(this); }
};

struct CallExpr : Expr
{
    std::unique_ptr<Expr> callee;
    std::vector<std::unique_ptr<Expr>> arguments;
    CallExpr(std::unique_ptr<Expr> callee, std::vector<std::unique_ptr<Expr>> args)
        : callee(std::move(callee)), arguments(std::move(args)) {}
    void Accept(Visitor* v) override { v->Visit(this); }
};

struct WhileStmt : Stmt
{
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> body;
    void Accept(Visitor* v) override { v->Visit(this); }
};

struct ArrayExpr : Expr
{
    std::vector<std::unique_ptr<Expr>> elements;
    explicit ArrayExpr(std::vector<std::unique_ptr<Expr>> elements) : elements(std::move(elements)) {}
    void Accept(Visitor* v) override { v->Visit(this); }
};

struct IndexExpr : Expr
{
    std::unique_ptr<Expr> array;
    std::unique_ptr<Expr> index;
    IndexExpr(std::unique_ptr<Expr> array, std::unique_ptr<Expr> index)
        : array(std::move(array)), index(std::move(index)) {}
    void Accept(Visitor* v) override { v->Visit(this); }
};

struct AssignExpr : Expr
{
    std::unique_ptr<Expr> target;
    std::unique_ptr<Expr> value;

    AssignExpr(std::unique_ptr<Expr> target, std::unique_ptr<Expr> value)
        : target(std::move(target)), value(std::move(value)) {}
    void Accept(Visitor* v) override { v->Visit(this); }
};

struct StructDeclStmt : Stmt
{
    std::string name;
    std::vector<std::string> implementedInterfaces;
    std::vector<std::unique_ptr<Stmt>> members;

    void Accept(Visitor *v) override { v->Visit(this); }
};

struct ClassDeclStmt : Stmt
{
    std::string name;
    std::vector<std::string> implementedInterfaces;
    std::vector<std::unique_ptr<Stmt>> members;

    void Accept(Visitor* v) override { v->Visit(this); }
};

struct GetExpr : Expr
{
    std::unique_ptr<Expr> object;
    std::string propertyName;

    GetExpr(std::unique_ptr<Expr> object, std::string propertyName)
        : object(std::move(object)), propertyName(std::move(propertyName)) {}
    void Accept(Visitor* v) override { v->Visit(this); }
};

struct SetExpr : Expr
{
    std::unique_ptr<Expr> object;
    std::string propertyName;
    std::unique_ptr<Expr> value;

    SetExpr(std::unique_ptr<Expr> object, std::string propertyName, std::unique_ptr<Expr> value)
        : object(std::move(object)), propertyName(std::move(propertyName)), value(std::move(value)) {}
    void Accept(Visitor* v) override { v->Visit(this); }
};

struct ClosureExpr : Expr
{
    std::vector<Parameter> parameters;
    std::string returnType;
    std::unique_ptr<BlockStmt> body;

    void Accept(Visitor* v) override { v->Visit(this); }
};

struct ReturnStmt : Stmt
{
    std::unique_ptr<Expr> value;

    explicit ReturnStmt(std::unique_ptr<Expr> value) : value(std::move(value)) {}
    void Accept(Visitor* v) override { v->Visit(this); }
};

struct FuncDeclStmt : Stmt
{
    AccessLevel accessLevel = AccessLevel::Internal;
    std::string name;
    std::vector<Parameter> parameters;
    std::string returnType;
    std::unique_ptr<BlockStmt> body;
    void Accept(Visitor* v) override { v->Visit(this); }
};

struct NullExpr : Expr
{
    void Accept(Visitor* v) override { v->Visit(this); }
};

struct InterfaceDeclStmt : Stmt
{
    std::string name;
    std::vector<std::unique_ptr<FuncDeclStmt>> methods;
    std::vector<std::unique_ptr<VarDeclStmt>> properties;

    void Accept(Visitor* v) override { v->Visit(this); }
};

struct ImportStmt : Stmt
{
    std::string moduleName;

    explicit ImportStmt(std::string name)
        : moduleName(std::move(name)) {}

    void Accept(Visitor* v) override { v->Visit(this); }
};