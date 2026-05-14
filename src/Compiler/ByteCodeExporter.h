#pragma once
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <stdexcept>
#include "../VirtualMachine/Value/Value.h"
#include "../VirtualMachine/Chunk.h"
#include "../Signature/Signature.h"

class ByteCodeExporter
{
public:
    void Export(const FunctionPtr& mainFunc, const std::string& filename)
    {
        std::ofstream out(filename, std::ios::binary);
        if (!out.is_open())
        {
            throw std::runtime_error("Could not open output file: " + filename);
        }

        m_funcIds.clear();
        m_allFuncs.clear();

        CollectFunctions(mainFunc);

        out.write(SIGNATURE.c_str(), 4);

        auto count = static_cast<uint32_t>(m_allFuncs.size());
        out.write(reinterpret_cast<const char*>(&count), sizeof(count));

        for (uint32_t i = 0; i < count; ++i)
        {
            auto func = m_allFuncs[i];

            auto nameLen = static_cast<uint16_t>(func->name.size());
            out.write(reinterpret_cast<const char*>(&nameLen), sizeof(nameLen));
            if (nameLen > 0)
            {
                out.write(func->name.data(), nameLen);
            }

            out.write(reinterpret_cast<const char*>(&func->arity), sizeof(func->arity));
            out.write(reinterpret_cast<const char*>(&func->maxLocals), sizeof(func->maxLocals));

            auto constCount = static_cast<uint32_t>(func->chunk->constants.size());
            out.write(reinterpret_cast<const char*>(&constCount), sizeof(constCount));

            for (const auto& c : func->chunk->constants)
            {
                if (std::holds_alternative<int64_t>(c))
                {
                    uint8_t tag = 0; out.write(reinterpret_cast<const char*>(&tag), 1);
                    int64_t v = std::get<int64_t>(c);
                    out.write(reinterpret_cast<const char*>(&v), sizeof(v));
                }
                else if (std::holds_alternative<double>(c))
                {
                    uint8_t tag = 1; out.write(reinterpret_cast<const char*>(&tag), 1);
                    double v = std::get<double>(c);
                    out.write(reinterpret_cast<const char*>(&v), sizeof(v));
                }
                else if (std::holds_alternative<bool>(c))
                {
                    uint8_t tag = 2; out.write(reinterpret_cast<const char*>(&tag), 1);
                    bool v = std::get<bool>(c);
                    out.write(reinterpret_cast<const char*>(&v), sizeof(v));
                }
                else if (std::holds_alternative<StringPtr>(c))
                {
                    uint8_t tag = 3; out.write(reinterpret_cast<const char*>(&tag), 1);
                    auto str = std::get<StringPtr>(c);
                    auto slen = static_cast<uint32_t>(str->size());
                    out.write(reinterpret_cast<const char*>(&slen), sizeof(slen));
                    out.write(str->data(), slen);
                }
                else if (std::holds_alternative<FunctionPtr>(c))
                {
                    uint8_t tag = 4; out.write(reinterpret_cast<const char*>(&tag), 1);
                    uint32_t fid = m_funcIds[std::get<FunctionPtr>(c).get()];
                    out.write(reinterpret_cast<const char*>(&fid), sizeof(fid));
                }
                else if (std::holds_alternative<Null>(c))
                {
                    uint8_t tag = 5; out.write(reinterpret_cast<const char*>(&tag), 1);
                }
            }

            auto codeLen = static_cast<uint32_t>(func->chunk->code.size());
            out.write(reinterpret_cast<const char*>(&codeLen), sizeof(codeLen));
            if (codeLen > 0)
            {
                out.write(reinterpret_cast<const char*>(func->chunk->code.data()), codeLen);
            }
        }
    }

private:
    std::unordered_map<Function*, uint32_t> m_funcIds;
    std::vector<FunctionPtr> m_allFuncs;

    void CollectFunctions(const FunctionPtr& func)
    {
        if (m_funcIds.contains(func.get())) return;

        auto id = static_cast<uint32_t>(m_allFuncs.size());
        m_funcIds[func.get()] = id;
        m_allFuncs.push_back(func);

        for (const auto& c : func->chunk->constants)
        {
            if (std::holds_alternative<FunctionPtr>(c))
            {
                CollectFunctions(std::get<FunctionPtr>(c));
            }
        }
    }
};