#pragma once
#include <memory>
#include <vector>
#include <unordered_map>
#include <stdexcept>
#include "../ASTBuilder/AST.h"
#include "../Utils/TypeInfo.h"
#include "../VirtualMachine/VirtualMachine.h"
#include "../VirtualMachine/OpCode.h"

class Compiler : public Visitor
{
public:
    Compiler()
    {
        m_functions["print"] = {
            TypeInfo::Simple(TypeKind::Void), { TypeInfo::Simple(TypeKind::Any) }
        };
    }

    FunctionPtr Compile(const std::vector<std::unique_ptr<Stmt>>& ast)
    {
        InitState("main", 0, TypeInfo::Simple(TypeKind::Void));

        for (const auto& stmt : ast)
        {
            CompileStmt(stmt.get());
        }

        Emit(OP_CONSTANT);
        Emit(MakeConstant(0));
        Emit(OP_RETURN);

        return EndState();
    }

private:
    struct Local
    {
        std::string name;
        int depth;
        bool isConst;
        TypeInfoPtr type;
    };

    struct CompilerState
    {
        FunctionPtr function;
        std::vector<Local> locals;
        int maxLocals = 0;
        int scopeDepth = 0;
        TypeInfoPtr returnType;
        TypeInfoPtr currentClass;
        CompilerState* parent = nullptr;
    };

    struct GlobalVar
    {
        bool isConst{};
        TypeInfoPtr type;
    };

    struct FuncInfo
    {
        TypeInfoPtr returnType;
        std::vector<TypeInfoPtr> paramTypes;
    };

    struct FieldInfo
    {
        TypeInfoPtr type;
        bool isWeak{};
    };

    struct ClassInfo
    {
        std::string name;
        std::unordered_map<std::string, FieldInfo> fields;
        std::unordered_map<std::string, FuncInfo> methods;
    };

    CompilerState* m_current = nullptr;
    std::unordered_map<std::string, GlobalVar> m_globalVariables;
    std::unordered_map<std::string, ClassInfo> m_classes;
    std::unordered_map<std::string, FuncInfo> m_functions;
    
    TypeInfoPtr m_lastExprType = TypeInfo::Simple(TypeKind::Any);

    TypeInfoPtr ResolveType(const std::string& name)
    {
        if (name.empty() || name == "Any") return TypeInfo::Simple(TypeKind::Any);

        if (name.front() == '[' && name.back() == ']')
        {
            std::string inner = name.substr(1, name.size() - 2);
            auto type = TypeInfo::Simple(TypeKind::Array);
            type->elementType = ResolveType(inner);
            return type;
        }

        if (name == "Void")
        {
            return TypeInfo::Simple(TypeKind::Void);
        }
        if (name == "Int")
        {
            return TypeInfo::Simple(TypeKind::Int);
        }
        if (name == "Double")
        {
            return TypeInfo::Simple(TypeKind::Double);
        }
        if (name == "Bool")
        {
            return TypeInfo::Simple(TypeKind::Bool);
        }
        if (name == "String")
        {
            return TypeInfo::Simple(TypeKind::String);
        }
        if (name == "Array")
        {
            return TypeInfo::Simple(TypeKind::Array);
        }
        if (m_classes.contains(name))
        {
            return TypeInfo::Class(name);
        }

        throw std::runtime_error("Unknown type '" + name + "'");
    }

    void InitState(const std::string& name, int arity, TypeInfoPtr returnType, TypeInfoPtr currentClass = nullptr)
    {
        auto* newState = new CompilerState();
        newState->parent = m_current;
        newState->returnType = std::move(returnType);
        newState->currentClass = std::move(currentClass);
        newState->function = std::make_shared<Function>();
        newState->function->name = name;
        newState->function->arity = arity;
        newState->function->chunk = std::make_unique<Chunk>();

        newState->locals.push_back({name, 0, true, TypeInfo::Simple(TypeKind::Any)});
        newState->maxLocals = 1;
        m_current = newState;
    }

    FunctionPtr EndState()
    {
        FunctionPtr func = m_current->function;
        func->maxLocals = m_current->maxLocals;

        CompilerState* enclosing = m_current->parent;
        delete m_current;
        m_current = enclosing;

        return func;
    }

    void Emit(uint8_t byte)
    {
        m_current->function->chunk->code.push_back(byte);
    }

    uint8_t MakeConstant(const Value& value)
    {
        m_current->function->chunk->constants.push_back(value);
        return static_cast<uint8_t>(m_current->function->chunk->constants.size() - 1);
    }

    void BeginScope()
    {
        m_current->scopeDepth++;
    }

    void EndScope()
    {
        m_current->scopeDepth--;
        while (!m_current->locals.empty() && m_current->locals.back().depth > m_current->scopeDepth)
        {
            Emit(OP_POP);
            m_current->locals.pop_back();
        }
    }

