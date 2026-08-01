#pragma once

#include <cstddef>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace XBase::Json {

struct Value {
    enum Type { Null, Bool, Number, String, Array, Object };

    Type type = Null;
    std::variant<std::monostate, bool, double, std::string, std::vector<Value>, std::unordered_map<std::string, Value>> data;

    Value() : type(Null) {}
    Value(bool v) : type(Bool), data(v) {}
    Value(double v) : type(Number), data(v) {}
    Value(int v) : type(Number), data(static_cast<double>(v)) {}
    Value(const char* v) : type(String), data(std::string(v)) {}
    Value(const std::string& v) : type(String), data(v) {}

    bool IsNull() const { return type == Null; }
    bool IsBool() const { return type == Bool; }
    bool IsNumber() const { return type == Number; }
    bool IsString() const { return type == String; }
    bool IsArray() const { return type == Array; }
    bool IsObject() const { return type == Object; }

    bool AsBool(bool def = false) const;
    double AsNumber(double def = 0.0) const;
    int AsInt(int def = 0) const;
    std::string AsString(const std::string& def = "") const;

    const Value& operator[](const std::string& key) const;
    const Value& operator[](std::size_t index) const;

    Value& Set(const std::string& key, const Value& val);
    void Push(const Value& val);

    std::string Serialize(bool pretty = true, int indent = 0) const;

    static Value Parse(const std::string& text);
    static Value Load(const std::string& filePath);
    static bool Save(const Value& val, const std::string& filePath);
};

} // namespace XBase::Json