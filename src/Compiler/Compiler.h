#pragma once
#include <memory>
#include <vector>
#include <unordered_map>
#include <stdexcept>
#include <unordered_set>
#include <filesystem>

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

        m_functions["Int"] = {
            TypeInfo::Simple(TypeKind::Int), { TypeInfo::Simple(TypeKind::Any) }
        };
        m_functions["Double"] = {
            TypeInfo::Simple(TypeKind::Double), { TypeInfo::Simple(TypeKind::Any) }
        };

        m_functions["UIInitWindow"] = {
            TypeInfo::Simple(TypeKind::Void),
            { TypeInfo::Simple(TypeKind::Int), TypeInfo::Simple(TypeKind::Int), TypeInfo::Simple(TypeKind::String) }
        };

        m_functions["UIShouldClose"] = {
            TypeInfo::Simple(TypeKind::Bool),
            {}
        };

        m_functions["UIBeginFrame"] = {
            TypeInfo::Simple(TypeKind::Void),
            { TypeInfo::Simple(TypeKind::Int), TypeInfo::Simple(TypeKind::Int) }
        };

        m_functions["UIDrawRect"] = {
            TypeInfo::Simple(TypeKind::Void),
            {
                TypeInfo::Simple(TypeKind::Double), TypeInfo::Simple(TypeKind::Double),
                TypeInfo::Simple(TypeKind::Double), TypeInfo::Simple(TypeKind::Double),
                TypeInfo::Simple(TypeKind::Double), TypeInfo::Simple(TypeKind::Double),
                TypeInfo::Simple(TypeKind::Double), TypeInfo::Simple(TypeKind::Double)
            }
        };

        m_functions["UIEndFrame"] = {
            TypeInfo::Simple(TypeKind::Void),
            {}
        };

        m_functions["UIDrawText"] = {
            TypeInfo::Simple(TypeKind::Void),
            {
                TypeInfo::Simple(TypeKind::Double), TypeInfo::Simple(TypeKind::Double),
                TypeInfo::Simple(TypeKind::String),
                TypeInfo::Simple(TypeKind::Double),
                TypeInfo::Simple(TypeKind::Double), TypeInfo::Simple(TypeKind::Double),
                TypeInfo::Simple(TypeKind::Double)
            }
        };

        m_functions["UIMeasureTextWidth"] = {
            TypeInfo::Simple(TypeKind::Double),
            { TypeInfo::Simple(TypeKind::String), TypeInfo::Simple(TypeKind::Double) }
        };

        m_functions["UIGetMouseX"] = { TypeInfo::Simple(TypeKind::Double), {} };
        m_functions["UIGetMouseY"] = { TypeInfo::Simple(TypeKind::Double), {} };
        m_functions["UIIsMouseDown"] = { TypeInfo::Simple(TypeKind::Bool), {} };
        m_functions["UIIsMouseJustPressed"] = { TypeInfo::Simple(TypeKind::Bool), {} };

        m_functions["UIGetPendingText"] = { TypeInfo::Simple(TypeKind::String), {} };
        m_functions["UIIsBackspacePressed"] = { TypeInfo::Simple(TypeKind::Bool), {} };
        m_functions["UIIsEnterPressed"] = { TypeInfo::Simple(TypeKind::Bool), {} };

        m_functions["UISetFocused"] = { TypeInfo::Simple(TypeKind::Bool), { TypeInfo::Simple(TypeKind::String) } };
        m_functions["UIIsFocused"] = { TypeInfo::Simple(TypeKind::Bool), { TypeInfo::Simple(TypeKind::String) } };
        m_functions["UIClearFocus"] = { TypeInfo::Simple(TypeKind::Bool), {} };
        m_functions["UIGetVersion"] = { TypeInfo::Simple(TypeKind::Any), { TypeInfo::Simple(TypeKind::Any) } };
        m_functions["UIIsDirty"]           = { TypeInfo::Simple(TypeKind::Bool), {} };
        m_functions["UIGetScrollDelta"]    = { TypeInfo::Simple(TypeKind::Double), {} };
        m_functions["UIBeginClip"]         = { TypeInfo::Simple(TypeKind::Void), {
            TypeInfo::Simple(TypeKind::Double), TypeInfo::Simple(TypeKind::Double),
            TypeInfo::Simple(TypeKind::Double), TypeInfo::Simple(TypeKind::Double)
        } };
        m_functions["UIEndClip"]           = { TypeInfo::Simple(TypeKind::Void), {} };
        m_functions["UIDrawRoundedRect"] = { TypeInfo::Simple(TypeKind::Any), {
            TypeInfo::Simple(TypeKind::Double), TypeInfo::Simple(TypeKind::Double),
            TypeInfo::Simple(TypeKind::Double), TypeInfo::Simple(TypeKind::Double),
            TypeInfo::Simple(TypeKind::Double),
            TypeInfo::Simple(TypeKind::Double), TypeInfo::Simple(TypeKind::Double),
            TypeInfo::Simple(TypeKind::Double), TypeInfo::Simple(TypeKind::Double)
        } };

        m_functions["ResultOk"]    = { TypeInfo::Simple(TypeKind::Any), { TypeInfo::Simple(TypeKind::Any) } };
        m_functions["ResultError"] = { TypeInfo::Simple(TypeKind::Any), { TypeInfo::Simple(TypeKind::String) } };

        m_functions["JsonParse"]     = { TypeInfo::Simple(TypeKind::Any), { TypeInfo::Simple(TypeKind::String) } };
        m_functions["JsonStringify"] = { TypeInfo::Simple(TypeKind::String), { TypeInfo::Simple(TypeKind::Any) } };

        m_functions["HttpGet"]    = { TypeInfo::Simple(TypeKind::Any), { TypeInfo::Simple(TypeKind::String) } };
        m_functions["HttpPost"]   = { TypeInfo::Simple(TypeKind::Any), { TypeInfo::Simple(TypeKind::String), TypeInfo::Simple(TypeKind::String) } };
        m_functions["HttpPut"]    = { TypeInfo::Simple(TypeKind::Any), { TypeInfo::Simple(TypeKind::String), TypeInfo::Simple(TypeKind::String) } };
        m_functions["HttpDelete"] = { TypeInfo::Simple(TypeKind::Any), { TypeInfo::Simple(TypeKind::String) } };

        m_functions["Async"] = { TypeInfo::Simple(TypeKind::Any), { TypeInfo::Simple(TypeKind::Any) } };

        m_functions["DateNow"]       = { TypeInfo::Simple(TypeKind::Double), {} };
        m_functions["DateTimestamp"] = { TypeInfo::Simple(TypeKind::Double), {} };
        m_functions["DateFormat"]    = { TypeInfo::Simple(TypeKind::String), { TypeInfo::Simple(TypeKind::Any), TypeInfo::Simple(TypeKind::String) } };
        m_functions["DateYear"]      = { TypeInfo::Simple(TypeKind::Int), {} };
        m_functions["DateMonth"]     = { TypeInfo::Simple(TypeKind::Int), {} };
        m_functions["DateDay"]       = { TypeInfo::Simple(TypeKind::Int), {} };
        m_functions["DateHour"]      = { TypeInfo::Simple(TypeKind::Int), {} };
        m_functions["DateMinute"]    = { TypeInfo::Simple(TypeKind::Int), {} };
        m_functions["DateSecond"]    = { TypeInfo::Simple(TypeKind::Int), {} };

        ClassInfo dateClassInfo;
        dateClassInfo.name = "Date";
        dateClassInfo.staticFieldNames.insert("now");
        dateClassInfo.staticFieldNames.insert("timestamp");
        dateClassInfo.staticFieldNames.insert("format");
        dateClassInfo.staticFieldNames.insert("year");
        dateClassInfo.staticFieldNames.insert("month");
        dateClassInfo.staticFieldNames.insert("day");
        dateClassInfo.staticFieldNames.insert("hour");
        dateClassInfo.staticFieldNames.insert("minute");
        dateClassInfo.staticFieldNames.insert("second");
        m_classes["Date"] = dateClassInfo;
        m_globalVariables["Date"] = { true, TypeInfo::Class("Date") };
    }
    FunctionPtr Compile(const std::vector<std::unique_ptr<Stmt>>& ast,
                        const std::string& sourceFile = "")
    {
        if (!sourceFile.empty())
        {
            namespace fs = std::filesystem;
            m_importDirStack.push_back(
                fs::weakly_canonical(fs::path(sourceFile)).parent_path().string()
            );
        }
        InitState("main", 0, TypeInfo::Simple(TypeKind::Void));

        for (const auto& stmt : ast)
        {
            CompileStmt(stmt.get());
        }

        Emit(OP_CONSTANT);
        Emit16(MakeConstant(0));
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
        bool isCaptured = false;
    };

    struct Upvalue
    {
        uint8_t index;
        bool isLocal;
        TypeInfoPtr type;
    };

    struct CompilerState
    {
        FunctionPtr function;
        std::vector<Local> locals;
        std::vector<Upvalue> upvalues;
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
        AccessLevel accessLevel = AccessLevel::Internal;
    };

    struct FieldInfo
    {
        TypeInfoPtr type;
        bool isWeak{};
        AccessLevel accessLevel = AccessLevel::Internal;
    };

    struct ClassInfo
    {
        std::string name;
        bool isStruct = false;
        std::vector<std::string> implementedInterfaces;
        std::unordered_map<std::string, FieldInfo> fields;
        std::unordered_map<std::string, FuncInfo> methods;
        std::unordered_set<std::string> staticFieldNames;
    };

    struct InterfaceInfo
    {
        std::string name;
        std::unordered_map<std::string, FieldInfo> properties;
        std::unordered_map<std::string, FuncInfo> methods;
    };

    CompilerState* m_current = nullptr;
    std::unordered_map<std::string, GlobalVar> m_globalVariables;
    std::unordered_map<std::string, ClassInfo> m_classes;
    std::unordered_map<std::string, FuncInfo> m_functions;
    std::unordered_map<std::string, InterfaceInfo> m_interfaces;
    std::unordered_set<std::string> m_importedModules;
    std::unordered_set<std::string> m_importStack;
    std::vector<std::string> m_importDirStack;
    
    TypeInfoPtr m_lastExprType = TypeInfo::Simple(TypeKind::Any);

    TypeInfoPtr ResolveType(const std::string& name)
    {
        if (name.empty() || name == "Any") return TypeInfo::Simple(TypeKind::Any);

        if (name.back() == '?')
        {
            std::string inner = name.substr(0, name.size() - 1);
            return TypeInfo::Optional(ResolveType(inner));
        }

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
        if (name.starts_with("Array<") && name.back() == '>')
        {
            std::string inner = name.substr(6, name.size() - 7);
            auto type = TypeInfo::Simple(TypeKind::Array);
            type->elementType = ResolveType(inner);
            return type;
        }
        if (m_classes.contains(name))
        {
            return TypeInfo::Class(name);
        }
        std::string searchName = name;
        if (name.starts_with("any "))
        {
            searchName = name.substr(4);
        }

        if (m_interfaces.contains(searchName))
        {
            auto interfaceType = TypeInfo::Simple(TypeKind::Interface);
            interfaceType->name = searchName;
            return interfaceType;
        }
        if (name.front() == '(')
        {
            int balance = 0;
            size_t closeParen = 0;
            for (size_t i = 0; i < name.size(); ++i)
            {
                if (name[i] == '(')
                {
                    balance++;
                }
                else if (name[i] == ')')
                {
                    balance--;
                    if (balance == 0)
                    {
                        closeParen = i;
                        break;
                    }
                }
            }

            std::string paramsStr = name.substr(1, closeParen - 1);
            std::vector<TypeInfoPtr> paramTypes;

            if (!paramsStr.empty())
            {
                size_t start = 0, nest = 0;
                for (size_t i = 0; i < paramsStr.size(); ++i)
                {
                    if (paramsStr[i] == '(' || paramsStr[i] == '[')
                    {
                        nest++;
                    }
                    else if (paramsStr[i] == ')' || paramsStr[i] == ']')
                    {
                        nest--;
                    }
                    else if (paramsStr[i] == ',' && nest == 0)
                    {
                        std::string p = paramsStr.substr(start, i - start);
                        p.erase(0, p.find_first_not_of(' '));
                        p.erase(p.find_last_not_of(' ') + 1);
                        paramTypes.push_back(ResolveType(p));
                        start = i + 1;
                    }
                }
                std::string p = paramsStr.substr(start);
                p.erase(0, p.find_first_not_of(' '));
                p.erase(p.find_last_not_of(' ') + 1);
                if (!p.empty())
                {
                    paramTypes.push_back(ResolveType(p));
                }
            }

            size_t colonPos = name.find(':', closeParen);
            std::string retStr = "Void";
            if (colonPos != std::string::npos)
            {
                retStr = name.substr(colonPos + 1);
                retStr.erase(0, retStr.find_first_not_of(' '));
            }

            auto funcType = TypeInfo::Simple(TypeKind::Func);
            funcType->paramTypes = paramTypes;
            funcType->returnType = ResolveType(retStr);
            return funcType;
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

    void Emit16(uint16_t idx)
    {
        m_current->function->chunk->code.push_back(static_cast<uint8_t>((idx >> 8) & 0xFF));
        m_current->function->chunk->code.push_back(static_cast<uint8_t>(idx & 0xFF));
    }

    uint16_t MakeConstant(const Value& value)
    {
        auto& constants = m_current->function->chunk->constants;
        if (std::holds_alternative<StringPtr>(value))
        {
            const std::string& s = *std::get<StringPtr>(value);
            for (size_t i = 0; i < constants.size(); ++i)
            {
                if (std::holds_alternative<StringPtr>(constants[i]) &&
                    *std::get<StringPtr>(constants[i]) == s)
                {
                    return static_cast<uint16_t>(i);
                }
            }
        }
        else if (std::holds_alternative<double>(value))
        {
            double d = std::get<double>(value);
            for (size_t i = 0; i < constants.size(); ++i)
            {
                if (std::holds_alternative<double>(constants[i]) && std::get<double>(constants[i]) == d)
                {
                    return static_cast<uint8_t>(i);
                }
            }
        }
        else if (std::holds_alternative<bool>(value))
        {
            bool b = std::get<bool>(value);
            for (size_t i = 0; i < constants.size(); ++i)
            {
                if (std::holds_alternative<bool>(constants[i]) && std::get<bool>(constants[i]) == b)
                {
                    return static_cast<uint8_t>(i);
                }
            }
        }
        else if (std::holds_alternative<int64_t>(value))
        {
            int64_t n = std::get<int64_t>(value);
            for (size_t i = 0; i < constants.size(); ++i)
            {
                if (std::holds_alternative<int64_t>(constants[i]) && std::get<int64_t>(constants[i]) == n)
                {
                    return static_cast<uint8_t>(i);
                }
            }
        }
        constants.push_back(value);
        return static_cast<uint16_t>(constants.size() - 1);
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
            Emit16(MakeConstant(Null{}));
            initType = TypeInfo::Simple(TypeKind::Null);
        }

        TypeInfoPtr finalType = initType;
        if (!varDecl->typeName.empty())
        {
            finalType = ResolveType(varDecl->typeName);

            if (!AreTypesCompatible(initType, finalType))
            {
                throw std::runtime_error("Type Error at line " + std::to_string(varDecl->line) + ": Cannot initialize variable '" + varDecl->name +
                    "' with incompatible type.");
            }
        }

        if (isGlobalScope())
        {
            Emit(OP_DEFINE_GLOBAL);
            Emit16(MakeConstant(std::make_shared<std::string>(varDecl->name)));
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
            Emit16(MakeConstant(false));
        }

        Emit(OP_RETURN);
        EndScope();
        FunctionPtr compiledFunc = EndState();

        Emit(OP_CONSTANT);
        Emit16(MakeConstant(compiledFunc));

        if (isGlobalScope())
        {
            Emit(OP_DEFINE_GLOBAL);
            Emit16(MakeConstant(std::make_shared<std::string>(funcDecl->name)));
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

        if (!AreTypesCompatible(retType, m_current->returnType))
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
                Emit16(MakeConstant(false));
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
        classInfo.implementedInterfaces = classDecl->implementedInterfaces;

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
                classInfo.methods[funcDeclaration->name] = {
                    ResolveType(funcDeclaration->returnType),
                    paramTypes,
                    funcDeclaration->accessLevel
                };
            }
            else if (auto* varDeclaration = dynamic_cast<VarDeclStmt*>(member.get()))
            {
                TypeInfoPtr type = varDeclaration->typeName.empty()
                    ? TypeInfo::Simple(TypeKind::Any)
                    : ResolveType(varDeclaration->typeName);

                if (varDeclaration->computedBody)
                {
                    classInfo.methods[varDeclaration->name] = {
                        type,
                        {},
                        varDeclaration->accessLevel
                    };
                }
                else
                {
                    classInfo.fields[varDeclaration->name] = {
                        type,
                        varDeclaration->isWeak,
                        varDeclaration->accessLevel
                    };
                }
            }
        }
        m_classes[classDecl->name] = classInfo;

        ValidateInterfaces(classDecl->name, classDecl->implementedInterfaces, classInfo);

        Emit(OP_CLASS);
        Emit16(MakeConstant(std::make_shared<std::string>(classDecl->name)));

        if (isGlobalScope())
        {
            Emit(OP_DEFINE_GLOBAL);
            Emit16(MakeConstant(std::make_shared<std::string>(classDecl->name)));
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
                    Emit16(MakeConstant(false));
                }
                Emit(OP_RETURN);
                EndScope();

                FunctionPtr compiledMethod = EndState();

                Emit(OP_CONSTANT);
                Emit16(MakeConstant(compiledMethod));

                uint16_t methodNameConst = MakeConstant(std::make_shared<std::string>(funcDeclaration->name));

                if (isGlobalScope())
                {
                    Emit(OP_GET_GLOBAL);
                    Emit16(MakeConstant(std::make_shared<std::string>(classDecl->name)));
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
                Emit16(methodNameConst);
            }
            else if (auto* varDeclaration = dynamic_cast<VarDeclStmt*>(member.get()))
            {
                if (!varDeclaration->computedBody)
                {
                    if (isGlobalScope())
                    {
                        Emit(OP_GET_GLOBAL);
                        Emit16(MakeConstant(std::make_shared<std::string>(classDecl->name)));
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

                    Emit(varDeclaration->isTrackable ? OP_FIELD_TRACKABLE : OP_FIELD);
                    Emit16(MakeConstant(std::make_shared<std::string>(varDeclaration->name)));
                    Emit(OP_POP);
                }
            }
        }

        for (const auto& member : classDecl->members)
        {
            if (auto* varDeclaration = dynamic_cast<VarDeclStmt*>(member.get()))
            {
                if (varDeclaration->computedBody)
                {
                    TypeInfoPtr retType = varDeclaration->typeName.empty() ? TypeInfo::Simple(TypeKind::Any) : ResolveType(varDeclaration->typeName);

                    InitState(varDeclaration->name, 0, retType, TypeInfo::Class(classDecl->name));
                    BeginScope();

                    m_current->locals[0].name = "self";
                    m_current->locals[0].type = TypeInfo::Class(classDecl->name);

                    for (const auto& s : varDeclaration->computedBody->statements)
                    {
                        CompileStmt(s.get());
                    }

                    Emit(OP_CONSTANT);
                    Emit16(MakeConstant(false));
                    Emit(OP_RETURN);
                    EndScope();

                    FunctionPtr compiledGetter = EndState();

                    Emit(OP_CONSTANT);
                    Emit16(MakeConstant(compiledGetter));

                    if (isGlobalScope())
                    {
                        Emit(OP_GET_GLOBAL);
                        Emit16(MakeConstant(std::make_shared<std::string>(classDecl->name)));
                    }
                    else
                    {
                        int slot = -1;
                        for (int i = static_cast<int>(m_current->locals.size()) - 1; i >= 0; --i)
                        {
                            if (m_current->locals[i].name == classDecl->name)
                            {
                                slot = i; break;
                            }
                        }
                        Emit(OP_GET_LOCAL);
                        Emit(static_cast<uint8_t>(slot));
                    }

                    Emit(OP_METHOD);
                    Emit16(MakeConstant(std::make_shared<std::string>(varDeclaration->name)));
                }
            }
        }

        bool classHasExplicitInit = false;
        for (const auto& member : classDecl->members)
        {
            if (auto* f = dynamic_cast<FuncDeclStmt*>(member.get()))
            {
                if (f->name == "init") {
                    classHasExplicitInit = true;
                    break;
                }
            }
        }

        if (!classHasExplicitInit)
        {
            std::vector<VarDeclStmt*> defaultFields;
            for (const auto& member : classDecl->members)
            {
                auto* v = dynamic_cast<VarDeclStmt*>(member.get());
                if (v && !v->isStatic && !v->computedBody && v->initExpr)
                {
                    defaultFields.push_back(v);
                }
            }
            if (!defaultFields.empty())
            {
                InitState("init", 0, TypeInfo::Simple(TypeKind::Any), TypeInfo::Class(classDecl->name));
                BeginScope();
                m_current->locals[0].name = "self";
                m_current->locals[0].type = TypeInfo::Class(classDecl->name);

                for (auto* v : defaultFields)
                {
                    Emit(OP_GET_LOCAL); Emit(static_cast<uint8_t>(0));
                    CompileExpr(v->initExpr.get());
                    Emit(OP_SET_PROPERTY); Emit16(MakeConstant(std::make_shared<std::string>(v->name)));
                    Emit(OP_POP);
                }

                Emit(OP_GET_LOCAL); Emit(static_cast<uint8_t>(0));
                Emit(OP_RETURN); EndScope();

                FunctionPtr initFunc = EndState();
                Emit(OP_CONSTANT); Emit16(MakeConstant(initFunc));

                if (isGlobalScope())
                {
                    Emit(OP_GET_GLOBAL);
                    Emit16(MakeConstant(std::make_shared<std::string>(classDecl->name)));
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
                Emit16(MakeConstant(std::make_shared<std::string>("init")));
            }
        }
    }

    void Visit(StructDeclStmt* structDecl) override
    {
        m_classes[structDecl->name] = ClassInfo{structDecl->name, true, {}, {}, {}};

        ClassInfo structInfo;
        structInfo.name = structDecl->name;
        structInfo.isStruct = true;
        structInfo.implementedInterfaces = structDecl->implementedInterfaces;

        for (const auto& member : structDecl->members)
        {
            if (auto* funcDeclaration = dynamic_cast<FuncDeclStmt*>(member.get()))
            {
                std::vector<TypeInfoPtr> paramTypes;
                for (const auto& param : funcDeclaration->parameters)
                {
                    paramTypes.push_back(ResolveType(param.type));
                }
                structInfo.methods[funcDeclaration->name] = { ResolveType(funcDeclaration->returnType), paramTypes, funcDeclaration->accessLevel };
            }
            else if (auto* varDeclaration = dynamic_cast<VarDeclStmt*>(member.get()))
            {
                if (varDeclaration->isStatic)
                {
                    structInfo.staticFieldNames.insert(varDeclaration->name);
                    continue;
                }
                TypeInfoPtr type = varDeclaration->typeName.empty()
                    ? TypeInfo::Simple(TypeKind::Any)
                    : ResolveType(varDeclaration->typeName);
                if (varDeclaration->computedBody)
                {
                    structInfo.methods[varDeclaration->name] = { type, {}, varDeclaration->accessLevel };
                }
                else
                {
                    structInfo.fields[varDeclaration->name] = { type, varDeclaration->isWeak, varDeclaration->accessLevel };
                }
            }
        }
        m_classes[structDecl->name] = structInfo;
        ValidateInterfaces(structDecl->name, structDecl->implementedInterfaces, structInfo);

        Emit(OP_STRUCT);
        Emit16(MakeConstant(std::make_shared<std::string>(structDecl->name)));

        if (isGlobalScope())
        {
            Emit(OP_DEFINE_GLOBAL);
            Emit16(MakeConstant(std::make_shared<std::string>(structDecl->name)));
            m_globalVariables[structDecl->name] = { true, TypeInfo::Class(structDecl->name) };
        }
        else
        {
            int slot = static_cast<int>(m_current->locals.size());
            Emit(OP_SET_LOCAL);
            Emit(static_cast<uint8_t>(slot));
            Emit(OP_POP);
            m_current->locals.push_back({structDecl->name, m_current->scopeDepth, true, TypeInfo::Class(structDecl->name)});
            m_current->maxLocals = std::max(m_current->maxLocals, static_cast<int>(m_current->locals.size()));
        }

        for (const auto& member : structDecl->members)
        {
            if (auto* funcDeclaration = dynamic_cast<FuncDeclStmt*>(member.get()))
            {
                InitState(
                    funcDeclaration->name,
                    static_cast<int>(funcDeclaration->parameters.size()),
                    ResolveType(funcDeclaration->returnType),
                    TypeInfo::Class(structDecl->name)
                );
                BeginScope();
                m_current->locals[0].name = "self";
                m_current->locals[0].type = TypeInfo::Class(structDecl->name);
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
                    Emit16(MakeConstant(false));
                }
                Emit(OP_RETURN);
                EndScope();

                FunctionPtr compiledMethod = EndState();
                Emit(OP_CONSTANT);
                Emit16(MakeConstant(compiledMethod));

                if (isGlobalScope())
                {
                    Emit(OP_GET_GLOBAL);
                    Emit16(MakeConstant(std::make_shared<std::string>(structDecl->name)));
                }
                else
                {
                    int slot = -1;
                    for (int i = static_cast<int>(m_current->locals.size()) - 1; i >= 0; --i)
                    {
                        if (m_current->locals[i].name == structDecl->name)
                        {
                            slot = i;
                            break;
                        }
                    }
                    Emit(OP_GET_LOCAL);
                    Emit(static_cast<uint8_t>(slot));
                }
                Emit(OP_METHOD);
                Emit16(MakeConstant(std::make_shared<std::string>(funcDeclaration->name)));
            }
            else if (auto* varDeclaration = dynamic_cast<VarDeclStmt*>(member.get()))
            {
                if (varDeclaration->isStatic)
                {
                    continue;
                }

                if (!varDeclaration->computedBody)
                {
                    if (isGlobalScope())
                    {
                        Emit(OP_GET_GLOBAL);
                        Emit16(MakeConstant(std::make_shared<std::string>(structDecl->name)));
                    }
                    else
                    {
                        int slot = -1;
                        for (int i = static_cast<int>(m_current->locals.size()) - 1; i >= 0; --i)
                        {
                            if (m_current->locals[i].name == structDecl->name)
                            {
                                slot = i;
                                break;
                            }
                        }
                        Emit(OP_GET_LOCAL);
                        Emit(static_cast<uint8_t>(slot));
                    }
                    Emit(varDeclaration->isTrackable ? OP_FIELD_TRACKABLE : OP_FIELD);
                    Emit16(MakeConstant(std::make_shared<std::string>(varDeclaration->name)));
                    Emit(OP_POP);
                }
                else
                {
                    TypeInfoPtr retType = varDeclaration->typeName.empty()
                        ? TypeInfo::Simple(TypeKind::Any)
                        : ResolveType(varDeclaration->typeName);
                    InitState(varDeclaration->name, 0, retType, TypeInfo::Class(structDecl->name));
                    BeginScope();
                    m_current->locals[0].name = "self";
                    m_current->locals[0].type = TypeInfo::Class(structDecl->name);
                    for (const auto& s : varDeclaration->computedBody->statements)
                    {
                        CompileStmt(s.get());
                    }
                    Emit(OP_CONSTANT);
                    Emit16(MakeConstant(false));
                    Emit(OP_RETURN);
                    EndScope();

                    FunctionPtr compiledGetter = EndState();
                    Emit(OP_CONSTANT);
                    Emit16(MakeConstant(compiledGetter));

                    if (isGlobalScope())
                    {
                        Emit(OP_GET_GLOBAL);
                        Emit16(MakeConstant(std::make_shared<std::string>(structDecl->name)));
                    }
                    else
                    {
                        int slot = -1;
                        for (int i = static_cast<int>(m_current->locals.size()) - 1; i >= 0; --i)
                        {
                            if (m_current->locals[i].name == structDecl->name)
                            {
                                slot = i;
                                break;
                            }
                        }
                        Emit(OP_GET_LOCAL);
                        Emit(static_cast<uint8_t>(slot));
                    }
                    Emit(OP_METHOD);
                    Emit16(MakeConstant(std::make_shared<std::string>(varDeclaration->name)));
                }
            }
        }

        for (const auto& member : structDecl->members)
        {
            auto* varDeclaration = dynamic_cast<VarDeclStmt*>(member.get());
            if (!varDeclaration || !varDeclaration->isStatic || !varDeclaration->initExpr)
            {
                continue;
            }

            CompileExpr(varDeclaration->initExpr.get());

            if (isGlobalScope())
            {
                Emit(OP_GET_GLOBAL);
                Emit16(MakeConstant(std::make_shared<std::string>(structDecl->name)));
            }
            else
            {
                int slot = -1;
                for (int i = static_cast<int>(m_current->locals.size()) - 1; i >= 0; --i)
                {
                    if (m_current->locals[i].name == structDecl->name)
                    {
                        slot = i;
                        break;
                    }
                }
                Emit(OP_GET_LOCAL);
                Emit(static_cast<uint8_t>(slot));
            }
            Emit(OP_STATIC_FIELD);
            Emit16(MakeConstant(std::make_shared<std::string>(varDeclaration->name)));
        }

        bool hasExplicitInit = false;
        for (const auto& member : structDecl->members)
        {
            if (auto* f = dynamic_cast<FuncDeclStmt*>(member.get()))
            {
                if (f->name == "init")
                {
                    hasExplicitInit = true;
                    break;
                }
            }
        }

        if (!hasExplicitInit)
        {
            std::vector<VarDeclStmt*> requiredFields, defaultFields;
            for (const auto& member : structDecl->members)
            {
                auto* v = dynamic_cast<VarDeclStmt*>(member.get());
                if (!v || v->isStatic || v->computedBody)
                {
                    continue;
                }
                if (v->initExpr)
                {
                    defaultFields.push_back(v);
                }
                else
                {
                    requiredFields.push_back(v);
                }
            }

            int paramCount = static_cast<int>(requiredFields.size());

            std::vector<TypeInfoPtr> initParamTypes(paramCount, TypeInfo::Simple(TypeKind::Any));
            m_classes[structDecl->name].methods["init"] = { TypeInfo::Class(structDecl->name), initParamTypes, AccessLevel::Internal };

            InitState("init", paramCount, TypeInfo::Simple(TypeKind::Any), TypeInfo::Class(structDecl->name));
            BeginScope();
            m_current->locals[0].name = "self";
            m_current->locals[0].type = TypeInfo::Class(structDecl->name);

            for (int fi = 0; fi < paramCount; ++fi)
            {
                m_current->locals.push_back({requiredFields[fi]->name, m_current->scopeDepth, true, TypeInfo::Simple(TypeKind::Any)});
                m_current->maxLocals = std::max(m_current->maxLocals, static_cast<int>(m_current->locals.size()));
            }

            for (int fi = 0; fi < paramCount; ++fi)
            {
                Emit(OP_GET_LOCAL);
                Emit(0);
                Emit(OP_GET_LOCAL);
                Emit(static_cast<uint8_t>(fi + 1));
                Emit(OP_SET_PROPERTY);
                Emit16(MakeConstant(std::make_shared<std::string>(requiredFields[fi]->name)));
                Emit(OP_POP);
            }

            for (auto* v : defaultFields)
            {
                Emit(OP_GET_LOCAL); Emit(0);
                CompileExpr(v->initExpr.get());
                Emit(OP_SET_PROPERTY);
                Emit16(MakeConstant(std::make_shared<std::string>(v->name)));
                Emit(OP_POP);
            }

            Emit(OP_GET_LOCAL);
            Emit(0);
            Emit(OP_RETURN);
            EndScope();

            FunctionPtr initFunc = EndState();
            Emit(OP_CONSTANT);
            Emit16(MakeConstant(initFunc));

            if (isGlobalScope())
            {
                Emit(OP_GET_GLOBAL);
                Emit16(MakeConstant(std::make_shared<std::string>(structDecl->name)));
            }
            else
            {
                int slot = -1;
                for (int i = static_cast<int>(m_current->locals.size()) - 1; i >= 0; --i)
                {
                    if (m_current->locals[i].name == structDecl->name)
                    {
                        slot = i;
                        break;
                    }
                }

                Emit(OP_GET_LOCAL);
                Emit(static_cast<uint8_t>(slot));
            }
            Emit(OP_METHOD);
            Emit16(MakeConstant(std::make_shared<std::string>("init")));
        }
    }

    void Visit(AssignExpr* assign) override
    {
        if (auto* ident = dynamic_cast<IdentifierExpr*>(assign->target.get()))
        {
            TypeInfoPtr varType = TypeInfo::Simple(TypeKind::Any);
            int slot = ResolveLocal(m_current, ident->name);
            int upvalue = -1;

            if (slot != -1)
            {
                if (m_current->locals[slot].isConst)
                {
                    throw std::runtime_error("Cannot reassign to const");
                }
                varType = m_current->locals[slot].type;
            }
            else
            {
                upvalue = ResolveUpvalue(m_current, ident->name, varType);
                if (upvalue == -1)
                {
                    if (m_globalVariables.contains(ident->name))
                    {
                        if (m_globalVariables[ident->name].isConst)
                        {
                            throw std::runtime_error("Cannot reassign to global const");
                        }
                        varType = m_globalVariables[ident->name].type;
                    }
                    else
                    {
                        throw std::runtime_error("Undefined variable '" + ident->name + "'");
                    }
                }
            }

            TypeInfoPtr valType = CompileExpr(assign->value.get());
            if (!AreTypesCompatible(valType, varType))
            {
                throw std::runtime_error("Type Error: Incompatible assignment");
            }

            if (slot != -1)
            {
                Emit(OP_SET_LOCAL);
                Emit(static_cast<uint8_t>(slot));
            }
            else if (upvalue != -1)
            {
                Emit(OP_SET_UPVALUE);
                Emit(static_cast<uint8_t>(upvalue));
            }
            else
            {
                Emit(OP_SET_GLOBAL);
                Emit16(MakeConstant(std::make_shared<std::string>(ident->name)));
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
            Emit16(MakeConstant(num->value));
            m_lastExprType = TypeInfo::Simple(TypeKind::Double);
        }
        else
        {
            Emit16(MakeConstant(static_cast<int64_t>(num->value)));
            m_lastExprType = TypeInfo::Simple(TypeKind::Int);
        }
    }

    void Visit(BoolExpr* b) override
    {
        Emit(OP_CONSTANT);
        Emit16(MakeConstant(b->value));
        m_lastExprType = TypeInfo::Simple(TypeKind::Bool);
    }

    void Visit(StringExpr* str) override
    {
        Emit(OP_CONSTANT);
        Emit16(MakeConstant(std::make_shared<std::string>(str->value)));
        m_lastExprType = TypeInfo::Simple(TypeKind::String);
    }

    void Visit(BinaryExpr* bin) override
    {
        if (bin->op == "??")
        {
            CompileExpr(bin->left.get());
            int jumpToEnd = EmitJump(OP_JUMP_IF_NOT_NULL);
            CompileExpr(bin->right.get());
            PatchJump(jumpToEnd);
            m_lastExprType = TypeInfo::Simple(TypeKind::Any);
            return;
        }

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
            {
                m_lastExprType = TypeInfo::Simple(TypeKind::Double);
            }
            else
            {
                m_lastExprType = TypeInfo::Simple(TypeKind::Int);
            }
            return;
        }

        if (bin->op == "==" || bin->op == "!=" || bin->op == "<" || bin->op == ">" || bin->op == "<=" || bin->op == ">=")
        {
            if (!lType->IsCompatible(rType))
            {
                throw std::runtime_error("Type Error at line " + std::to_string(bin->line) + ": Cannot compare different types");
            }

            if (bin->op == "==")
                Emit(OP_EQUAL);
            else if (bin->op == "<")
                Emit(OP_LESS);
            else if (bin->op == ">")
                Emit(OP_GREATER);
            else if (bin->op == "<=")
            {
                Emit(OP_GREATER);
                Emit(OP_NOT);
            }
            else if (bin->op == ">=")
            {
                Emit(OP_LESS);
                Emit(OP_NOT);
            }
            else if (bin->op == "!=")
            {
                Emit(OP_EQUAL);
                Emit(OP_NOT);
            }

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
        }
    }

    void Visit(IdentifierExpr* ident) override
    {
        if (ident->name == "self" && !m_current->currentClass)
        {
            throw std::runtime_error("Cannot use 'self' outside class");
        }

        int slot = ResolveLocal(m_current, ident->name);
        TypeInfoPtr typeOut = TypeInfo::Simple(TypeKind::Any);

        if (slot != -1)
        {
            Emit(OP_GET_LOCAL);
            Emit(static_cast<uint8_t>(slot));
            typeOut = m_current->locals[slot].type;
        }
        else
        {
            int upvalue = ResolveUpvalue(m_current, ident->name, typeOut);
            if (upvalue != -1)
            {
                Emit(OP_GET_UPVALUE);
                Emit(static_cast<uint8_t>(upvalue));
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
                    auto funcType = TypeInfo::Simple(TypeKind::Func);
                    funcType->paramTypes = m_functions[ident->name].paramTypes;
                    funcType->returnType = m_functions[ident->name].returnType;
                    typeOut = funcType;
                }
                else
                {
                    throw std::runtime_error("Compiler Error at line " + std::to_string(ident->line) + ": Undefined identifier '" + ident->name + "'");
                }

                Emit(OP_GET_GLOBAL);
                Emit16(MakeConstant(std::make_shared<std::string>(ident->name)));
            }
        }
        m_lastExprType = typeOut;
    }

    void Visit(CallExpr* call) override
    {
        bool isConstructor = false;
        bool isFunc = false;
        std::string funcName;

        if (auto* identCallee = dynamic_cast<IdentifierExpr*>(call->callee.get()))
        {
            funcName = identCallee->name;
            if (m_classes.contains(funcName))
            {
                isConstructor = true;
            }
            else if (m_functions.contains(funcName))
            {
                isFunc = true;
            }
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
                    if (method.accessLevel == AccessLevel::Private)
                    {
                        if (!m_current->currentClass || m_current->currentClass->name != objType->name)
                        {
                            throw std::runtime_error("Access Error at line " + std::to_string(call->line) +
                                ": Method '" + getCallee->propertyName + "' is private and cannot be called outside of '" + objType->name + "'");
                        }
                    }
                    Emit(OP_GET_PROPERTY);
                    Emit16(MakeConstant(std::make_shared<std::string>(getCallee->propertyName)));

                    if (call->arguments.size() != method.paramTypes.size())
                    {
                        throw std::runtime_error("Type Error at line " + std::to_string(call->line) + ": Method arguments count mismatch");
                    }

                    for (size_t i = 0; i < call->arguments.size(); ++i)
                    {
                        TypeInfoPtr argType = CompileExpr(call->arguments[i].get());
                        if (!AreTypesCompatible(argType, method.paramTypes[i]))
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

            Emit(OP_GET_PROPERTY);
            Emit16(MakeConstant(std::make_shared<std::string>(getCallee->propertyName)));
            for (const auto& arg : call->arguments)
            {
                CompileExpr(arg.get());
            }
            Emit(OP_CALL);
            Emit(static_cast<uint8_t>(call->arguments.size()));
            m_lastExprType = TypeInfo::Simple(TypeKind::Any);
            return;
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
                    if (!AreTypesCompatible(argType, initMethod.paramTypes[i]))
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
                if (!AreTypesCompatible(argType, func.paramTypes[i]))
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
        if (getExpr->isSafeChain)
        {
            CompileExpr(getExpr->object.get());
            int jumpIfNull = EmitJump(OP_JUMP_IF_NULL);
            Emit(OP_GET_PROPERTY);
            Emit16(MakeConstant(std::make_shared<std::string>(getExpr->propertyName)));
            int jumpToEnd = EmitJump(OP_JUMP);
            PatchJump(jumpIfNull);
            Emit(OP_NULL);
            PatchJump(jumpToEnd);
            m_lastExprType = TypeInfo::Simple(TypeKind::Any);
            return;
        }

        TypeInfoPtr objType = CompileExpr(getExpr->object.get());

        if (objType->kind == TypeKind::Array)
        {
            Emit(OP_GET_PROPERTY);
            Emit16(MakeConstant(std::make_shared<std::string>(getExpr->propertyName)));

            if (getExpr->propertyName == "count" ||
                getExpr->propertyName == "isEmpty")
            {
                m_lastExprType = TypeInfo::Simple(TypeKind::Int);
            }
            else if (getExpr->propertyName == "first" ||
                     getExpr->propertyName == "last")
            {
                m_lastExprType = TypeInfo::Simple(TypeKind::Any);
            }
            else if (getExpr->propertyName == "forEach" ||
                getExpr->propertyName == "map" ||
                getExpr->propertyName == "filter" ||
                getExpr->propertyName == "sort" ||
                getExpr->propertyName == "sorted" ||
                getExpr->propertyName == "append" ||
                getExpr->propertyName == "remove" ||
                getExpr->propertyName == "removeAt" ||
                getExpr->propertyName == "reversed" ||
                getExpr->propertyName == "contains" ||
                getExpr->propertyName == "joined" ||
                getExpr->propertyName == "reduce")
            {
                m_lastExprType = TypeInfo::Simple(TypeKind::Func);
            }
            else
            {
               throw std::runtime_error("Compiler Error at line " + std::to_string(getExpr->line) +
                   ": Array has no property or method '" + getExpr->propertyName + "'");
            }
            return;
        }

        if (objType->kind == TypeKind::Class && m_classes.contains(objType->name))
        {
            const auto& klass = m_classes[objType->name];

            if (klass.fields.contains(getExpr->propertyName))
            {
                const auto& fieldInfo = klass.fields.at(getExpr->propertyName);
                if (fieldInfo.accessLevel == AccessLevel::Private)
                {
                    if (!m_current->currentClass || m_current->currentClass->name != objType->name)
                    {
                        throw std::runtime_error("Access Error at line " + std::to_string(getExpr->line) +
                            ": Property '" + getExpr->propertyName + "' is private and cannot be accessed outside of '" + objType->name + "'");
                    }
                }
                Emit(OP_GET_PROPERTY);
                Emit16(MakeConstant(std::make_shared<std::string>(getExpr->propertyName)));
                m_lastExprType = klass.fields.at(getExpr->propertyName).type;
                return;
            }

            if (klass.methods.contains(getExpr->propertyName))
            {
                const auto& methodInfo = klass.methods.at(getExpr->propertyName);

                if (methodInfo.accessLevel == AccessLevel::Private)
                {
                    if (!m_current->currentClass || m_current->currentClass->name != objType->name)
                    {
                        throw std::runtime_error("Access Error at line " + std::to_string(getExpr->line) +
                            ": Property/Method '" + getExpr->propertyName + "' is private and cannot be accessed outside of '" + objType->name + "'");
                    }
                }
                Emit(OP_GET_PROPERTY);
                Emit16(MakeConstant(std::make_shared<std::string>(getExpr->propertyName)));

                m_lastExprType = TypeInfo::Simple(TypeKind::Func);
                return;
            }
            if (klass.staticFieldNames.contains(getExpr->propertyName))
            {
                Emit(OP_GET_PROPERTY);
                Emit16(MakeConstant(std::make_shared<std::string>(getExpr->propertyName)));
                m_lastExprType = TypeInfo::Simple(TypeKind::Any);
                return;
            }
            throw std::runtime_error("Compiler Error at line " + std::to_string(getExpr->line) + ": Class/Struct '" + objType->name + "' has no property or method '" + getExpr->propertyName + "'");
        }

        Emit(OP_GET_PROPERTY);
        Emit16(MakeConstant(std::make_shared<std::string>(getExpr->propertyName)));
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

            if (!AreTypesCompatible(valType, targetType))
            {
                throw std::runtime_error("Type Error at line " + std::to_string(setExpr->line) + ": Cannot assign incompatible value to property '" + setExpr->propertyName + "'");
            }

            const auto& fieldInfo = klass.fields.at(setExpr->propertyName);
            if (fieldInfo.accessLevel == AccessLevel::Private)
            {
                if (!m_current->currentClass || m_current->currentClass->name != objType->name)
                {
                    throw std::runtime_error("Access Error at line " + std::to_string(setExpr->line) +
                        ": Property '" + setExpr->propertyName + "' is private and cannot be accessed outside of '" + objType->name + "'");
                }
            }

            if (isWeak)
            {
                Emit(OP_SET_PROPERTY_WEAK);
            }
            else
            {
                Emit(OP_SET_PROPERTY);
            }

            Emit16(MakeConstant(std::make_shared<std::string>(setExpr->propertyName)));
            m_lastExprType = targetType;
            return;
        }

        Emit(OP_SET_PROPERTY);
        Emit16(MakeConstant(std::make_shared<std::string>(setExpr->propertyName)));
        m_lastExprType = TypeInfo::Simple(TypeKind::Any);
    }

    void Visit(ClosureExpr* closure) override
    {
        static int closureCount = 0;
        std::string name = "anon_closure_" + std::to_string(closureCount++);

        TypeInfoPtr retType = ResolveType(closure->returnType);
        std::vector<TypeInfoPtr> paramTypes;
        paramTypes.reserve(closure->parameters.size());
        for (const auto& param : closure->parameters)
        {
            paramTypes.push_back(ResolveType(param.type));
        }

        InitState(name, static_cast<int>(closure->parameters.size()), retType, m_current->currentClass);
        BeginScope();

        for (const auto& param : closure->parameters)
        {
            m_current->locals.push_back({param.name, m_current->scopeDepth, true, ResolveType(param.type)});
            m_current->maxLocals = std::max(m_current->maxLocals, static_cast<int>(m_current->locals.size()));
        }

        for (const auto& s : closure->body->statements)
        {
            CompileStmt(s.get());
        }

        Emit(OP_CONSTANT);
        Emit16(MakeConstant(false));
        Emit(OP_RETURN);

        EndScope();

        std::vector<Upvalue> capturedUpvalues = m_current->upvalues;
        FunctionPtr compiledClosure = EndState();

        Emit(OP_CLOSURE);
        Emit16(MakeConstant(compiledClosure));
        Emit(static_cast<uint8_t>(capturedUpvalues.size()));

        for (const auto& uv : capturedUpvalues)
        {
            Emit(uv.isLocal ? 1 : 0);
            Emit(uv.index);
        }

        auto funcType = TypeInfo::Simple(TypeKind::Func);
        funcType->paramTypes = paramTypes;
        funcType->returnType = retType;
        m_lastExprType = funcType;
    }

    void Visit(NullExpr* expr) override
    {
        Emit(OP_CONSTANT);
        Emit16(MakeConstant(Null{}));
        m_lastExprType = TypeInfo::Simple(TypeKind::Null);
    }

    void Visit(AwaitExpr* expr) override
    {
        expr->expr->Accept(this);
        Emit(OP_AWAIT);
        m_lastExprType = TypeInfo::Simple(TypeKind::Any);
    }

    void Visit(InterfaceDeclStmt* interfaceDecl) override
    {
        InterfaceInfo info;
        info.name = interfaceDecl->name;

        m_interfaces[interfaceDecl->name] = info;

        for (const auto& method : interfaceDecl->methods)
        {
            std::vector<TypeInfoPtr> paramTypes;
            for (const auto& param : method->parameters)
            {
                paramTypes.push_back(ResolveType(param.type));
            }
            m_interfaces[interfaceDecl->name].methods[method->name] = { ResolveType(method->returnType), paramTypes, AccessLevel::Internal };
        }

        for (const auto& prop : interfaceDecl->properties)
        {
            m_interfaces[interfaceDecl->name].properties[prop->name] = { ResolveType(prop->typeName), false, AccessLevel::Internal };
        }
    }

    void Visit(ImportStmt* stmt) override
    {
        namespace fs = std::filesystem;

        fs::path importPath(stmt->path);
        if (!importPath.has_extension())
        {
            importPath += ".rocket";
        }

        fs::path resolved;
        std::string raw = stmt->path;
        if (raw.starts_with("./") || raw.starts_with("../"))
        {
            fs::path base = m_importDirStack.empty()
                ? fs::current_path()
                : fs::path(m_importDirStack.back());
            resolved = fs::weakly_canonical(base / importPath);
        }
        else
        {
            resolved = fs::weakly_canonical(fs::current_path() / importPath);
        }

        std::string modulePath = resolved.string();

        if (m_importedModules.contains(modulePath))
        {
            return;
        }

        if (m_importStack.contains(modulePath))
        {
            throw std::runtime_error("Circular dependency detected: " + modulePath);
        }

        m_importStack.insert(modulePath);

        std::ifstream file(modulePath);
        if (!file.is_open())
        {
            throw std::runtime_error("Compile Error: Could not open imported file: " + modulePath);
        }

        std::string source((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();

        m_importDirStack.push_back(resolved.parent_path().string());

        Lexer moduleLexer(source);
        std::vector<Token> moduleTokens = moduleLexer.Tokenize();
        ASTBuilder moduleParser(std::move(moduleTokens));
        auto moduleAST = moduleParser.Parse();

        for (const auto& astNode : moduleAST)
        {
            astNode->Accept(this);
        }

        m_importDirStack.pop_back();
        m_importStack.erase(modulePath);
        m_importedModules.insert(modulePath);
    }

    void Visit(ForInStmt* stmt) override
    {
        BeginScope();

        CompileExpr(stmt->iterable.get());
        int iterSlot = static_cast<int>(m_current->locals.size());
        Emit(OP_SET_LOCAL);
        Emit(static_cast<uint8_t>(iterSlot));
        Emit(OP_POP);
        m_current->locals.push_back({"__iter", m_current->scopeDepth, true, TypeInfo::Simple(TypeKind::Any)});
        m_current->maxLocals = std::max(m_current->maxLocals, static_cast<int>(m_current->locals.size()));

        Emit(OP_GET_LOCAL);
        Emit(static_cast<uint8_t>(iterSlot));
        Emit(OP_ARRAY_LEN);
        int lenSlot = static_cast<int>(m_current->locals.size());
        Emit(OP_SET_LOCAL);
        Emit(static_cast<uint8_t>(lenSlot));
        Emit(OP_POP);
        m_current->locals.push_back({"__len", m_current->scopeDepth, true, TypeInfo::Simple(TypeKind::Int)});
        m_current->maxLocals = std::max(m_current->maxLocals, static_cast<int>(m_current->locals.size()));

        Emit(OP_CONSTANT);
        Emit16(MakeConstant(0));
        int iSlot = static_cast<int>(m_current->locals.size());
        Emit(OP_SET_LOCAL);
        Emit(static_cast<uint8_t>(iSlot));
        Emit(OP_POP);
        m_current->locals.push_back({"__i", m_current->scopeDepth, false, TypeInfo::Simple(TypeKind::Int)});
        m_current->maxLocals = std::max(m_current->maxLocals, static_cast<int>(m_current->locals.size()));

        int loopStart = static_cast<int>(m_current->function->chunk->code.size());
        Emit(OP_GET_LOCAL);
        Emit(static_cast<uint8_t>(iSlot));
        Emit(OP_GET_LOCAL);
        Emit(static_cast<uint8_t>(lenSlot));
        Emit(OP_LESS);
        int exitJump = EmitJump(OP_JUMP_IF_FALSE);

        BeginScope();
        Emit(OP_GET_LOCAL);
        Emit(static_cast<uint8_t>(iterSlot));
        Emit(OP_GET_LOCAL);
        Emit(static_cast<uint8_t>(iSlot));
        Emit(OP_GET_INDEX);
        int itemSlot = static_cast<int>(m_current->locals.size());
        Emit(OP_SET_LOCAL);
        Emit(static_cast<uint8_t>(itemSlot));
        Emit(OP_POP);
        m_current->locals.push_back({stmt->varName, m_current->scopeDepth, true, TypeInfo::Simple(TypeKind::Any)});
        m_current->maxLocals = std::max(m_current->maxLocals, static_cast<int>(m_current->locals.size()));

        for (const auto& s : stmt->body->statements)
        {
            CompileStmt(s.get());
        }

        EndScope();

        Emit(OP_GET_LOCAL); Emit(static_cast<uint8_t>(iSlot));
        Emit(OP_CONSTANT); Emit16(MakeConstant(1));
        Emit(OP_ADD);
        Emit(OP_SET_LOCAL); Emit(static_cast<uint8_t>(iSlot));
        Emit(OP_POP);

        int loopJump = EmitJump(OP_JUMP);
        m_current->function->chunk->code[loopJump] = (loopStart >> 8) & 0xff;
        m_current->function->chunk->code[loopJump + 1] = loopStart & 0xff;
        PatchJump(exitJump);

        EndScope();
    }

    void Visit(EnumDeclStmt* stmt) override
    {
        ClassInfo enumInfo;
        enumInfo.name = stmt->name;
        for (const auto& c : stmt->cases)
        {
            enumInfo.staticFieldNames.insert(c);
        }
        m_classes[stmt->name] = enumInfo;

        Emit(OP_CLASS);
        Emit16(MakeConstant(std::make_shared<std::string>(stmt->name)));

        if (isGlobalScope())
        {
            Emit(OP_DEFINE_GLOBAL);
            Emit16(MakeConstant(std::make_shared<std::string>(stmt->name)));
            m_globalVariables[stmt->name] = { true, TypeInfo::Class(stmt->name) };
        }
        else
        {
            int slot = static_cast<int>(m_current->locals.size());
            Emit(OP_SET_LOCAL); Emit(static_cast<uint8_t>(slot)); Emit(OP_POP);
            m_current->locals.push_back({stmt->name, m_current->scopeDepth, true, TypeInfo::Class(stmt->name)});
            m_current->maxLocals = std::max(m_current->maxLocals, static_cast<int>(m_current->locals.size()));
        }

        for (int i = 0; i < static_cast<int>(stmt->cases.size()); ++i)
        {
            const std::string& caseName = stmt->cases[i];

            Emit(OP_CONSTANT);
            Emit16(MakeConstant(i));

            if (isGlobalScope())
            {
                Emit(OP_GET_GLOBAL);
                Emit16(MakeConstant(std::make_shared<std::string>(stmt->name)));
            }
            else
            {
                int slot = -1;
                for (int j = static_cast<int>(m_current->locals.size()) - 1; j >= 0; --j)
                {
                    if (m_current->locals[j].name == stmt->name)
                    {
                        slot = j;
                        break;
                    }
                }
                Emit(OP_GET_LOCAL);
                Emit(static_cast<uint8_t>(slot));
            }

            Emit(OP_STATIC_FIELD);
            Emit16(MakeConstant(std::make_shared<std::string>(caseName)));
        }
    }

    void Visit(IfLetStmt* stmt) override
    {
        CompileExpr(stmt->initExpr.get());

        int jumpToElse = EmitJump(OP_JUMP_IF_NULL);

        int xSlot = static_cast<int>(m_current->locals.size());
        Emit(OP_SET_LOCAL);
        Emit(static_cast<uint8_t>(xSlot));
        Emit(OP_POP);
        m_current->locals.push_back({stmt->varName, m_current->scopeDepth, stmt->isConst, TypeInfo::Simple(TypeKind::Any)});
        m_current->maxLocals = std::max(m_current->maxLocals, static_cast<int>(m_current->locals.size()));

        CompileStmt(stmt->trueBlock.get());

        if (!m_current->locals.empty() && m_current->locals.back().name == stmt->varName)
        {
            m_current->locals.pop_back();
        }

        int jumpToEnd = EmitJump(OP_JUMP);
        PatchJump(jumpToElse);

        if (stmt->falseBlock)
        {
            CompileStmt(stmt->falseBlock.get());
        }

        PatchJump(jumpToEnd);
    }

    int ResolveLocal(CompilerState* state, const std::string& name)
    {
        for (int i = static_cast<int>(state->locals.size()) - 1; i >= 0; --i)
        {
            if (state->locals[i].name == name)
            {
                return i;
            }
        }
        return -1;
    }

    int AddUpvalue(CompilerState* state, uint8_t index, bool isLocal, TypeInfoPtr type)
    {
        int upvalueCount = static_cast<int>(state->upvalues.size());

        for (int i = 0; i < upvalueCount; i++)
        {
            if (state->upvalues[i].index == index && state->upvalues[i].isLocal == isLocal)
            {
                return i;
            }
        }

        state->upvalues.push_back({index, isLocal, type});
        return upvalueCount;
    }

    int ResolveUpvalue(CompilerState* state, const std::string& name, TypeInfoPtr& outType)
    {
        if (state->parent == nullptr)
        {
            return -1;
        }

        int local = ResolveLocal(state->parent, name);
        if (local != -1)
        {
            state->parent->locals[local].isCaptured = true;
            outType = state->parent->locals[local].type;
            return AddUpvalue(state, static_cast<uint8_t>(local), true, outType);
        }

        int upvalue = ResolveUpvalue(state->parent, name, outType);
        if (upvalue != -1)
        {
            return AddUpvalue(state, static_cast<uint8_t>(upvalue), false, outType);
        }

        return -1;
    }

    void ValidateInterfaces(const std::string& typeName, const std::vector<std::string>& implementedInterfaces, const ClassInfo& info)
    {
        for (const auto& interfaceName : implementedInterfaces)
        {
            if (!m_interfaces.contains(interfaceName))
            {
                throw std::runtime_error("Type Error: Interface '" + interfaceName + "' is not defined.");
            }

            const auto& interfaceInfo = m_interfaces[interfaceName];

            for (const auto& [mName, mFunc] : interfaceInfo.methods)
            {
                if (!info.methods.contains(mName))
                {
                    throw std::runtime_error("Type Error: '" + typeName + "' does not implement method '" + mName + "' from interface '" + interfaceName + "'.");
                }

                const auto& classMethod = info.methods.at(mName);

                if (!classMethod.returnType->IsCompatible(mFunc.returnType))
                {
                    throw std::runtime_error("Type Error: Return type mismatch for method '" + mName + "' in '" + typeName + "'.");
                }

                if (classMethod.paramTypes.size() != mFunc.paramTypes.size())
                {
                    throw std::runtime_error("Type Error: Parameter count mismatch for method '" + mName + "' in '" + typeName + "'.");
                }

                for (size_t i = 0; i < classMethod.paramTypes.size(); ++i)
                {
                    if (!classMethod.paramTypes[i]->IsCompatible(mFunc.paramTypes[i]))
                    {
                        throw std::runtime_error("Type Error: Parameter type mismatch for method '" + mName + "' in '" + typeName + "'.");
                    }
                }
            }

            for (const auto& [pName, pField] : interfaceInfo.properties)
            {
                bool hasField = info.fields.contains(pName);
                bool hasGetter = info.methods.contains(pName) && info.methods.at(pName).paramTypes.empty();

                if (!hasField && !hasGetter)
                {
                    throw std::runtime_error("Type Error: '" + typeName + "' does not implement property '" + pName + "' from interface '" + interfaceName + "'.");
                }

                TypeInfoPtr actualType = hasField ? info.fields.at(pName).type : info.methods.at(pName).returnType;
                if (!actualType->IsCompatible(pField.type))
                {
                    throw std::runtime_error("Type Error: Type mismatch for property '" + pName + "' in '" + typeName + "'.");
                }
            }
        }
    }

    bool AreTypesCompatible(const TypeInfoPtr& actual, const TypeInfoPtr& expected)
    {
        if (actual->kind == TypeKind::Any || expected->kind == TypeKind::Any)
        {
            return true;
        }
        if (actual->kind == TypeKind::Null)
        {
            return true;
        }
        if (expected->kind == TypeKind::Func && actual->kind == TypeKind::Func)
        {
            return true;
        }

        if (expected->kind == TypeKind::Interface)
        {
            if (actual->kind == TypeKind::Interface && actual->name == expected->name)
            {
                return true;
            }

            if (actual->kind == TypeKind::Class)
            {
                if (m_classes.contains(actual->name))
                {
                    const auto& classInfo = m_classes.at(actual->name);
                    for (const auto& implemented : classInfo.implementedInterfaces)
                    {
                        if (implemented == expected->name)
                        {
                            return true;
                        }
                    }
                }
            }
            return false;
        }

        if (expected->kind == TypeKind::Optional && actual->kind == TypeKind::Optional)
        {
            return AreTypesCompatible(actual->elementType, expected->elementType);
        }

        if (expected->kind == TypeKind::Array && actual->kind == TypeKind::Array)
        {
            if (!expected->elementType || !actual->elementType)
            {
                return true;
            }
            return AreTypesCompatible(actual->elementType, expected->elementType);
        }

        return actual->IsCompatible(expected);
    }
};