    int EmitJump(uint8_t instruction)
    {
        Emit(instruction);
        Emit(0xff);
        Emit(0xff);

        return static_cast<int>(m_current->function->chunk->code.size() - 2);
    }

    void PatchJump(int offset)
    {
        size_t target = m_current->function->chunk->code.size();
        if (target > UINT16_MAX)
        {
            throw std::runtime_error("Too much code to jump over.");
        }
        m_current->function->chunk->code[offset] = (target >> 8) & 0xff;
        m_current->function->chunk->code[offset + 1] = target & 0xff;
    }

    [[nodiscard]] bool isGlobalScope() const
    {
        return m_current->parent == nullptr && m_current->scopeDepth == 0;
    }

    void CompileStmt(Stmt* stmt)
    {
        stmt->Accept(this);
    }

    TypeInfoPtr CompileExpr(Expr* expr)
    {
        expr->Accept(this);
        return m_lastExprType;
    }

    void Visit(ExprStmt* exprStmt) override
    {
        CompileExpr(exprStmt->expr.get());
        Emit(OP_POP);
    }

    void Visit(VarDeclStmt* varDecl) override
    {
        TypeInfoPtr initType = TypeInfo::Simple(TypeKind::Any);

        if (varDecl->initExpr)
        {
            initType = CompileExpr(varDecl->initExpr.get());
        }
        else
        {
            Emit(OP_CONSTANT);
            Emit(MakeConstant(false)); // по-хорошему здесь должен быть Null{}
            initType = TypeInfo::Simple(TypeKind::Optional);
        }

        TypeInfoPtr finalType = initType;
        if (!varDecl->typeName.empty())
        {
            finalType = ResolveType(varDecl->typeName);

            if (!finalType->IsCompatible(initType))
            {
                throw std::runtime_error("Type Error at line " + std::to_string(varDecl->line) + ": Cannot initialize variable '" + varDecl->name +
                    "' with incompatible type.");
            }
        }

        if (isGlobalScope())
        {
            Emit(OP_DEFINE_GLOBAL);
            Emit(MakeConstant(std::make_shared<std::string>(varDecl->name)));
            m_globalVariables[varDecl->name] = { varDecl->isConst, finalType };
        }
        else
        {
            const auto slot = static_cast<int>(m_current->locals.size());
            Emit(OP_SET_LOCAL);
            Emit(static_cast<uint8_t>(slot));
            Emit(OP_POP);
            m_current->locals.push_back({varDecl->name, m_current->scopeDepth, varDecl->isConst, finalType});
            m_current->maxLocals = std::max(m_current->maxLocals, static_cast<int>(m_current->locals.size()));
        }
    }

    void Visit(FuncDeclStmt* funcDecl) override
    {
        std::vector<TypeInfoPtr> paramTypes;
        paramTypes.reserve(funcDecl->parameters.size());
        for (const auto& param : funcDecl->parameters)
        {
            paramTypes.push_back(ResolveType(param.type));
        }

        TypeInfoPtr retType = ResolveType(funcDecl->returnType);
        m_functions[funcDecl->name] = { retType, paramTypes };

        InitState(funcDecl->name, static_cast<int>(funcDecl->parameters.size()), retType);
        BeginScope();

        for (const auto& param : funcDecl->parameters)
        {
            m_current->locals.push_back({param.name, m_current->scopeDepth, true, ResolveType(param.type)});
            m_current->maxLocals = std::max(m_current->maxLocals, static_cast<int>(m_current->locals.size()));
        }

        for (const auto& s : funcDecl->body->statements)
        {
            CompileStmt(s.get());
        }

        if (funcDecl->name == "init")
        {
            Emit(OP_GET_LOCAL);
            Emit(0);
        }
        else
        {
            Emit(OP_CONSTANT);
            Emit(MakeConstant(false));
        }

        Emit(OP_RETURN);
        EndScope();
        FunctionPtr compiledFunc = EndState();

        Emit(OP_CONSTANT);
        Emit(MakeConstant(compiledFunc));

        if (isGlobalScope())
        {
            Emit(OP_DEFINE_GLOBAL);
            Emit(MakeConstant(std::make_shared<std::string>(funcDecl->name)));
            m_globalVariables[funcDecl->name] = { true, TypeInfo::Simple(TypeKind::Func) };
        }
        else
        {
            const auto slot = static_cast<int>(m_current->locals.size());
            Emit(OP_SET_LOCAL);
            Emit(static_cast<uint8_t>(slot));
            Emit(OP_POP);
            m_current->locals.push_back({funcDecl->name, m_current->scopeDepth, true, TypeInfo::Simple(TypeKind::Func)});
            m_current->maxLocals = std::max(m_current->maxLocals, static_cast<int>(m_current->locals.size()));
        }
    }

