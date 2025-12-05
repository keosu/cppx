import std;
import cppx.serialization;
import cppx.logging;

using namespace cppx;

int main() {
    auto logger = get_logger("serialization_simple_demo");
    logger->info("=== cppx.serialization Simple Demo ===\n");
    
    // ========================================================================
    // 1. JSON 基础操作
    // ========================================================================
    logger->info("1. JSON basic operations:");
    {
        // 创建 JSON 对象
        json j = json::object();
        j["name"] = "Alice";
        j["age"] = 30;
        j["active"] = true;
        
        logger->info("   JSON object: {}", j.dump());
        logger->info("   Pretty JSON:\n{}", j.dump(2));
    }
    logger->info("");
    
    // ========================================================================
    // 2. JSON 数组
    // ========================================================================
    logger->info("2. JSON arrays:");
    {
        json arr = json::array();
        arr.push_back(1);
        arr.push_back(2);
        arr.push_back(3);
        
        logger->info("   Array: {}", arr.dump());
    }
    logger->info("");
    
    // ========================================================================
    // 3. JSON 解析
    // ========================================================================
    logger->info("3. JSON parsing:");
    {
        std::string json_str = R"({"name": "Bob", "age": 25, "scores": [95, 87, 92]})";
        auto j = json::parse(json_str);
        
        logger->info("   Parsed name: {}", j["name"].as_string());
        logger->info("   Parsed age: {}", j["age"].as_int());
        logger->info("   First score: {}", j["scores"][0].as_int());
    }
    logger->info("");
    
    // ========================================================================
    // 4. 基础类型序列化
    // ========================================================================
    logger->info("4. Basic type serialization:");
    {
        int num = 42;
        logger->info("   int: {}", to_json_string(num));
        
        std::string text = "hello world";
        logger->info("   string: {}", to_json_string(text));
        
        std::vector<int> numbers = {1, 2, 3, 4, 5};
        logger->info("   vector: {}", to_json_string(numbers));
        
        std::map<std::string, int> scores = {{"math", 95}, {"english", 87}};
        logger->info("   map: {}", to_json_string(scores));
    }
    logger->info("");
    
    // ========================================================================
    // 5. optional 类型
    // ========================================================================
    logger->info("5. Optional type:");
    {
        std::optional<int> has_value = 42;
        std::optional<int> no_value;
        
        logger->info("   optional (has value): {}", to_json_string(has_value));
        logger->info("   optional (empty): {}", to_json_string(no_value));
    }
    logger->info("");
    
    // ========================================================================
    // 6. 嵌套结构
    // ========================================================================
    logger->info("6. Nested JSON:");
    {
        json person = json::object();
        person["name"] = "Charlie";
        person["age"] = 28;
        
        json address = json::object();
        address["city"] = "New York";
        address["country"] = "USA";
        person["address"] = address;
        
        json hobbies = json::array();
        hobbies.push_back("reading");
        hobbies.push_back("coding");
        hobbies.push_back("gaming");
        person["hobbies"] = hobbies;
        
        logger->info("   Nested JSON:\n{}", person.dump(2));
    }
    logger->info("");
    
    // ========================================================================
    // 7. 文件 I/O
    // ========================================================================
    logger->info("7. File I/O:");
    {
        std::vector<std::string> data = {"one", "two", "three"};
        
        const char* filename = "test_data.json";
        save_json(filename, data);
        logger->info("   ✓ Saved to {}", filename);
        
        auto loaded = load_json<std::vector<std::string>>(filename);
        logger->info("   ✓ Loaded {} items", loaded.size());
        
        // 清理
        std::filesystem::remove(filename);
    }
    logger->info("");
    
    logger->info("=== Demo completed ===");
    return 0;
}

