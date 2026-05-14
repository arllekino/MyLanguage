#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdlib>

#include "../ASTBuilder/AST.h"

class ASMGenerator
{
public:
    void CompileAndRun(const std::vector<std::unique_ptr<Stmt>>& ast, const std::string& filename)
    {
        std::ofstream out(filename);
        if (!out.is_open())
        {
            throw std::runtime_error("Could not open ASM output file: " + filename);
        }

        out << ".section __TEXT,__text,regular,pure_instructions" << std::endl;
        out << ".global _main" << std::endl;
        out << ".align 2" << std::endl << std::endl;
        out << "_main:" << std::endl;

        out << "  stp x29, x30, [sp, #-16]!" << std::endl;
        out << "  mov x29, sp" << std::endl << std::endl;

        for (const auto& stmt : ast)
        {
            GenerateStmt(stmt.get(), out);
        }

        out << std::endl << "  mov w0, #0" << std::endl;
        out << "  ldp x29, x30, [sp], #16" << std::endl;
        out << "  ret" << std::endl << std::endl;

        out << ".section __DATA,__data" << std::endl;
        out << "fmt_int: .asciz \"%lld\\n\"\n";
        out.close();

        std::string exeName = filename.substr(0, filename.find_last_of('.')) + "_exec";

        std::string compileCmd = "clang -arch arm64 " + filename + " -o " + exeName;
        int compileResult = std::system(compileCmd.c_str());

        if (compileResult == 0)
        {
            std::system(exeName.c_str());
        }
        else
        {
            std::cerr << "[ASM] Compilation failed.\n";
        }
    }

private:
    int m_labelCount = 0;

    void GenerateStmt(Stmt* stmt, std::ostream& out)
    {
        if (auto* exprStmt = dynamic_cast<ExprStmt*>(stmt))
        {
            if (auto* call = dynamic_cast<CallExpr*>(exprStmt->expr.get()))
            {
                if (auto* ident = dynamic_cast<IdentifierExpr*>(call->callee.get()))
                {
                    if (ident->name == "print" && !call->arguments.empty())
                    {
                        GeneratePrint(call->arguments[0].get(), out);
                        return;
                    }
                }
            }

            GenerateExpr(exprStmt->expr.get(), out);
            out << "  add sp, sp, #16" << std::endl;
        }
        else if (auto* ifStmt = dynamic_cast<IfStmt*>(stmt))
        {
            int id = m_labelCount++;
            GenerateExpr(ifStmt->condition.get(), out);

            out << "  ldr x0, [sp], #16" << std::endl;
            out << "  cmp x0, #0" << std::endl;
            out << "  b.eq L_else_" << id << std::endl << std::endl;

            GenerateStmt(ifStmt->trueBlock.get(), out);
            out << "  b L_end_" << id << std::endl << std::endl;

            out << "L_else_" << id << ":" << std::endl;
            if (ifStmt->falseBlock)
            {
                GenerateStmt(ifStmt->falseBlock.get(), out);
            }

            out << "L_end_" << id << ":" << std::endl;
        }
        else if (auto* whileStmt = dynamic_cast<WhileStmt*>(stmt))
        {
            int id = m_labelCount++;

            out << "L_while_start_" << id << ":" << std::endl;

            GenerateExpr(whileStmt->condition.get(), out);
            out << "  ldr x0, [sp], #16" << std::endl;
            out << "  cmp x0, #0" << std::endl;
            out << "  b.eq L_while_end_" << id << std::endl << std::endl;

            GenerateStmt(whileStmt->body.get(), out);

            out << "  b L_while_start_" << id << std::endl;
            out << "L_while_end_" << id << ":" << std::endl;
        }
        else if (const auto* block = dynamic_cast<BlockStmt*>(stmt))
        {
            for (const auto& s : block->statements)
            {
                GenerateStmt(s.get(), out);
            }
        }
        else
        {
            throw std::runtime_error("ASM Error: Unsupported statement type in ASMGenerator!");
        }
    }

    void GeneratePrint(Expr* expr, std::ostream& out)
    {
        GenerateExpr(expr, out);

        out << "  adrp x0, fmt_int@PAGE" << std::endl;
        out << "  add x0, x0, fmt_int@PAGEOFF" << std::endl;

        out << "  bl _printf" << std::endl;

        out << "  add sp, sp, #16" << std::endl << std::endl;
    }

    void GenerateExpr(Expr* expr, std::ostream& out)
    {
        if (const auto* num = dynamic_cast<NumberExpr*>(expr))
        {
            auto val = static_cast<int64_t>(num->value);
            out << "  mov x0, #" << val << std::endl;
            out << "  str x0, [sp, #-16]!" << std::endl;
        }
        else if (const auto* bin = dynamic_cast<BinaryExpr*>(expr))
        {
            GenerateExpr(bin->left.get(), out);
            GenerateExpr(bin->right.get(), out);

            out << "  ldr x1, [sp], #16" << std::endl;
            out << "  ldr x0, [sp], #16" << std::endl;

            if (bin->op == "+")
                out << "  add x0, x0, x1" << std::endl;
            else if (bin->op == "-")
                out << "  sub x0, x0, x1" << std::endl;
            else if (bin->op == "*")
                out << "  mul x0, x0, x1" << std::endl;
            else if (bin->op == "/")
                out << "  sdiv x0, x0, x1" << std::endl;
            else if (bin->op == "=="
                || bin->op == "!="
                || bin->op == "<"
                || bin->op == ">"
                || bin->op == "<="
                || bin->op == ">=")
            {
                out << "  cmp x0, x1" << std::endl;
                if (bin->op == "==")
                    out << "  cset x0, eq" << std::endl;
                else if (bin->op == "!=")
                    out << "  cset x0, ne" << std::endl;
                else if (bin->op == "<")
                    out << "  cset x0, lt" << std::endl;
                else if (bin->op == ">")
                    out << "  cset x0, gt" << std::endl;
                else if (bin->op == "<=")
                    out << "  cset x0, le" << std::endl;
                else if (bin->op == ">=")
                    out << "  cset x0, ge" << std::endl;
            }

            out << "  str x0, [sp, #-16]!" << std::endl;
        }
        else if (auto* b = dynamic_cast<BoolExpr*>(expr))
        {
            int64_t val = b->value ? 1 : 0;
            out << "  mov x0, #" << val << std::endl;
            out << "  str x0, [sp, #-16]!" << std::endl;
        }
        else
        {
            throw std::runtime_error("ASM Error: Unsupported expression type in ASMGenerator!");
        }
    }
};