    void Visit(ReturnStmt* returnStmt) override
    {
        TypeInfoPtr retType = TypeInfo::Simple(TypeKind::Void);
        if (returnStmt->value)
        {
            retType = CompileExpr(returnStmt->value.get());
        }

        if (!m_current->returnType->IsCompatible(retType))
        {
            throw std::runtime_error("Type Error at line " + std::to_string(returnStmt->line) + ": Incompatible return value type.");
        }

        if (returnStmt->value)
        {
            if (m_current->returnType->kind == TypeKind::Void)
            {
                throw std::runtime_error("Compiler Error at line " + std::to_string(returnStmt->line) + ": Cannot return a value from a Void function");
            }
        }
        else
        {
            if (m_current->returnType->kind != TypeKind::Void)
            {
                throw std::runtime_error("Compiler Error at line " + std::to_string(returnStmt->line) + ": Expected a return value");
            }

            if (m_current->function->name == "init")
            {
                Emit(OP_GET_LOCAL);
                Emit(0);
            }
            else
            {
                Emit(OP_CONSTANT);
                Emit(MakeConstant(false));
            }
        }
        Emit(OP_RETURN);
    }

    void Visit(BlockStmt* blockStmt) override
    {
        BeginScope();
        for (const auto& s : blockStmt->statements)
        {
            CompileStmt(s.get());
        }
        EndScope();
    }

    void Visit(IfStmt* ifStmt) override
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

    void Visit(WhileStmt* whileStmt) override
    {
        int loopStart = static_cast<int>(m_current->function->chunk->code.size());
        CompileExpr(whileStmt->condition.get());
        int exitJump = EmitJump(OP_JUMP_IF_FALSE);
        CompileStmt(whileStmt->body.get());
        int loopJump = EmitJump(OP_JUMP);

        if (loopStart > UINT16_MAX)
        {
            throw std::runtime_error("Loop is too large.");
        }
        m_current->function->chunk->code[loopJump] = (loopStart >> 8) & 0xff;
        m_current->function->chunk->code[loopJump + 1] = loopStart & 0xff;
        PatchJump(exitJump);
    }

    void Visit(ClassDeclStmt* classDecl) override
    {
        m_classes[classDecl->name] = ClassInfo{classDecl->name, {}, {}};

        ClassInfo classInfo;
        classInfo.name = classDecl->name;

        for (const auto& member : classDecl->members)
        {
            if (auto* funcDeclaration = dynamic_cast<FuncDeclStmt*>(member.get()))
            {
                std::vector<TypeInfoPtr> paramTypes;
                paramTypes.reserve(funcDeclaration->parameters.size());
                for (const auto& param : funcDeclaration->parameters)
                {
                    paramTypes.push_back(ResolveType(param.type));
                }
                classInfo.methods[funcDeclaration->name] = { ResolveType(funcDeclaration->returnType), paramTypes };
            }
            else if (auto* varDeclaration = dynamic_cast<VarDeclStmt*>(member.get()))
            {
                TypeInfoPtr t = varDeclaration->typeName.empty() ? TypeInfo::Simple(TypeKind::Any) : ResolveType(varDeclaration->typeName);
                classInfo.fields[varDeclaration->name] = { t, varDeclaration->isWeak };
            }
        }
        m_classes[classDecl->name] = classInfo;

        Emit(OP_CLASS);
        Emit(MakeConstant(std::make_shared<std::string>(classDecl->name)));

        if (isGlobalScope())
        {
            Emit(OP_DEFINE_GLOBAL);
            Emit(MakeConstant(std::make_shared<std::string>(classDecl->name)));
            m_globalVariables[classDecl->name] = { true, TypeInfo::Class(classDecl->name) };
        }
        else
        {
            int slot = static_cast<int>(m_current->locals.size());
            Emit(OP_SET_LOCAL);
            Emit(static_cast<uint8_t>(slot));
            Emit(OP_POP);
            m_current->locals.push_back({classDecl->name, m_current->scopeDepth, true, TypeInfo::Class(classDecl->name)});
            m_current->maxLocals = std::max(m_current->maxLocals, static_cast<int>(m_current->locals.size()));
        }

        for (const auto& member : classDecl->members)
        {
            if (auto* funcDeclaration = dynamic_cast<FuncDeclStmt*>(member.get()))
            {
                InitState(
                    funcDeclaration->name,
                    static_cast<int>(funcDeclaration->parameters.size()),
                    ResolveType(funcDeclaration->returnType),
                    TypeInfo::Class(classDecl->name)
                );
                BeginScope();

                m_current->locals[0].name = "self";
                m_current->locals[0].type = TypeInfo::Class(classDecl->name);

                for (const auto& param : funcDeclaration->parameters)
                {
                    m_current->locals.push_back({param.name, m_current->scopeDepth, true, ResolveType(param.type)});
                    m_current->maxLocals = std::max(m_current->maxLocals, static_cast<int>(m_current->locals.size()));
                }

                for (const auto& s : funcDeclaration->body->statements)
                {
                    CompileStmt(s.get());
                }

                if (funcDeclaration->name == "init")
                {
                    Emit(OP_GET_LOCAL);
                    Emit(0);
                }
                else
                {
                    Emit(OP_CONSTANT);
                    Emit(MakeConstant(false));
                }
                Emit(OP_RETURN);
                EndScope();

                FunctionPtr compiledMethod = EndState();

                Emit(OP_CONSTANT);
                Emit(MakeConstant(compiledMethod));

                uint8_t methodNameConst = MakeConstant(std::make_shared<std::string>(funcDeclaration->name));

                if (isGlobalScope())
                {
                    Emit(OP_GET_GLOBAL);
                    Emit(MakeConstant(std::make_shared<std::string>(classDecl->name)));
                }
                else
                {
                    int slot = -1;
                    for (int i = static_cast<int>(m_current->locals.size()) - 1; i >= 0; --i)
                    {
                        if (m_current->locals[i].name == classDecl->name)
                        {
                            slot = i;
                            break;
                        }
                    }
                    Emit(OP_GET_LOCAL);
                    Emit(static_cast<uint8_t>(slot));
                }

                Emit(OP_METHOD);
                Emit(methodNameConst);
            }
            else if (auto* varDeclaration = dynamic_cast<VarDeclStmt*>(member.get()))
            {
                if (isGlobalScope())
                {
                    Emit(OP_GET_GLOBAL);
                    Emit(MakeConstant(std::make_shared<std::string>(classDecl->name)));
                }
                else
                {
                    int slot = -1;
                    for (int i = static_cast<int>(m_current->locals.size()) - 1; i >= 0; --i)
                    {
                        if (m_current->locals[i].name == classDecl->name)
                        {
                            slot = i;
                            break;
                        }
                    }
                    Emit(OP_GET_LOCAL);
                    Emit(static_cast<uint8_t>(slot));
                }

                Emit(OP_FIELD);
                Emit(MakeConstant(std::make_shared<std::string>(varDeclaration->name)));
                Emit(OP_POP);
            }
        }
    }

