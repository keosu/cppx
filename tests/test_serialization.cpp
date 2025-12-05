import std;
import cppx.serialization;
import cppx.logging;
import cppx.test;

#include "../src/test_macros.h"
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

struct Address {
    std::string street;
    std::string city;
    std::string country;
};
CPPX_SERIALIZABLE_3(Address, street, city, country)

struct Company {
    std::string name;
    Address headquarters;
    std::vector<Person> employees;
};
CPPX_SERIALIZABLE_3(Company, name, headquarters, employees)

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
// 测试用例
// ============================================================================

TEST_CASE("json basic types") {
    auto logger = get_logger("test_serialization");
    
    SECTION("null") {
        json j = nullptr;
        REQUIRE(j.is_null());
        REQUIRE_EQ(j.dump(), "null");
    }
    
    SECTION("boolean") {
        json j_true = true;
        json j_false = false;
        REQUIRE(j_true.is_bool());
        REQUIRE_EQ(j_true.as_bool(), true);
        REQUIRE_EQ(j_true.dump(), "true");
        REQUIRE_EQ(j_false.dump(), "false");
    }
    
    SECTION("number") {
        json j_int = 42;
        json j_float = 3.14;
        REQUIRE(j_int.is_number());
        REQUIRE_EQ(j_int.as_int(), 42);
        REQUIRE_EQ(j_int.dump(), "42");
        REQUIRE(j_float.is_number());
    }
    
    SECTION("string") {
        json j = "hello world";
        REQUIRE(j.is_string());
        REQUIRE_EQ(j.as_string(), "hello world");
        REQUIRE_EQ(j.dump(), "\"hello world\"");
    }
}

TEST_CASE("json array") {
    auto logger = get_logger("test_serialization");
    
    SECTION("empty array") {
        json j = json::array();
        REQUIRE(j.is_array());
        REQUIRE_EQ(j.size(), 0);
        REQUIRE_EQ(j.dump(), "[]");
    }
    
    SECTION("array with elements") {
        json j = json::array();
        j.push_back(1);
        j.push_back(2);
        j.push_back(3);
        
        REQUIRE_EQ(j.size(), 3);
        REQUIRE_EQ(j[0].as_int(), 1);
        REQUIRE_EQ(j[1].as_int(), 2);
        REQUIRE_EQ(j[2].as_int(), 3);
        REQUIRE_EQ(j.dump(), "[1,2,3]");
    }
}

TEST_CASE("json object") {
    auto logger = get_logger("test_serialization");
    
    SECTION("empty object") {
        json j = json::object();
        REQUIRE(j.is_object());
        REQUIRE_EQ(j.size(), 0);
        REQUIRE_EQ(j.dump(), "{}");
    }
    
    SECTION("object with fields") {
        json j = json::object();
        j["name"] = "Alice";
        j["age"] = 30;
        
        REQUIRE(j.contains("name"));
        REQUIRE(j.contains("age"));
        REQUIRE_EQ(j["name"].as_string(), "Alice");
        REQUIRE_EQ(j["age"].as_int(), 30);
    }
}

TEST_CASE("json parsing") {
    auto logger = get_logger("test_serialization");
    
    SECTION("parse null") {
        auto j = json::parse("null");
        REQUIRE(j.is_null());
    }
    
    SECTION("parse boolean") {
        auto j1 = json::parse("true");
        auto j2 = json::parse("false");
        REQUIRE_EQ(j1.as_bool(), true);
        REQUIRE_EQ(j2.as_bool(), false);
    }
    
    SECTION("parse number") {
        auto j1 = json::parse("42");
        auto j2 = json::parse("3.14");
        auto j3 = json::parse("-10");
        REQUIRE_EQ(j1.as_int(), 42);
        REQUIRE(j2.as_number() > 3.13 && j2.as_number() < 3.15);
        REQUIRE_EQ(j3.as_int(), -10);
    }
    
    SECTION("parse string") {
        auto j = json::parse("\"hello world\"");
        REQUIRE_EQ(j.as_string(), "hello world");
    }
    
    SECTION("parse array") {
        auto j = json::parse("[1, 2, 3, 4, 5]");
        REQUIRE(j.is_array());
        REQUIRE_EQ(j.size(), 5);
        REQUIRE_EQ(j[0].as_int(), 1);
        REQUIRE_EQ(j[4].as_int(), 5);
    }
    
    SECTION("parse object") {
        auto j = json::parse(R"({"name": "Alice", "age": 30})");
        REQUIRE(j.is_object());
        REQUIRE_EQ(j["name"].as_string(), "Alice");
        REQUIRE_EQ(j["age"].as_int(), 30);
    }
    
    SECTION("parse nested") {
        auto j = json::parse(R"({
            "name": "Alice",
            "scores": [95, 87, 92],
            "address": {
                "city": "New York",
                "country": "USA"
            }
        })");
        
        REQUIRE(j.is_object());
        REQUIRE_EQ(j["name"].as_string(), "Alice");
        REQUIRE(j["scores"].is_array());
        REQUIRE_EQ(j["scores"].size(), 3);
        REQUIRE(j["address"].is_object());
        REQUIRE_EQ(j["address"]["city"].as_string(), "New York");
    }
}

