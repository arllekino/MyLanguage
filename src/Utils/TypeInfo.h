#pragma once
#include <memory>
#include <string>
#include <vector>

enum class TypeKind
{
    Any, Void, Null, Int, Double, Bool, String,
    Class, Array, Optional, Func
};

struct TypeInfo;
using TypeInfoPtr = std::shared_ptr<TypeInfo>;

struct TypeInfo
{
    TypeKind kind;
    std::string name;
    TypeInfoPtr elementType;

    TypeInfoPtr returnType;
    std::vector<TypeInfoPtr> paramTypes;

    bool IsCompatible(const TypeInfoPtr& other) const
    {
        if (kind == TypeKind::Any || other->kind == TypeKind::Any)
        {
            return true;
        }
        if (kind == TypeKind::Optional && other->kind == TypeKind::Null)
        {
            return true;
        }
        if (kind != other->kind)
        {
            return false;
        }
        if (kind == TypeKind::Class)
        {
            return name == other->name;
        }
        if (kind == TypeKind::Func)
        {
            if (paramTypes.size() != other->paramTypes.size())
            {
                return false;
            }
            for (size_t i = 0; i < paramTypes.size(); i++)
            {
                if (!paramTypes[i]->IsCompatible(other->paramTypes[i]))
                {
                    return false;
                }
            }
            return returnType->IsCompatible(other->returnType);
        }
        return true;
    }

    static TypeInfoPtr Simple(TypeKind kind)
    {
        return std::make_shared<TypeInfo>(TypeInfo{kind, "", nullptr, nullptr, {}});
    }

    static TypeInfoPtr Class(const std::string& name) {
        return std::make_shared<TypeInfo>(TypeInfo{TypeKind::Class, name, nullptr, nullptr, {}});
    }
};