    void Visit(StructDeclStmt* structDecl) override
    {
        m_classes[structDecl->name] = ClassInfo{structDecl->name, {}, {}};

        ClassInfo structInfo;
        structInfo.name = structDecl->name;

        std::vector<TypeInfoPtr> initParamTypes;
        std::vector<std::string> initParamNames;

        for (const auto& member : structDecl->members)
        {
            TypeInfoPtr t = member->typeName.empty() ? TypeInfo::Simple(TypeKind::Any) : ResolveType(member->typeName);

            if (member->computedBody)
            {
                structInfo.methods[member->name] = { t, {} };
            }
            else
            {
                structInfo.fields[member->name] = { t, member->isWeak };

                if (!member->initExpr) {
                    initParamTypes.push_back(t);
                    initParamNames.push_back(member->name);
                }
            }
        }

        structInfo.methods["init"] = { TypeInfo::Class(structDecl->name), initParamTypes };
        m_classes[structDecl->name] = structInfo;

        Emit(OP_CLASS);
        Emit(MakeConstant(std::make_shared<std::string>(structDecl->name)));

        if (isGlobalScope())
        {
            Emit(OP_DEFINE_GLOBAL);
            Emit(MakeConstant(std::make_shared<std::string>(structDecl->name)));
            m_globalVariables[structDecl->name] = { true, TypeInfo::Class(structDecl->name) };
        }
        else
        {
            int slot = static_cast<int>(m_current->locals.size());
            Emit(OP_SET_LOCAL);
            Emit(static_cast<uint8_t>(slot));
            Emit(OP_POP);
            m_current->locals.push_back({
                structDecl->name,
                m_current->scopeDepth,
                true,
                TypeInfo::Class(structDecl->name)
            });
            m_current->maxLocals = std::max(m_current->maxLocals, static_cast<int>(m_current->locals.size()));
        }

        for (const auto& member : structDecl->members)
        {
            if (!member->computedBody)
            {
                if (isGlobalScope())
                {
                    Emit(OP_GET_GLOBAL); Emit
                    (MakeConstant(std::make_shared<std::string>(structDecl->name)));
                }
                else
                {
                    int slot = -1;
                    for (int i = static_cast<int>(m_current->locals.size()) - 1; i >= 0; --i)
                    {
                        if (m_current->locals[i].name == structDecl->name)
                        {
                            slot = i; break;
                        }
                    }
                    Emit(OP_GET_LOCAL);
                    Emit(static_cast<uint8_t>(slot));
                }
                Emit(OP_FIELD);
                Emit(MakeConstant(std::make_shared<std::string>(member->name)));
                Emit(OP_POP);
            }
        }

        InitState(
            "init",
            static_cast<int>(initParamNames.size()),
            TypeInfo::Class(structDecl->name),
            TypeInfo::Class(structDecl->name)
        );
        BeginScope();
        m_current->locals[0].name = "self";
        m_current->locals[0].type = TypeInfo::Class(structDecl->name);

        for (size_t i = 0; i < initParamNames.size(); ++i)
        {
            m_current->locals.push_back({
                initParamNames[i],
                m_current->scopeDepth,
                true,
                initParamTypes[i]
            });
            m_current->maxLocals = std::max(m_current->maxLocals, static_cast<int>(m_current->locals.size()));

            Emit(OP_GET_LOCAL);
            Emit(0);
            Emit(OP_GET_LOCAL);
            Emit(static_cast<uint8_t>(i + 1));
            Emit(OP_SET_PROPERTY);
            Emit(MakeConstant(std::make_shared<std::string>(initParamNames[i])));
            Emit(OP_POP);
        }

        Emit(OP_GET_LOCAL);
        Emit(0);
        Emit(OP_RETURN);
        EndScope();

        FunctionPtr compiledInit = EndState();
        Emit(OP_CONSTANT); Emit(MakeConstant(compiledInit));

        if (isGlobalScope())
        {
            Emit(OP_GET_GLOBAL);
            Emit(MakeConstant(std::make_shared<std::string>(structDecl->name)));
        }
        else
        {
            int slot = -1;
            for (int i = static_cast<int>(m_current->locals.size()) - 1; i >= 0; --i)
            {
                if (m_current->locals[i].name == structDecl->name)
                {
                    slot = i; break;
                }
            }
            Emit(OP_GET_LOCAL);
            Emit(static_cast<uint8_t>(slot));
        }
        Emit(OP_METHOD);
        Emit(MakeConstant(std::make_shared<std::string>("init")));

        for (const auto& member : structDecl->members)
        {
            if (member->computedBody)
            {
                TypeInfoPtr retType = member->typeName.empty() ? TypeInfo::Simple(TypeKind::Any) : ResolveType(member->typeName);

                InitState(member->name, 0, retType, TypeInfo::Class(structDecl->name));
                BeginScope();

                m_current->locals[0].name = "self";
                m_current->locals[0].type = TypeInfo::Class(structDecl->name);

                for (const auto& s : member->computedBody->statements)
                {
                    CompileStmt(s.get());
                }

                Emit(OP_CONSTANT);
                Emit(MakeConstant(false));
                Emit(OP_RETURN);
                EndScope();

                FunctionPtr compiledGetter = EndState();

                Emit(OP_CONSTANT);
                Emit(MakeConstant(compiledGetter));

                if (isGlobalScope())
                {
                    Emit(OP_GET_GLOBAL);
                    Emit(MakeConstant(std::make_shared<std::string>(structDecl->name)));
                }
                else
                {
                    int slot = -1;
                    for (int i = static_cast<int>(m_current->locals.size()) - 1; i >= 0; --i)
                    {
                        if (m_current->locals[i].name == structDecl->name)
                        {
                            slot = i; break;
                        }
                    }
                    Emit(OP_GET_LOCAL);
                    Emit(static_cast<uint8_t>(slot));
                }

                Emit(OP_METHOD);
                Emit(MakeConstant(std::make_shared<std::string>(member->name)));
            }
        }
    }

