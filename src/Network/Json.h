#pragma once
#include <string>
#include <stdexcept>
#include <nlohmann/json.hpp>
#include "../VirtualMachine/Value/Value.h"

using nlohmann::json;


inline Value JsonToValue(const json& j, KlassPtr objKlass) {
    if (j.is_null())    return Null{};
    if (j.is_boolean()) return j.get<bool>();
    if (j.is_number_integer()) return j.get<int64_t>();
    if (j.is_number_float())   return j.get<double>();
    if (j.is_string())  return std::make_shared<std::string>(j.get<std::string>());

    if (j.is_array()) {
        auto arr = std::make_shared<Array>();
        for (const auto& el : j)
            arr->values.push_back(JsonToValue(el, objKlass));
        return arr;
    }

    if (j.is_object()) {
        auto inst = std::make_shared<Instance>(objKlass);
        for (auto& [key, val] : j.items())
            inst->fields[key] = JsonToValue(val, objKlass);
        return inst;
    }

    return Null{};
}

inline Value JsonParseValue(const std::string& src, KlassPtr objKlass) {
    try {
        return JsonToValue(json::parse(src), objKlass);
    } catch (const json::parse_error& e) {
        throw std::runtime_error(std::string("JSON parse error: ") + e.what());
    }
}


inline json ValueToJson(const Value& val) {
    if (std::holds_alternative<Null>(val))    return nullptr;
    if (std::holds_alternative<bool>(val))    return std::get<bool>(val);
    if (std::holds_alternative<int64_t>(val)) return std::get<int64_t>(val);
    if (std::holds_alternative<double>(val))  return std::get<double>(val);
    if (std::holds_alternative<StringPtr>(val)) {
        auto s = std::get<StringPtr>(val);
        return s ? *s : "";
    }
    if (std::holds_alternative<ArrayPtr>(val)) {
        json arr = json::array();
        for (const auto& el : std::get<ArrayPtr>(val)->values)
            arr.push_back(ValueToJson(el));
        return arr;
    }
    if (std::holds_alternative<InstancePtr>(val)) {
        json obj = json::object();
        for (const auto& [k, v] : std::get<InstancePtr>(val)->fields)
            obj[k] = ValueToJson(v);
        return obj;
    }
    return nullptr;
}

inline std::string JsonEncodeValue(const Value& val) {
    return ValueToJson(val).dump();
}
