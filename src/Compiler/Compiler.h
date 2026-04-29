#pragma once
#include <memory>
#include <vector>
#include <unordered_map>
#include <stdexcept>
#include "../ASTBuilder/AST.h"
#include "../VirtualMachine/VirtualMachine.h"
#include "../VirtualMachine/OpCode.h"

class Compiler
{
public:
    Compiler()
    {
        m_mainFunc = std::make_shared<Function>();
        m_mainFunc->name = "main";
        m_mainFunc->chunk = std::make_unique<Chunk>();
    }

    FunctionPtr Compile(const std::vector<std::unique_ptr<Stmt>>& ast)
    {
        for (const auto& stmt : ast)
        {
            CompileStmt(stmt.get());
        }

        Emit(OP_CONSTANT);
        Emit(MakeConstant(0));
        Emit(OP_RETURN);

        return m_mainFunc;
    }

private:
    FunctionPtr m_mainFunc;

    struct Local {
        std::string name;
        int depth;
    };

    std::vector<Local> m_locals;
    int m_scopeDepth = 0;

    void BeginScope()
    {
        m_scopeDepth++;
    }

    void EndScope()
    {
        m_scopeDepth--;
        while (!m_locals.empty() && m_locals.back().depth > m_scopeDepth)
        {
            Emit(OP_POP);
            m_locals.pop_back();
        }
    }

    int EmitJump(uint8_t instruction)
    {
        Emit(instruction);
        Emit(0xff);
        Emit(0xff);
        return static_cast<int>(m_mainFunc->chunk->code.size() - 2);
    }

    void PatchJump(int offset)
    {
        size_t target = m_mainFunc->chunk->code.size();
        if (target > UINT16_MAX)
        {
            throw std::runtime_error("Too much code to jump over.");
        }

        m_mainFunc->chunk->code[offset] = (target >> 8) & 0xff;
        m_mainFunc->chunk->code[offset + 1] = target & 0xff;
    }

    void CompileStmt(Stmt* stmt)
    {
        if (auto* exprStmt = dynamic_cast<ExprStmt*>(stmt))
        {
            CompileExpr(exprStmt->expr.get());
            Emit(OP_POP);
        }
        else if (auto* varDecl = dynamic_cast<VarDeclStmt*>(stmt))
        {
            if (!varDecl->typeName.empty())
            {
                const std::string& typeName = varDecl->typeName;

                // TODO: добавить классы/структуры/alias
                if (typeName != "Int" && typeName != "Double" && typeName != "String" && typeName != "Bool")
                {
                    throw std::runtime_error("Compiler Error: Unknown type '" + typeName + "'");
                }

                if (varDecl->initExpr)
                {
                    if (auto* num = dynamic_cast<NumberExpr*>(varDecl->initExpr.get()))
                    {
                        if (num->isDouble && typeName != "Double")
                        {
                            throw std::runtime_error("Type Error: Cannot assign Double to '" + typeName + "'");
                        }
                        if (!num->isDouble && typeName != "Int")
                        {
                            throw std::runtime_error("Type Error: Cannot assign Int to '" + typeName + "'");
                        }
                    }
                    else if (auto* str = dynamic_cast<StringExpr*>(varDecl->initExpr.get()))
                    {
                        if (typeName != "String")
                        {
                            throw std::runtime_error("Type Error: Cannot assign String to '" + typeName + "'");
                        }
                    }
                    else if (auto* b = dynamic_cast<BoolExpr*>(varDecl->initExpr.get()))
                    {
                        if (typeName != "Bool")
                        {
                            throw std::runtime_error("Type Error: Cannot assign Bool to '" + typeName + "'");
                        }
                    }
                }
            }

            if (varDecl->initExpr)
            {
                CompileExpr(varDecl->initExpr.get());
            }
            else
            {
                Emit(OP_CONSTANT);
                Emit(MakeConstant(false));
            }

            int slot = m_locals.size();
            Emit(OP_SET_LOCAL);
            Emit(static_cast<uint8_t>(slot));
            Emit(OP_POP);

            m_locals.push_back({varDecl->name, m_scopeDepth});
        }
        else if (auto* blockStmt = dynamic_cast<BlockStmt*>(stmt))
        {
            BeginScope();
            for (const auto& s : blockStmt->statements)
            {
                CompileStmt(s.get());
            }
            EndScope();
        }
        else if (auto* ifStmt = dynamic_cast<IfStmt*>(stmt))
        {
            CompileExpr(ifStmt->condition.get());

            int jumpIfFalse = EmitJump(OP_JUMP_IF_FALSE);

            CompileStmt(ifStmt->trueBlock.get());

            if (ifStmt->falseBlock)
            {
                int jumpToEnd = EmitJump(OP_JUMP);
                PatchJump(jumpIfFalse);

                CompileStmt(ifStmt->falseBlock.get());
                PatchJump(jumpToEnd);
            }
            else
            {
                PatchJump(jumpIfFalse);
            }
        }
    }