    void Visit(AssignExpr* assign) override
    {
        if (auto* ident = dynamic_cast<IdentifierExpr*>(assign->target.get()))
        {
            int slot = -1;
            TypeInfoPtr varType = TypeInfo::Simple(TypeKind::Any);

            for (int i = static_cast<int>(m_current->locals.size()) - 1; i >= 0; --i)
            {
                if (m_current->locals[i].name == ident->name)
                {
                    if (m_current->locals[i].isConst)
                    {
                        throw std::runtime_error("Compiler Error at line " + std::to_string(ident->line) + ": Cannot reassign to constant '" + ident->name + "'");
                    }
                    slot = i;
                    varType = m_current->locals[i].type;
                    break;
                }
            }

            if (slot == -1)
            {
                if (m_globalVariables.contains(ident->name))
                {
                    if (m_globalVariables[ident->name].isConst)
                    {
                        throw std::runtime_error("Compiler Error at line " + std::to_string(ident->line) + ": Cannot reassign to global constant '" + ident->name + "'");
                    }
                    varType = m_globalVariables[ident->name].type;
                }
                else
                {
                    throw std::runtime_error("Compiler Error at line " + std::to_string(ident->line) + ": Variable '" + ident->name + "' is not declared");
                }
            }

            TypeInfoPtr valType = CompileExpr(assign->value.get());

            if (!varType->IsCompatible(valType))
            {
                throw std::runtime_error("Type Error at line " + std::to_string(assign->line) + ": Cannot assign incompatible value to variable '" + ident->name + "'");
            }

            if (slot != -1)
            {
                Emit(OP_SET_LOCAL);
                Emit(static_cast<uint8_t>(slot));
            }
            else
            {
                Emit(OP_SET_GLOBAL);
                Emit(MakeConstant(std::make_shared<std::string>(ident->name)));
            }
            m_lastExprType = varType;
        }
        else if (auto* idxExpr = dynamic_cast<IndexExpr*>(assign->target.get()))
        {
            TypeInfoPtr arrType = CompileExpr(idxExpr->array.get());
            TypeInfoPtr idxType = CompileExpr(idxExpr->index.get());

            if (idxType->kind != TypeKind::Int)
            {
                throw std::runtime_error("Type Error at line " + std::to_string(idxExpr->line) + ": Array index must be an Int");
            }

            TypeInfoPtr valType = CompileExpr(assign->value.get());
            Emit(OP_SET_INDEX);
            m_lastExprType = valType;
        }
        else 
        {
             throw std::runtime_error("Parse Error: Invalid assignment target");
        }
    }

