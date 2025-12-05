// Preview of cppx.serialization usage (NOT YET IMPLEMENTED)
// This is a design preview - compile will fail until implementation is complete

import std;
import cppx.serialization;  // 待实现
import cppx.logging;

#include "../src/serialization_macros.h"  // 待实现

using namespace cppx;

// ============================================================================
// 示例 1: 简单结构体
// ============================================================================

struct Point {
    int x;
    int y;
};

// 注册序列化支持（一行搞定）
CPPX_SERIALIZABLE(Point, x, y);

// ============================================================================
// 示例 2: 包含各种类型的复杂结构
// ============================================================================

enum class Priority {
    Low,
    Medium,
    High
};
CPPX_ENUM_SERIALIZABLE(Priority, Low, Medium, High);

struct Person {
    std::string name;
    int age;
    std::vector<std::string> hobbies;
    std::optional<std::string> email;
    Priority priority;
    std::map<std::string, int> scores;
};

CPPX_SERIALIZABLE(Person, name, age, hobbies, email, priority, scores);

// ============================================================================
// 示例 3: 嵌套结构
// ============================================================================

struct Address {
    std::string street;
    std::string city;
    std::string country;
};
CPPX_SERIALIZABLE(Address, street, city, country);

struct Company {
    std::string name;
    Address headquarters;
    std::vector<Person> employees;
};
CPPX_SERIALIZABLE(Company, name, headquarters, employees);

// ============================================================================
// 主程序
// ============================================================================

int main() {
    auto logger = get_logger("serialization_demo");
    logger->info("=== cppx.serialization Demo ===\n");
    
    // ========================================================================
    // 1. 基础类型序列化
    // ========================================================================
    logger->info("1. Basic types:");
    {
        int num = 42;
        logger->info("   int to JSON: {}", to_json(num));
        
        std::string text = "hello world";
        logger->info("   string to JSON: {}", to_json(text));
        
        std::vector<int> numbers = {1, 2, 3, 4, 5};
        logger->info("   vector to JSON: {}", to_json(numbers));
    }
    logger->info("");
    
    // ========================================================================
    // 2. 简单结构体
    // ========================================================================
    logger->info("2. Simple struct serialization:");
    {
        Point p{100, 200};
        
        // JSON 序列化
        auto json_str = to_json(p, 2);  // 2 空格缩进
        logger->info("   Point to JSON:\n{}", json_str);
        
        // JSON 反序列化
        auto result = from_json<Point>(R"({"x": 50, "y": 75})");
        if (result.is_ok()) {
            auto p2 = result.value();
            logger->info("   Deserialized: Point({}, {})", p2.x, p2.y);
        }
        
        // 二进制序列化
        auto binary = to_binary(p);
        logger->info("   Binary size: {} bytes", binary.size());
        
        // 二进制反序列化
        auto bin_result = from_binary<Point>(binary);
        if (bin_result.is_ok()) {
            logger->info("   ✓ Binary round-trip successful");
        }
    }
    logger->info("");
    
    // ========================================================================
    // 3. 复杂结构体
    // ========================================================================
    logger->info("3. Complex struct with various types:");
    {
        Person person{
            .name = "Alice",
            .age = 30,
            .hobbies = {"reading", "coding", "gaming"},
            .email = "alice@example.com",
            .priority = Priority::High,
            .scores = {{"math", 95}, {"english", 87}}
        };
        
        auto json_str = to_json(person, 2);
        logger->info("   Person to JSON:\n{}", json_str);
        
        // 保存到文件
        auto save_result = save_json("person.json", person);
        if (save_result.is_ok()) {
            logger->info("   ✓ Saved to person.json");
        }
        
        // 从文件加载
        auto load_result = load_json<Person>("person.json");
        if (load_result.is_ok()) {
            auto loaded = load_result.value();
            logger->info("   ✓ Loaded from person.json");
            logger->info("   Name: {}, Age: {}", loaded.name, loaded.age);
        }
    }
    logger->info("");
    
    // ========================================================================
    // 4. 嵌套结构
    // ========================================================================
    logger->info("4. Nested structs:");
    {
        Company company{
            .name = "ACME Corp",
            .headquarters = {"123 Main St", "New York", "USA"},
            .employees = {
                {"Alice", 30, {"coding"}, "alice@acme.com", Priority::High, {}},
                {"Bob", 25, {"design"}, std::nullopt, Priority::Medium, {}}
            }
        };
        
        auto json_str = to_json(company, 2);
        logger->info("   Company to JSON:\n{}", json_str);
    }
    logger->info("");
    
    // ========================================================================
    // 5. 容器类型
    // ========================================================================
    logger->info("5. Standard containers:");
    {
        // optional
        std::optional<int> has_value = 42;
        std::optional<int> no_value;
        logger->info("   optional (has value): {}", to_json(has_value));
        logger->info("   optional (empty): {}", to_json(no_value));
        
        // map
        std::map<std::string, std::string> dict = {
            {"key1", "value1"},
            {"key2", "value2"}
        };
        logger->info("   map: {}", to_json(dict));
    }
    logger->info("");
    
    // ========================================================================
    // 6. 错误处理
    // ========================================================================
    logger->info("6. Error handling:");
    {
        // 解析错误的 JSON
        auto result = from_json<Person>(R"({"invalid": "json"})");
        if (result.is_err()) {
            logger->info("   ✓ Detected invalid JSON: {}", result.error());
        }
        
        // 类型不匹配
        auto result2 = from_json<int>(R"("not a number")");
        if (result2.is_err()) {
            logger->info("   ✓ Detected type mismatch: {}", result2.error());
        }
    }
    logger->info("");
    
    logger->info("=== Demo completed ===");
    return 0;
}

