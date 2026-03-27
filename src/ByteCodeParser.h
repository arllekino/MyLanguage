#pragma once
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <stdexcept>

#include "VirtualMachine/OpCode.h"
#include "VirtualMachine/Chunk.h"

class ByteCodeParser
{
public:
    explicit ByteCodeParser(const std::string& filename)
    {
        m_file.open(filename);
        if (!m_file.is_open())
        {
            throw std::runtime_error("Could not open file: " + filename);
        }
    }

    Chunk Parse()
    {
        std::string line;

        while (std::getline(m_file, line))
        {
            if (line.empty() || line.starts_with("//")) continue;

            std::istringstream iss(line);
            std::string token;
            iss >> token;

            if (token == ".constants")
            {
                m_state = ParserState::Constants;
                continue;
            }
            if (token == ".code")
            {
                m_state = ParserState::Code;
                continue;
            }
            if (token == ".end_def")
            {
                break;
            }
            if (token.starts_with("."))
            {
                continue; // for a moment ignore .def, .argc, .locals, .name
            }

            if (m_state == ParserState::Constants)
            {
                ParseConstant(token, iss);
            }
            else if (m_state == ParserState::Code)
            {
                ParseInstruction(token, iss);
            }
        }

        BackpatchJumps();
        return std::move(m_chunk);
    }

private:
    enum class ParserState { Init, Constants, Code };

    std::ifstream m_file;
    Chunk m_chunk;
    ParserState m_state = ParserState::Init;

    std::unordered_map<std::string, uint16_t> m_labels;
    std::vector<std::pair<size_t, std::string>> m_unresolvedJumps;

    void ParseConstant(const std::string& type, std::istringstream& iss)
    {
        std::string valueStr;
        std::getline(iss >> std::ws, valueStr);

        if (type == "number")
        {
            if (valueStr.find('.') != std::string::npos)
            {
                m_chunk.constants.emplace_back(std::stod(valueStr));
            }
            else
            {
                m_chunk.constants.emplace_back(std::stoll(valueStr));
            }
        }
        else if (type == "string")
        {
            if (valueStr.size() >= 2 && valueStr.front() == '"' && valueStr.back() == '"')
            {
                valueStr = valueStr.substr(1, valueStr.size() - 2);
            }
            m_chunk.constants.emplace_back(std::make_shared<std::string>(valueStr));
        }
        else if (type == "true")  { m_chunk.constants.emplace_back(true); }
        else if (type == "false") { m_chunk.constants.emplace_back(false); }
        else
        {
            std::cerr << "Unknown constant type: " << type << std::endl;
        }
    }

    void ParseInstruction(const std::string& token, std::istringstream& iss)
    {
        if (token.back() == ':')
        {
            std::string labelName = token.substr(0, token.size() - 1);
            m_labels[labelName] = static_cast<uint16_t>(m_chunk.code.size());
            return;
        }

        const std::string& opcode = token;

        if (opcode == "const" || opcode == "set_local" || opcode == "get_local")
        {
            int arg; iss >> arg;
            uint8_t code = (opcode == "const") ? OP_CONSTANT :
                           (opcode == "set_local") ? OP_SET_LOCAL : OP_GET_LOCAL;

            m_chunk.code.push_back(code);
            m_chunk.code.push_back(static_cast<uint8_t>(arg));
        }
        else if (opcode == "jmp" || opcode == "jmp_false" || opcode == "loop")
        {
            std::string labelName;
            iss >> labelName;

            uint8_t code = (opcode == "jmp")
                ? OP_JUMP
                : (opcode == "loop")
                    ? OP_LOOP
                    : OP_JUMP_IF_FALSE;

            m_chunk.code.push_back(code);
            m_unresolvedJumps.emplace_back(m_chunk.code.size(), labelName);

            m_chunk.code.push_back(0);
            m_chunk.code.push_back(0);
        }
        else if (opcode == "add")
        {
            m_chunk.code.push_back(OP_ADD);
        }
        else if (opcode == "mul")
        {
            m_chunk.code.push_back(OP_MUL);
        }
        else if (opcode == "div")
        {
            m_chunk.code.push_back(OP_DIV);
        }
        else if (opcode == "mod")
        {
            m_chunk.code.push_back(OP_MOD);
        }
        else if (opcode == "sub")
        {
            m_chunk.code.push_back(OP_SUB);
        }
        else if (opcode == "clt")
        {
            m_chunk.code.push_back(OP_LESS);
        }
        else if (opcode == "cgt")
        {
            m_chunk.code.push_back(OP_GREATER);
        }
        else if (opcode == "ceq")
        {
            m_chunk.code.push_back(OP_EQUAL);
        }
        else if (opcode == "get_index")
        {
            m_chunk.code.push_back(OP_GET_INDEX);
        }
        else if (opcode == "set_index")
        {
            m_chunk.code.push_back(OP_SET_INDEX);
        }
        else if (opcode == "build_array")
        {
            m_chunk.code.push_back(OP_BUILD_ARRAY);
        }
        else if (opcode == "array_len")
        {
            m_chunk.code.push_back(OP_ARRAY_LEN);
        }
        else if (opcode == "array_push")
        {
            m_chunk.code.push_back(OP_ARRAY_PUSH);
        }
        else if (opcode == "pop")
        {
            m_chunk.code.push_back(OP_POP);
        }
        else if (opcode == "print")
        {
            m_chunk.code.push_back(OP_PRINT);
        }
        else if (opcode == "return")
        {
            m_chunk.code.push_back(OP_RETURN);
        }
        else
        {
            std::cerr << "Unknown opcode: " << opcode << std::endl;
        }
    }

    void BackpatchJumps()
    {
        for (const auto& [offset, labelName] : m_unresolvedJumps)
        {
            if (!m_labels.contains(labelName))
            {
                throw std::runtime_error("Unknown label: " + labelName);
            }

            uint16_t address = m_labels[labelName];

            m_chunk.code[offset] = (address >> 8) & 0xFF;
            m_chunk.code[offset + 1] = address & 0xFF;
        }
    }
};