    void Visit(NumberExpr* num) override
    {
        Emit(OP_CONSTANT);
        if (num->isDouble)
        {
            Emit(MakeConstant(num->value));
            m_lastExprType = TypeInfo::Simple(TypeKind::Double);
        }
        else
        {
            Emit(MakeConstant(static_cast<int64_t>(num->value)));
            m_lastExprType = TypeInfo::Simple(TypeKind::Int);
        }
    }

    void Visit(BoolExpr* b) override
    {
        Emit(OP_CONSTANT);
        Emit(MakeConstant(b->value));
        m_lastExprType = TypeInfo::Simple(TypeKind::Bool);
    }

    void Visit(StringExpr* str) override
    {
        Emit(OP_CONSTANT);
        Emit(MakeConstant(std::make_shared<std::string>(str->value)));
        m_lastExprType = TypeInfo::Simple(TypeKind::String);
    }

    void Visit(BinaryExpr* bin) override
    {
        TypeInfoPtr lType = CompileExpr(bin->left.get());
        TypeInfoPtr rType = CompileExpr(bin->right.get());

        if (bin->op == "+" || bin->op == "-" || bin->op == "*" || bin->op == "/" || bin->op == "%")
        {
            if (lType->kind == TypeKind::String && bin->op == "+")
            {
                Emit(OP_ADD);
                m_lastExprType = TypeInfo::Simple(TypeKind::String);
                return;
            }

            if ((lType->kind != TypeKind::Int && lType->kind != TypeKind::Double && lType->kind != TypeKind::Any) ||
                (rType->kind != TypeKind::Int && rType->kind != TypeKind::Double && rType->kind != TypeKind::Any))
            {
                throw std::runtime_error("Type Error at line " + std::to_string(bin->line) + ": Arithmetic operators are not supported for these types");
            }

            if (bin->op == "+") Emit(OP_ADD);
            else if (bin->op == "-") Emit(OP_SUB);
            else if (bin->op == "*") Emit(OP_MUL);
            else if (bin->op == "/") Emit(OP_DIV);
            else if (bin->op == "%") Emit(OP_MOD);

            if (lType->kind == TypeKind::Double || rType->kind == TypeKind::Double)
                m_lastExprType = TypeInfo::Simple(TypeKind::Double);
            else
                m_lastExprType = TypeInfo::Simple(TypeKind::Int);
            return;
        }

        if (bin->op == "==" || bin->op == "!=" || bin->op == "<" || bin->op == ">" || bin->op == "<=" || bin->op == ">=")
        {
            if (!lType->IsCompatible(rType))
            {
                throw std::runtime_error("Type Error at line " + std::to_string(bin->line) + ": Cannot compare different types");
            }

            if (bin->op == "==") Emit(OP_EQUAL);
            else if (bin->op == "<") Emit(OP_LESS);
            else if (bin->op == ">") Emit(OP_GREATER);
            else if (bin->op == "<=") { Emit(OP_GREATER); Emit(OP_NOT); }
            else if (bin->op == ">=") { Emit(OP_LESS); Emit(OP_NOT); }
            else if (bin->op == "!=") { Emit(OP_EQUAL); Emit(OP_NOT); }

            m_lastExprType = TypeInfo::Simple(TypeKind::Bool);
            return;
        }

        if (bin->op == "&&" || bin->op == "||")
        {
            if (lType->kind != TypeKind::Bool || rType->kind != TypeKind::Bool)
            {
                throw std::runtime_error("Type Error at line " + std::to_string(bin->line) + ": Logical operators '&&' and '||' require Bool operands");
            }

            if (bin->op == "&&") Emit(OP_AND);
            else if (bin->op == "||") Emit(OP_OR);
            
            m_lastExprType = TypeInfo::Simple(TypeKind::Bool);
            return;
        }
    }

