#pragma once
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <stdexcept>

#include "VirtualMachine/OpCode.h"
#include "VirtualMachine/VirtualMachine.h"

class ByteCodeParser
{
public:
    explicit ByteCodeParser(const std::string& filename)
    {
        m_file.open(filename);
        if (!m_file.is_open())
            throw std::runtime_error("Could not open file: " + filename);
    }

    std::vector<FunctionPtr> Parse()
    {
        std::vector<FunctionPtr> functions;
        std::string line;

        while (std::getline(m_file, line))
        {
            if (line.empty() || line.starts_with("//")) continue;

            std::istringstream iss(line);
            std::string token;
            iss >> token;

            if (token == ".def")
            {
                ResetState();
                continue;
            }
            if (token == ".name")
            {
                iss >> m_currentFunction->name;
                continue;
            }
            if (token == ".argc")
            {
                iss >> m_currentFunction->arity;
                continue;
            }
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
                BackpatchJumps();
                functions.push_back(m_currentFunction);
                continue;
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

        return functions;
    }

private:
    enum class ParserState { Init, Constants, Code };

    std::ifstream m_file;
    ParserState m_state = ParserState::Init;

    FunctionPtr m_currentFunction;
    std::unordered_map<std::string, uint16_t> m_labels;
    std::vector<std::pair<size_t, std::string>> m_unresolvedJumps;

    void ResetState()
    {
        m_currentFunction = std::make_shared<Function>();
        m_currentFunction->chunk = std::make_unique<Chunk>();
        m_labels.clear();
        m_unresolvedJumps.clear();
        m_state = ParserState::Init;
    }

    static std::string UnescapeString(const std::string& s)
    {
        std::string res;
        for (size_t i = 0; i < s.size(); ++i)
        {
            if (s[i] == '\\' && i + 1 < s.size())
            {
                char c = s[++i];
                if (c == 'n') res += '\n';
                else if (c == 't') res += '\t';
                else if (c == '"') res += '"';
                else if (c == '\\') res += '\\';
                else res += c;
            }
            else
            {
                res += s[i];
            }
        }
        return res;
    }

    void ParseConstant(const std::string& type, std::istringstream& iss) const
    {
        std::string valueStr;
        std::getline(iss >> std::ws, valueStr);

        if (type == "number")
        {
            if (valueStr.find('.') != std::string::npos)
            {
                m_currentFunction->chunk->constants.emplace_back(std::stod(valueStr));
            }
            else
            {
                m_currentFunction->chunk->constants.emplace_back(std::stoll(valueStr));
            }
        }
        else if (type == "string")
        {
            if (valueStr.size() >= 2 && valueStr.front() == '"' && valueStr.back() == '"')
            {
                valueStr = valueStr.substr(1, valueStr.size() - 2);
            }
            valueStr = UnescapeString(valueStr);
            m_currentFunction->chunk->constants.emplace_back(std::make_shared<std::string>(valueStr));
        }
        else if (type == "true")
        {
            m_currentFunction->chunk->constants.emplace_back(true);
        }
        else if (type == "false")
        {
            m_currentFunction->chunk->constants.emplace_back(false);
        }
    }

    void ParseInstruction(const std::string& token, std::istringstream& iss)
    {
        if (token.back() == ':')
        {
            m_labels[token.substr(0, token.size() - 1)] = static_cast<uint16_t>(m_currentFunction->chunk->code.size());
            return;
        }

        auto& code = m_currentFunction->chunk->code;

        if (token == "const" || token == "set_local" || token == "get_local" || token == "get_global")
        {
            int arg; iss >> arg;
            uint8_t op = (token == "const") ? OP_CONSTANT :
                         (token == "get_global") ? OP_GET_GLOBAL :
                         (token == "set_local") ? OP_SET_LOCAL : OP_GET_LOCAL;
            code.push_back(op);
            code.push_back(static_cast<uint8_t>(arg));
        }
        else if (token == "call")
        {
            int argc;
            iss >> argc;
            code.push_back(OP_CALL);
            code.push_back(static_cast<uint8_t>(argc));
        }
        else if (token == "jmp" || token == "jmp_false")
        {
            std::string label; iss >> label;
            code.push_back(token == "jmp" ? OP_JUMP : OP_JUMP_IF_FALSE);
            m_unresolvedJumps.emplace_back(code.size(), label);
            code.push_back(0); code.push_back(0);
        }
        else if (token == "add")
        {
            code.push_back(OP_ADD);
        }
        else if (token == "sub")
        {
            code.push_back(OP_SUB);
        }
        else if (token == "mul")
        {
            code.push_back(OP_MUL);
        }
        else if (token == "div")
        {
            code.push_back(OP_DIV);
        }
        else if (token == "mod")
        {
            code.push_back(OP_MOD);
        }
        else if (token == "clt")
        {
            code.push_back(OP_LESS);
        }
        else if (token == "cgt")
        {
            code.push_back(OP_GREATER);
        }
        else if (token == "ceq")
        {
            code.push_back(OP_EQUAL);
        }
        else if (token == "build_array")
        {
            code.push_back(OP_BUILD_ARRAY);
        }
        else if (token == "array_push")
        {
            code.push_back(OP_ARRAY_PUSH);
        }
        else if (token == "array_len")
        {
            code.push_back(OP_ARRAY_LEN);
        }
        else if (token == "get_index")
        {
            code.push_back(OP_GET_INDEX);
        }
        else if (token == "set_index")
        {
            code.push_back(OP_SET_INDEX);
        }
        else if (token == "pop")
        {
            code.push_back(OP_POP);
        }
        else if (token == "return")
        {
            code.push_back(OP_RETURN);
        }
    }

    void BackpatchJumps()
    {
        for (const auto& [offset, labelName] : m_unresolvedJumps)
        {
            uint16_t address = m_labels[labelName];
            m_currentFunction->chunk->code[offset] = (address >> 8) & 0xFF;
            m_currentFunction->chunk->code[offset + 1] = address & 0xFF;
        }
    }
};