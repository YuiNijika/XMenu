#pragma once
#include <string>
#include <vector>
#include <unordered_map>

namespace JsonLoader {
    // 简化的JSON解析结果
    struct JsonValue {
        enum Type {
            STRING,
            NUMBER,
            OBJECT,
            ARRAY,
            BOOL,
            NULL_VALUE
        };
        
        Type type;
        std::string string_value;
        double number_value;
        bool bool_value;
        std::vector<JsonValue> array_values;
        std::unordered_map<std::string, JsonValue> object_values;
        
        JsonValue() : type(NULL_VALUE), number_value(0), bool_value(false) {}
    };
    
    // 解析JSON字符串
    JsonValue Parse(const std::string& json);
    
    // 从文件加载JSON
    JsonValue LoadFromFile(const std::string& filepath);
    
    // 从资源加载JSON
    JsonValue LoadFromResource(int resourceId);
    
    // 辅助函数
    std::string GetString(const JsonValue& value, const std::string& key, const std::string& default_value = "");
    double GetNumber(const JsonValue& value, const std::string& key, double default_value = 0.0);
    bool GetBool(const JsonValue& value, const std::string& key, bool default_value = false);
    const JsonValue& GetObject(const JsonValue& value, const std::string& key);
    const std::vector<JsonValue>& GetArray(const JsonValue& value, const std::string& key);
}