    void CompileExpr(Expr* expr)
    {
        if (auto* num = dynamic_cast<NumberExpr*>(expr))
        {
            Emit(OP_CONSTANT);
            if (num->isDouble)
            {
                Emit(MakeConstant(num->value));
            }
            else
            {
                Emit(MakeConstant(static_cast<int64_t>(num->value)));
            }
        }
        else if (auto* b = dynamic_cast<BoolExpr*>(expr))
        {
            Emit(OP_CONSTANT);
            Emit(MakeConstant(b->value));
        }
        else if (auto* str = dynamic_cast<StringExpr*>(expr))
        {
            Emit(OP_CONSTANT);
            Emit(MakeConstant(std::make_shared<std::string>(str->value)));
        }
        else if (auto* bin = dynamic_cast<BinaryExpr*>(expr))
        {
            CompileExpr(bin->left.get());
            CompileExpr(bin->right.get());

            if (bin->op == "+") Emit(OP_ADD);
            else if (bin->op == "-") Emit(OP_SUB);
            else if (bin->op == "*") Emit(OP_MUL);
            else if (bin->op == "/") Emit(OP_DIV);
            else if (bin->op == "%") Emit(OP_MOD);
            else if (bin->op == "==") Emit(OP_EQUAL);
            else if (bin->op == "<") Emit(OP_LESS);
            else if (bin->op == ">") Emit(OP_GREATER);
            else if (bin->op == "<=") { Emit(OP_GREATER); Emit(OP_NOT); }
            else if (bin->op == ">=") { Emit(OP_LESS); Emit(OP_NOT); }
            else if (bin->op == "!=") { Emit(OP_EQUAL); Emit(OP_NOT); }
        }
        else if (auto* ident = dynamic_cast<IdentifierExpr*>(expr))
        {
            int slot = -1;
            for (int i = static_cast<int>(m_locals.size()) - 1; i >= 0; --i)
                {
                if (m_locals[i].name == ident->name) {
                    slot = i; break;
                }
            }

            if (slot != -1) {
                Emit(OP_GET_LOCAL);
                Emit(static_cast<uint8_t>(slot));
            } else {
                Emit(OP_GET_GLOBAL);
                Emit(MakeConstant(std::make_shared<std::string>(ident->name)));
            }
        }
        else if (auto* call = dynamic_cast<CallExpr*>(expr))
        {
            CompileExpr(call->callee.get());

            for (const auto& arg : call->arguments) {
                CompileExpr(arg.get());
            }

            Emit(OP_CALL);
            Emit(static_cast<uint8_t>(call->arguments.size()));
        }
    }

    void Emit(uint8_t byte)
    {
        m_mainFunc->chunk->code.push_back(byte);
    }

    uint8_t MakeConstant(const Value& value)
    {
        m_mainFunc->chunk->constants.push_back(value);
        return static_cast<uint8_t>(m_mainFunc->chunk->constants.size() - 1);
    }
};