    void Visit(IdentifierExpr* ident) override
    {
        if (ident->name == "self")
        {
            if (!m_current->currentClass)
            {
                throw std::runtime_error("Compiler Error at line " + std::to_string(ident->line) + ": Cannot use 'self' outside of a class method");
            }
        }

        int slot = -1;
        TypeInfoPtr typeOut = TypeInfo::Simple(TypeKind::Any);

        for (int i = static_cast<int>(m_current->locals.size()) - 1; i >= 0; --i)
        {
            if (m_current->locals[i].name == ident->name)
            {
                slot = i;
                typeOut = m_current->locals[i].type;
                break;
            }
        }

        if (slot != -1)
        {
            Emit(OP_GET_LOCAL);
            Emit(static_cast<uint8_t>(slot));
            m_lastExprType = typeOut;
        }
        else
        {
            if (m_globalVariables.contains(ident->name))
            {
                typeOut = m_globalVariables[ident->name].type;
            }
            else if (m_classes.contains(ident->name))
            {
                typeOut = TypeInfo::Class(ident->name);
            }
            else if (m_functions.contains(ident->name))
            {
                typeOut = TypeInfo::Simple(TypeKind::Func);
            }
            else
            {
                throw std::runtime_error("Compiler Error at line " + std::to_string(ident->line) + ": Undefined identifier '" + ident->name + "'");
            }

            Emit(OP_GET_GLOBAL);
            Emit(MakeConstant(std::make_shared<std::string>(ident->name)));
            m_lastExprType = typeOut;
        }
    }

    void Visit(CallExpr* call) override
    {
        bool isConstructor = false;
        bool isFunc = false;
        std::string funcName;

        if (auto* identCallee = dynamic_cast<IdentifierExpr*>(call->callee.get()))
        {
            funcName = identCallee->name;
            if (m_classes.contains(funcName)) isConstructor = true;
            else if (m_functions.contains(funcName)) isFunc = true;
        }
        else if (auto* getCallee = dynamic_cast<GetExpr*>(call->callee.get()))
        {
            TypeInfoPtr objType = CompileExpr(getCallee->object.get());
            if (objType->kind == TypeKind::Class && m_classes.contains(objType->name))
            {
                const auto& klass = m_classes[objType->name];
                if (klass.methods.contains(getCallee->propertyName))
                {
                    const auto& method = klass.methods.at(getCallee->propertyName);
                    Emit(OP_GET_PROPERTY);
                    Emit(MakeConstant(std::make_shared<std::string>(getCallee->propertyName)));

                    if (call->arguments.size() != method.paramTypes.size())
                    {
                        throw std::runtime_error("Type Error at line " + std::to_string(call->line) + ": Method arguments count mismatch");
                    }

                    for (size_t i = 0; i < call->arguments.size(); ++i)
                    {
                        TypeInfoPtr argType = CompileExpr(call->arguments[i].get());
                        if (!method.paramTypes[i]->IsCompatible(argType))
                        {
                            throw std::runtime_error("Type Error: Argument type mismatch");
                        }
                    }

                    Emit(OP_CALL);
                    Emit(static_cast<uint8_t>(call->arguments.size()));
                    m_lastExprType = method.returnType;
                    return;
                }
            }
        }

        CompileExpr(call->callee.get());

        if (isConstructor)
        {
            const auto& klass = m_classes[funcName];
            if (klass.methods.contains("init"))
            {
                const auto& initMethod = klass.methods.at("init");
                if (call->arguments.size() != initMethod.paramTypes.size())
                {
                    throw std::runtime_error("Type Error at line " + std::to_string(call->line) + ": Constructor arguments count mismatch");
                }

                for (size_t i = 0; i < call->arguments.size(); ++i)
                {
                    TypeInfoPtr argType = CompileExpr(call->arguments[i].get());
                    if (!initMethod.paramTypes[i]->IsCompatible(argType))
                    {
                        throw std::runtime_error("Type Error: Constructor argument type mismatch");
                    }
                }
            }
            else if (!call->arguments.empty())
            {
                throw std::runtime_error("Type Error at line " + std::to_string(call->line) + ": Class '" + funcName + "' has no init constructor, but arguments were provided.");
            }

            Emit(OP_CALL);
            Emit(static_cast<uint8_t>(call->arguments.size()));
            m_lastExprType = TypeInfo::Class(funcName);
            return;
        }

        if (isFunc)
        {
            const auto& func = m_functions[funcName];
            if (call->arguments.size() != func.paramTypes.size())
            {
                throw std::runtime_error("Type Error at line " + std::to_string(call->line) + ": Function arguments count mismatch");
            }

            for (size_t i = 0; i < call->arguments.size(); ++i)
            {
                TypeInfoPtr argType = CompileExpr(call->arguments[i].get());
                if (!func.paramTypes[i]->IsCompatible(argType))
                {
                    throw std::runtime_error("Type Error: Function argument type mismatch");
                }
            }

            Emit(OP_CALL);
            Emit(static_cast<uint8_t>(call->arguments.size()));
            m_lastExprType = func.returnType;
            return;
        }

        for (const auto& arg : call->arguments)
        {
            CompileExpr(arg.get());
        }
        Emit(OP_CALL);
        Emit(static_cast<uint8_t>(call->arguments.size()));
        m_lastExprType = TypeInfo::Simple(TypeKind::Any);
    }

