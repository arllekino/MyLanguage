#pragma once
#include <fstream>
#include <vector>
#include <string>
#include <stdexcept>
#include <cstring>
#include "../VirtualMachine/Value/Value.h"
#include "../VirtualMachine/Chunk.h"
#include "../Signature/Signature.h"

class ByteCodeParser
{
public:
    explicit ByteCodeParser(const std::string& filename)
    {
        m_file.open(filename, std::ios::binary);
        if (!m_file.is_open())
            throw std::runtime_error("Could not open binary file: " + filename);
    }

    FunctionPtr Parse()
    {
        char magic[4];
        m_file.read(magic, 4);
        if (std::strncmp(magic, SIGNATURE.c_str(), 4) != 0)
        {
            throw std::runtime_error("Invalid file format: Invalid  bytecode file.");
        }

        uint32_t count = 0;
        m_file.read(reinterpret_cast<char*>(&count), sizeof(count));

        std::vector<FunctionPtr> funcs(count);
        for (uint32_t i = 0; i < count; ++i)
        {
            funcs[i] = std::make_shared<Function>();
            funcs[i]->chunk = std::make_unique<Chunk>();
        }

        for (uint32_t i = 0; i < count; ++i)
        {
            auto func = funcs[i];

            uint16_t nameLen = 0;
            m_file.read(reinterpret_cast<char*>(&nameLen), sizeof(nameLen));
            if (nameLen > 0)
            {
                func->name.resize(nameLen);
                m_file.read(func->name.data(), nameLen);
            }

            m_file.read(reinterpret_cast<char*>(&func->arity), sizeof(func->arity));
            m_file.read(reinterpret_cast<char*>(&func->maxLocals), sizeof(func->maxLocals));

            uint32_t constCount = 0;
            m_file.read(reinterpret_cast<char*>(&constCount), sizeof(constCount));
            func->chunk->constants.reserve(constCount);

            for (uint32_t j = 0; j < constCount; ++j)
            {
                uint8_t tag = 0;
                m_file.read(reinterpret_cast<char*>(&tag), 1);

                if (tag == 0) // int64_t
                {
                    int64_t v; m_file.read(reinterpret_cast<char*>(&v), sizeof(v));
                    func->chunk->constants.emplace_back(v);
                }
                else if (tag == 1) // double
                {
                    double v; m_file.read(reinterpret_cast<char*>(&v), sizeof(v));
                    func->chunk->constants.emplace_back(v);
                }
                else if (tag == 2) // bool
                {
                    bool v; m_file.read(reinterpret_cast<char*>(&v), sizeof(v));
                    func->chunk->constants.emplace_back(v);
                }
                else if (tag == 3) // string
                {
                    uint32_t slen = 0; m_file.read(reinterpret_cast<char*>(&slen), sizeof(slen));
                    std::string s(slen, '\0');
                    m_file.read(s.data(), slen);
                    func->chunk->constants.emplace_back(std::make_shared<std::string>(s));
                }
                else if (tag == 4) // FunctionPtr (Link)
                {
                    uint32_t fid = 0; m_file.read(reinterpret_cast<char*>(&fid), sizeof(fid));
                    if (fid >= funcs.size()) throw std::runtime_error("Invalid function ID in constants.");
                    func->chunk->constants.emplace_back(funcs[fid]);
                }
                else if (tag == 5) // Null
                {
                    func->chunk->constants.emplace_back(Null{});
                }
                else
                {
                    throw std::runtime_error("Unknown constant tag: " + std::to_string(tag));
                }
            }

            uint32_t codeLen = 0;
            m_file.read(reinterpret_cast<char*>(&codeLen), sizeof(codeLen));
            if (codeLen > 0)
            {
                func->chunk->code.resize(codeLen);
                m_file.read(reinterpret_cast<char*>(func->chunk->code.data()), codeLen);
            }
        }

        return funcs.empty() ? nullptr : funcs[0];
    }

private:
    std::ifstream m_file;
};