TEST_CASE("serialize basic types") {
    auto logger = get_logger("test_serialization");
    
    SECTION("int") {
        int value = 42;
        auto json_str = to_json_string(value);
        REQUIRE_EQ(json_str, "42");
        
        auto loaded = from_json_string<int>(json_str);
        REQUIRE_EQ(loaded, 42);
    }
    
    SECTION("string") {
        std::string value = "hello";
        auto json_str = to_json_string(value);
        REQUIRE_EQ(json_str, "\"hello\"");
        
        auto loaded = from_json_string<std::string>(json_str);
        REQUIRE_EQ(loaded, "hello");
    }
    
    SECTION("vector") {
        std::vector<int> value = {1, 2, 3, 4, 5};
        auto json_str = to_json_string(value);
        REQUIRE_EQ(json_str, "[1,2,3,4,5]");
        
        auto loaded = from_json_string<std::vector<int>>(json_str);
        REQUIRE_EQ(loaded.size(), 5);
        REQUIRE_EQ(loaded[0], 1);
        REQUIRE_EQ(loaded[4], 5);
    }
}

TEST_CASE("serialize simple struct") {
    auto logger = get_logger("test_serialization");
    
    SECTION("Point struct") {
        Point p{10, 20};
        auto json_str = to_json_string(p);
        
        logger->info("Point JSON: {}", json_str);
        
        auto loaded = from_json_string<Point>(json_str);
        REQUIRE_EQ(loaded.x, 10);
        REQUIRE_EQ(loaded.y, 20);
    }
}

TEST_CASE("serialize complex struct") {
    auto logger = get_logger("test_serialization");
    
    SECTION("Person struct") {
        Person p{
            .name = "Alice",
            .age = 30,
            .hobbies = {"reading", "coding", "gaming"}
        };
        
        auto json_str = to_json_string(p, 2);
        logger->info("Person JSON:\n{}", json_str);
        
        auto loaded = from_json_string<Person>(json_str);
        REQUIRE_EQ(loaded.name, "Alice");
        REQUIRE_EQ(loaded.age, 30);
        REQUIRE_EQ(loaded.hobbies.size(), 3);
        REQUIRE_EQ(loaded.hobbies[0], "reading");
    }
}

TEST_CASE("serialize nested struct") {
    auto logger = get_logger("test_serialization");
    
    SECTION("Company struct") {
        Company company{
            .name = "ACME Corp",
            .headquarters = {"123 Main St", "New York", "USA"},
            .employees = {
                {"Alice", 30, {"coding"}},
                {"Bob", 25, {"design"}}
            }
        };
        
        auto json_str = to_json_string(company, 2);
        logger->info("Company JSON:\n{}", json_str);
        
        auto loaded = from_json_string<Company>(json_str);
        REQUIRE_EQ(loaded.name, "ACME Corp");
        REQUIRE_EQ(loaded.headquarters.city, "New York");
        REQUIRE_EQ(loaded.employees.size(), 2);
        REQUIRE_EQ(loaded.employees[0].name, "Alice");
    }
}

TEST_CASE("serialize enum") {
    auto logger = get_logger("test_serialization");
    
    SECTION("Priority enum") {
        Priority p = Priority::High;
        auto json_str = to_json_string(p);
        REQUIRE_EQ(json_str, "\"High\"");
        
        auto loaded = from_json_string<Priority>(json_str);
        REQUIRE(loaded == Priority::High);
    }
}

TEST_CASE("serialize optional") {
    auto logger = get_logger("test_serialization");
    
    SECTION("Task with optional") {
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
        
        logger->info("Task1 JSON:\n{}", json1);
        logger->info("Task2 JSON:\n{}", json2);
        
        auto loaded1 = from_json_string<Task>(json1);
        REQUIRE(loaded1.assignee.has_value());
        REQUIRE_EQ(loaded1.assignee.value(), "Alice");
        
        auto loaded2 = from_json_string<Task>(json2);
        REQUIRE(!loaded2.assignee.has_value());
    }
}

TEST_CASE("serialize map") {
    auto logger = get_logger("test_serialization");
    
    SECTION("string to int map") {
        std::map<std::string, int> scores = {
            {"Alice", 95},
            {"Bob", 87},
            {"Charlie", 92}
        };
        
        auto json_str = to_json_string(scores, 2);
        logger->info("Scores JSON:\n{}", json_str);
        
        auto loaded = from_json_string<std::map<std::string, int>>(json_str);
        REQUIRE_EQ(loaded.size(), 3);
        REQUIRE_EQ(loaded["Alice"], 95);
        REQUIRE_EQ(loaded["Bob"], 87);
    }
}

TEST_CASE("file I/O") {
    auto logger = get_logger("test_serialization");
    
    SECTION("save and load json") {
        Person person{
            .name = "Test Person",
            .age = 25,
            .hobbies = {"test1", "test2"}
        };
        
        const char* filename = "test_person.json";
        
        // 保存
        save_json(filename, person);
        logger->info("✓ Saved to {}", filename);
        
        // 加载
        auto loaded = load_json<Person>(filename);
        REQUIRE_EQ(loaded.name, "Test Person");
        REQUIRE_EQ(loaded.age, 25);
        REQUIRE_EQ(loaded.hobbies.size(), 2);
        
        logger->info("✓ Loaded from {}", filename);
        
        // 清理
        std::filesystem::remove(filename);
    }
}

// ============================================================================
// 主函数
// ============================================================================

int main() {
    auto logger = get_logger("test_serialization");
    logger->info("=== Running cppx.serialization tests ===\n");
    
    auto& runner = test_runner::instance();
    int result = runner.run_all();
    
    logger->info("\n=== Test summary ===");
    logger->info("Total: {}", runner.total_tests());
    logger->info("Passed: {}", runner.passed_tests());
    logger->info("Failed: {}", runner.failed_tests());
    
    return result;
}

