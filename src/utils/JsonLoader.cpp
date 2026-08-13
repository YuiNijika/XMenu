#include "JsonLoader.h"
#include "utils/Log.h"
#include <XBase/Platform.h>
#include <sstream>

namespace JsonLoader {
namespace {
    // 跳过空白字符
    void SkipWhitespace(const std::string& json, size_t& pos) {
        while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\n' || json[pos] == '\r' || json[pos] == '\t')) {
            ++pos;
        }
    }
    
    // 解析字符串
    std::string ParseString(const std::string& json, size_t& pos) {
        if (pos >= json.size() || json[pos] != '"') {
            return "";
        }
        ++pos; // 跳过开头的"
        
        std::string result;
        while (pos < json.size() && json[pos] != '"') {
            if (json[pos] == '\\' && pos + 1 < json.size()) {
                ++pos;
                switch (json[pos]) {
                    case '"': result += '"'; break;
                    case '\\': result += '\\'; break;
                    case '/': result += '/'; break;
                    case 'n': result += '\n'; break;
                    case 'r': result += '\r'; break;
                    case 't': result += '\t'; break;
                    default: result += json[pos]; break;
                }
            } else {
                result += json[pos];
            }
            ++pos;
        }
        
        if (pos < json.size()) {
            ++pos; // 跳过结尾的"
        }
        
        return result;
    }
    
    // 解析数字
    double ParseNumber(const std::string& json, size_t& pos) {
        size_t start = pos;
        bool is_float = false;
        
        if (pos < json.size() && json[pos] == '-') {
            ++pos;
        }
        
        while (pos < json.size() && (isdigit(json[pos]) || json[pos] == '.')) {
            if (json[pos] == '.') {
                is_float = true;
            }
            ++pos;
        }
        
        std::string num_str = json.substr(start, pos - start);
        return std::stod(num_str);
    }
    
    // 解析值
    JsonValue ParseValue(const std::string& json, size_t& pos) {
        SkipWhitespace(json, pos);
        
        if (pos >= json.size()) {
            return JsonValue();
        }
        
        JsonValue value;
        char current = json[pos];
        
        if (current == '"') {
            // 字符串
            value.type = JsonValue::STRING;
            value.string_value = ParseString(json, pos);
        } else if (current == '{') {
            // 对象
            value.type = JsonValue::OBJECT;
            ++pos; // 跳过{
            
            SkipWhitespace(json, pos);
            if (pos < json.size() && json[pos] == '}') {
                ++pos;
                return value;
            }
            
            while (pos < json.size()) {
                SkipWhitespace(json, pos);
                std::string key = ParseString(json, pos);
                
                SkipWhitespace(json, pos);
                if (pos < json.size() && json[pos] == ':') {
                    ++pos;
                }
                
                value.object_values[key] = ParseValue(json, pos);
                
                SkipWhitespace(json, pos);
                if (pos < json.size() && json[pos] == ',') {
                    ++pos;
                } else if (pos < json.size() && json[pos] == '}') {
                    ++pos;
                    break;
                }
            }
        } else if (current == '[') {
            // 数组
            value.type = JsonValue::ARRAY;
            ++pos; // 跳过[
            
            SkipWhitespace(json, pos);
            if (pos < json.size() && json[pos] == ']') {
                ++pos;
                return value;
            }
            
            while (pos < json.size()) {
                value.array_values.push_back(ParseValue(json, pos));
                
                SkipWhitespace(json, pos);
                if (pos < json.size() && json[pos] == ',') {
                    ++pos;
                } else if (pos < json.size() && json[pos] == ']') {
                    ++pos;
                    break;
                }
            }
        } else if (current == 't' || current == 'f') {
            // 布尔值
            value.type = JsonValue::BOOL;
            if (json.substr(pos, 4) == "true") {
                value.bool_value = true;
                pos += 4;
            } else if (json.substr(pos, 5) == "false") {
                value.bool_value = false;
                pos += 5;
            }
        } else if (current == 'n') {
            // null
            value.type = JsonValue::NULL_VALUE;
            pos += 4;
        } else if (isdigit(current) || current == '-') {
            // 数字
            value.type = JsonValue::NUMBER;
            value.number_value = ParseNumber(json, pos);
        }
        
        return value;
    }
}

JsonValue Parse(const std::string& json) {
    size_t pos = 0;
    return ParseValue(json, pos);
}

JsonValue LoadFromFile(const std::string& filepath) {
    std::string content;
    if (!XBase::Platform::ReadTextFile(filepath, content)) {
        Log::Warn(std::string("无法打开JSON文件: ") + filepath);
        return JsonValue();
    }
    return Parse(content);
}

JsonValue LoadFromResource(int resourceId) {
    std::string content;
    if (!XBase::Platform::ReadModuleResource(resourceId, content)) {
        Log::Warn(std::string("无法加载资源ID: ") + std::to_string(resourceId));
        return JsonValue();
    }
    return Parse(content);
}

std::string GetString(const JsonValue& value, const std::string& key, const std::string& default_value) {
    auto it = value.object_values.find(key);
    if (it != value.object_values.end() && it->second.type == JsonValue::STRING) {
        return it->second.string_value;
    }
    return default_value;
}

double GetNumber(const JsonValue& value, const std::string& key, double default_value) {
    auto it = value.object_values.find(key);
    if (it != value.object_values.end() && it->second.type == JsonValue::NUMBER) {
        return it->second.number_value;
    }
    return default_value;
}

bool GetBool(const JsonValue& value, const std::string& key, bool default_value) {
    auto it = value.object_values.find(key);
    if (it != value.object_values.end() && it->second.type == JsonValue::BOOL) {
        return it->second.bool_value;
    }
    return default_value;
}

const JsonValue& GetObject(const JsonValue& value, const std::string& key) {
    static JsonValue empty;
    auto it = value.object_values.find(key);
    if (it != value.object_values.end() && it->second.type == JsonValue::OBJECT) {
        return it->second;
    }
    return empty;
}

const std::vector<JsonValue>& GetArray(const JsonValue& value, const std::string& key) {
    static std::vector<JsonValue> empty;
    auto it = value.object_values.find(key);
    if (it != value.object_values.end() && it->second.type == JsonValue::ARRAY) {
        return it->second.array_values;
    }
    return empty;
}

} // namespace JsonLoader
