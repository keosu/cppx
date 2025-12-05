import std;
import cppx.serialization;
import cppx.logging;

#include "../src/serialization_macros.h"

using namespace cppx;

// 测试用结构体
struct Point {
    int x;
    int y;
};
CPPX_SERIALIZABLE_2(Point, x, y)

struct Person {
    std::string name;
    int age;
    std::vector<std::string> hobbies;
};
CPPX_SERIALIZABLE_3(Person, name, age, hobbies)

enum class Priority {
    Low,
    Medium,
    High
};
CPPX_ENUM_SERIALIZABLE_3(Priority, Low, Medium, High)

struct Task {
    std::string title;
    Priority priority;
    std::optional<std::string> assignee;
};
CPPX_SERIALIZABLE_3(Task, title, priority, assignee)

// ============================================================================
// Helper functions
// ============================================================================

#define assert(condition) \
    do { \
        if (!(condition)) { \
            logger->error("❌ Assertion failed: {} at {}:{}", #condition, __FILE__, __LINE__); \
            throw std::runtime_error("Assertion failed: " #condition); \
        } \
    } while (0)

#define TEST_SECTION(name) \
    logger->info("  📝 {}", name);

// ============================================================================
// 测试用例
// ============================================================================

void test_json_basic_types(std::shared_ptr<logger> logger) {
    logger->info("🧪 Test: JSON basic types");
    
    TEST_SECTION("null");
    {
        json j = nullptr;
        assert(j.is_null());
        assert(j.dump() == "null");
    }
    
    TEST_SECTION("boolean");
    {
        json j_true = true;
        json j_false = false;
        assert(j_true.is_bool());
        assert(j_true.as_bool() == true);
        assert(j_true.dump() == "true");
        assert(j_false.dump() == "false");
    }
    
    TEST_SECTION("number");
    {
        json j_int = 42;
        json j_float = 3.14;
        assert(j_int.is_number());
        assert(j_int.as_int() == 42);
        assert(j_int.dump() == "42");
        assert(j_float.is_number());
    }
    
    TEST_SECTION("string");
    {
        json j = "hello world";
        assert(j.is_string());
        assert(j.as_string() == "hello world");
        assert(j.dump() == "\"hello world\"");
    }
    
    logger->info("  ✓ All basic type tests passed\n");
}

void test_json_array(std::shared_ptr<logger> logger) {
    logger->info("🧪 Test: JSON array");
    
    TEST_SECTION("empty array");
    {
        json j = json::array();
        assert(j.is_array());
        assert(j.size() == 0);
        assert(j.dump() == "[]");
    }
    
    TEST_SECTION("array with elements");
    {
        json j = json::array();
        j.push_back(1);
        j.push_back(2);
        j.push_back(3);
        
        assert(j.size() == 3);
        assert(j[0].as_int() == 1);
        assert(j[1].as_int() == 2);
        assert(j[2].as_int() == 3);
        assert(j.dump() == "[1,2,3]");
    }
    
    logger->info("  ✓ All array tests passed\n");
}

void test_json_object(std::shared_ptr<logger> logger) {
    logger->info("🧪 Test: JSON object");
    
    TEST_SECTION("empty object");
    {
        json j = json::object();
        assert(j.is_object());
        assert(j.size() == 0);
        assert(j.dump() == "{}");
    }
    
    TEST_SECTION("object with fields");
    {
        json j = json::object();
        j["name"] = "Alice";
        j["age"] = 30;
        
        assert(j.contains("name"));
        assert(j.contains("age"));
        assert(j["name"].as_string() == "Alice");
        assert(j["age"].as_int() == 30);
    }
    
    logger->info("  ✓ All object tests passed\n");
}

void test_json_parsing(std::shared_ptr<logger> logger) {
    logger->info("🧪 Test: JSON parsing");
    
    TEST_SECTION("parse null");
    {
        auto j = json::parse("null");
        assert(j.is_null());
    }
    
    TEST_SECTION("parse boolean");
    {
        auto j1 = json::parse("true");
        auto j2 = json::parse("false");
        assert(j1.as_bool() == true);
        assert(j2.as_bool() == false);
    }
    
    TEST_SECTION("parse number");
    {
        auto j1 = json::parse("42");
        auto j2 = json::parse("3.14");
        auto j3 = json::parse("-10");
        assert(j1.as_int() == 42);
        assert(j2.as_number() > 3.13 && j2.as_number() < 3.15);
        assert(j3.as_int() == -10);
    }
    
    TEST_SECTION("parse string");
    {
        auto j = json::parse("\"hello world\"");
        assert(j.as_string() == "hello world");
    }
    
    TEST_SECTION("parse array");
    {
        auto j = json::parse("[1, 2, 3, 4, 5]");
        assert(j.is_array());
        assert(j.size() == 5);
        assert(j[0].as_int() == 1);
        assert(j[4].as_int() == 5);
    }
    
    TEST_SECTION("parse object");
    {
        auto j = json::parse(R"({"name": "Alice", "age": 30})");
        assert(j.is_object());
        assert(j["name"].as_string() == "Alice");
        assert(j["age"].as_int() == 30);
    }
    
    logger->info("  ✓ All parsing tests passed\n");
}

void test_serialize_basic_types(std::shared_ptr<logger> logger) {
    logger->info("🧪 Test: Serialize basic types");
    
    TEST_SECTION("int");
    {
        int value = 42;
        auto json_str = to_json_string(value);
        assert(json_str == "42");
        
        auto loaded = from_json_string<int>(json_str);
        assert(loaded == 42);
    }
    
    TEST_SECTION("string");
    {
        std::string value = "hello";
        auto json_str = to_json_string(value);
        assert(json_str == "\"hello\"");
        
        auto loaded = from_json_string<std::string>(json_str);
        assert(loaded == "hello");
    }
    
    TEST_SECTION("vector");
    {
        std::vector<int> value = {1, 2, 3, 4, 5};
        auto json_str = to_json_string(value);
        assert(json_str == "[1,2,3,4,5]");
        
        auto loaded = from_json_string<std::vector<int>>(json_str);
        assert(loaded.size() == 5);
        assert(loaded[0] == 1);
        assert(loaded[4] == 5);
    }
    
    logger->info("  ✓ All basic type serialization tests passed\n");
}

void test_serialize_simple_struct(std::shared_ptr<logger> logger) {
    logger->info("🧪 Test: Serialize simple struct");
    
    TEST_SECTION("Point struct");
    {
        Point p{10, 20};
        auto json_str = to_json_string(p);
        
        logger->info("    Point JSON: {}", json_str);
        
        auto loaded = from_json_string<Point>(json_str);
        assert(loaded.x == 10);
        assert(loaded.y == 20);
    }
    
    logger->info("  ✓ Simple struct serialization passed\n");
}

void test_serialize_complex_struct(std::shared_ptr<logger> logger) {
    logger->info("🧪 Test: Serialize complex struct");
    
    TEST_SECTION("Person struct");
    {
        Person p{
            .name = "Alice",
            .age = 30,
            .hobbies = {"reading", "coding", "gaming"}
        };
        
        auto json_str = to_json_string(p, 2);
        logger->info("    Person JSON:\n{}", json_str);
        
        auto loaded = from_json_string<Person>(json_str);
        assert(loaded.name == "Alice");
        assert(loaded.age == 30);
        assert(loaded.hobbies.size() == 3);
        assert(loaded.hobbies[0] == "reading");
    }
    
    logger->info("  ✓ Complex struct serialization passed\n");
}

void test_serialize_enum(std::shared_ptr<logger> logger) {
    logger->info("🧪 Test: Serialize enum");
    
    TEST_SECTION("Priority enum");
    {
        Priority p = Priority::High;
        auto json_str = to_json_string(p);
        assert(json_str == "\"High\"");
        
        auto loaded = from_json_string<Priority>(json_str);
        assert(loaded == Priority::High);
    }
    
    logger->info("  ✓ Enum serialization passed\n");
}

void test_serialize_optional(std::shared_ptr<logger> logger) {
    logger->info("🧪 Test: Serialize optional");
    
    TEST_SECTION("Task with optional");
    {
        Task task1{
            .title = "Task 1",
            .priority = Priority::High,
            .assignee = "Alice"
        };
        
        Task task2{
            .title = "Task 2",
            .priority = Priority::Low,
            .assignee = std::nullopt
        };
        
        auto json1 = to_json_string(task1, 2);
        auto json2 = to_json_string(task2, 2);
        
        logger->info("    Task1 JSON:\n{}", json1);
        logger->info("    Task2 JSON:\n{}", json2);
        
        auto loaded1 = from_json_string<Task>(json1);
        assert(loaded1.assignee.has_value());
        assert(loaded1.assignee.value() == "Alice");
        
        auto loaded2 = from_json_string<Task>(json2);
        assert(!loaded2.assignee.has_value());
    }
    
    logger->info("  ✓ Optional serialization passed\n");
}

void test_file_io(std::shared_ptr<logger> logger) {
    logger->info("🧪 Test: File I/O");
    
    TEST_SECTION("save and load json");
    {
        Person person{
            .name = "Test Person",
            .age = 25,
            .hobbies = {"test1", "test2"}
        };
        
        const char* filename = "test_person_simple.json";
        
        // 保存
        save_json(filename, person);
        logger->info("    ✓ Saved to {}", filename);
        
        // 加载
        auto loaded = load_json<Person>(filename);
        assert(loaded.name == "Test Person");
        assert(loaded.age == 25);
        assert(loaded.hobbies.size() == 2);
        
        logger->info("    ✓ Loaded from {}", filename);
        
        // 清理
        std::filesystem::remove(filename);
    }
    
    logger->info("  ✓ File I/O tests passed\n");
}

// ============================================================================
// 主函数
// ============================================================================

int main() {
    auto logger = get_logger("test_serialization");
    logger->info("╔═══════════════════════════════════════════════════╗");
    logger->info("║  🧪 cppx.serialization Tests                     ║");
    logger->info("╚═══════════════════════════════════════════════════╝\n");
    
    int passed = 0;
    int failed = 0;
    
    try {
        test_json_basic_types(logger);
        passed++;
    } catch (const std::exception& e) {
        logger->error("❌ Test failed: {}", e.what());
        failed++;
    }
    
    try {
        test_json_array(logger);
        passed++;
    } catch (const std::exception& e) {
        logger->error("❌ Test failed: {}", e.what());
        failed++;
    }
    
    try {
        test_json_object(logger);
        passed++;
    } catch (const std::exception& e) {
        logger->error("❌ Test failed: {}", e.what());
        failed++;
    }
    
    try {
        test_json_parsing(logger);
        passed++;
    } catch (const std::exception& e) {
        logger->error("❌ Test failed: {}", e.what());
        failed++;
    }
    
    try {
        test_serialize_basic_types(logger);
        passed++;
    } catch (const std::exception& e) {
        logger->error("❌ Test failed: {}", e.what());
        failed++;
    }
    
    try {
        test_serialize_simple_struct(logger);
        passed++;
    } catch (const std::exception& e) {
        logger->error("❌ Test failed: {}", e.what());
        failed++;
    }
    
    try {
        test_serialize_complex_struct(logger);
        passed++;
    } catch (const std::exception& e) {
        logger->error("❌ Test failed: {}", e.what());
        failed++;
    }
    
    try {
        test_serialize_enum(logger);
        passed++;
    } catch (const std::exception& e) {
        logger->error("❌ Test failed: {}", e.what());
        failed++;
    }
    
    try {
        test_serialize_optional(logger);
        passed++;
    } catch (const std::exception& e) {
        logger->error("❌ Test failed: {}", e.what());
        failed++;
    }
    
    try {
        test_file_io(logger);
        passed++;
    } catch (const std::exception& e) {
        logger->error("❌ Test failed: {}", e.what());
        failed++;
    }
    
    // 总结
    logger->info("╔═══════════════════════════════════════════════════╗");
    logger->info("║  📊 Test Summary                                 ║");
    logger->info("╠═══════════════════════════════════════════════════╣");
    logger->info("║  Total:  {:>2} tests                                ║", passed + failed);
    logger->info("║  ✓ Pass: {:>2} tests                                ║", passed);
    logger->info("║  ✗ Fail: {:>2} tests                                ║", failed);
    logger->info("╚═══════════════════════════════════════════════════╝");
    
    if (failed == 0) {
        logger->info("\n🎉 All tests passed!");
    } else {
        logger->error("\n❌ {} test(s) failed", failed);
    }
    
    return failed;
}