    void Visit(ArrayExpr* arr) override
    {
        Emit(OP_BUILD_ARRAY);
        for (const auto& el : arr->elements)
        {
            CompileExpr(el.get());
            Emit(OP_ARRAY_PUSH);
        }
        m_lastExprType = TypeInfo::Simple(TypeKind::Array);
    }

    void Visit(IndexExpr* idx) override
    {
        TypeInfoPtr arrType = CompileExpr(idx->array.get());
        TypeInfoPtr indexType = CompileExpr(idx->index.get());

        if (indexType->kind != TypeKind::Int)
        {
            throw std::runtime_error("Type Error at line " + std::to_string(idx->line) + ": Array index must be an Int");
        }
        
        Emit(OP_GET_INDEX);

        if (arrType->kind == TypeKind::Array && arrType->elementType)
        {
            m_lastExprType = arrType->elementType;
        }
        else 
        {
            m_lastExprType = TypeInfo::Simple(TypeKind::Any);
        }
    }

    void Visit(GetExpr* getExpr) override
    {
        TypeInfoPtr objType = CompileExpr(getExpr->object.get());

        if (objType->kind == TypeKind::Class && m_classes.contains(objType->name))
        {
            const auto& klass = m_classes[objType->name];

            if (klass.fields.contains(getExpr->propertyName))
            {
                Emit(OP_GET_PROPERTY);
                Emit(MakeConstant(std::make_shared<std::string>(getExpr->propertyName)));
                m_lastExprType = klass.fields.at(getExpr->propertyName).type;
                return;
            }

            if (klass.methods.contains(getExpr->propertyName))
            {
                Emit(OP_GET_PROPERTY);
                Emit(MakeConstant(std::make_shared<std::string>(getExpr->propertyName)));

                const auto& methodInfo = klass.methods.at(getExpr->propertyName);

                if (methodInfo.paramTypes.empty())
                {
                    Emit(OP_CALL);
                    Emit(0);
                    m_lastExprType = methodInfo.returnType;
                }
                else
                {
                    m_lastExprType = TypeInfo::Simple(TypeKind::Func);
                }
                return;
            }
            throw std::runtime_error("Compiler Error at line " + std::to_string(getExpr->line) + ": Class/Struct '" + objType->name + "' has no property or method '" + getExpr->propertyName + "'");
        }

        Emit(OP_GET_PROPERTY);
        Emit(MakeConstant(std::make_shared<std::string>(getExpr->propertyName)));
        m_lastExprType = TypeInfo::Simple(TypeKind::Any);
    }

    void Visit(SetExpr* setExpr) override
    {
        TypeInfoPtr objType = CompileExpr(setExpr->object.get());
        TypeInfoPtr valType = CompileExpr(setExpr->value.get());

        if (objType->kind == TypeKind::Class && m_classes.contains(objType->name))
        {
            const auto& klass = m_classes[objType->name];
            if (!klass.fields.contains(setExpr->propertyName))
            {
                throw std::runtime_error("Compiler Error at line " + std::to_string(setExpr->line) + ": Class '" + objType->name + "' has no property '" + setExpr->propertyName + "'");
            }

            TypeInfoPtr targetType = klass.fields.at(setExpr->propertyName).type;
            bool isWeak = klass.fields.at(setExpr->propertyName).isWeak;

            if (!targetType->IsCompatible(valType))
            {
                throw std::runtime_error("Type Error at line " + std::to_string(setExpr->line) + ": Cannot assign incompatible value to property '" + setExpr->propertyName + "'");
            }

            if (isWeak)
            {
                Emit(OP_SET_PROPERTY_WEAK);
            }
            else
            {
                Emit(OP_SET_PROPERTY);
            }

            Emit(MakeConstant(std::make_shared<std::string>(setExpr->propertyName)));
            m_lastExprType = targetType;
            return;
        }

        Emit(OP_SET_PROPERTY);
        Emit(MakeConstant(std::make_shared<std::string>(setExpr->propertyName)));
        m_lastExprType = TypeInfo::Simple(TypeKind::Any);
    }
};