# cppx.serialization - 设计方案

## 🎯 目标

实现一个现代化的 C++23 序列化/反序列化库，支持：
- ✅ 文本格式（JSON）
- ✅ 二进制格式（类似 MessagePack）
- ✅ 编译时反射（基于模板元编程）
- ✅ 运行时反射（可选）
- ✅ 简单易用
- ✅ 尽可能无侵入

## 📋 设计方案

### 方案对比

#### 方案 1: 宏注册（半侵入式）
```cpp
struct Person {
    std::string name;
    int age;
    std::vector<std::string> hobbies;
    
    // 使用宏注册字段
    CPPX_SERIALIZE(name, age, hobbies);
};
```

**优点**: 简单直接，编译时完全确定
**缺点**: 需要在类中添加宏（轻度侵入）

#### 方案 2: 外部注册（完全无侵入）⭐ 推荐
```cpp
// 类定义（完全不修改）
struct Person {
    std::string name;
    int age;
    std::vector<std::string> hobbies;
};

// 外部注册
namespace cppx {
    template<>
    struct serializer<Person> {
        static void to_json(json& j, const Person& p) {
            j = {
                {"name", p.name},
                {"age", p.age},
                {"hobbies", p.hobbies}
            };
        }
        
        static void from_json(const json& j, Person& p) {
            p.name = j["name"].as<std::string>();
            p.age = j["age"].as<int>();
            p.hobbies = j["hobbies"].as<std::vector<std::string>>();
        }
    };
}
```

**优点**: 完全无侵入，灵活
**缺点**: 需要为每个类型编写序列化代码

#### 方案 3: 混合方案（推荐实现）⭐⭐
```cpp
// 1. 对于简单结构，自动序列化
struct Point {
    int x;
    int y;
};
// 自动支持！使用聚合初始化反射

// 2. 对于复杂类型，提供辅助宏
struct Person {
    std::string name;
    int age;
    std::vector<std::string> hobbies;
};

// 使用辅助宏一行搞定
CPPX_SERIALIZABLE(Person, name, age, hobbies);

// 3. 或者手动特化（最灵活）
template<>
struct serializer<Person> { /* ... */ };
```

**优点**: 兼顾简单性和灵活性
**缺点**: 实现较复杂

## 🔧 核心 API 设计

### 基础类型

```cpp
namespace cppx {

// JSON 值类型
class json {
public:
    // 构造
    json();
    json(nullptr_t);
    json(bool value);
    json(int value);
    json(double value);
    json(std::string value);
    json(std::initializer_list<json>);
    
    // 类型检查
    bool is_null() const;
    bool is_bool() const;
    bool is_number() const;
    bool is_string() const;
    bool is_array() const;
    bool is_object() const;
    
    // 值访问
    template<typename T>
    T as() const;
    
    // 对象操作
    json& operator[](const std::string& key);
    const json& operator[](const std::string& key) const;
    
    // 数组操作
    json& operator[](size_t index);
    size_t size() const;
    void push_back(json value);
    
    // 序列化
    std::string dump(int indent = -1) const;
    static json parse(std::string_view str);
};

// 二进制序列化
class binary_serializer {
public:
    // 序列化到字节流
    template<typename T>
    static std::vector<uint8_t> serialize(const T& value);
    
    // 从字节流反序列化
    template<typename T>
    static expected<T, std::string> deserialize(std::span<const uint8_t> data);
};

// 序列化特化接口
template<typename T>
struct serializer {
    static void to_json(json& j, const T& value);
    static void from_json(const json& j, T& value);
    static void to_binary(std::vector<uint8_t>& buffer, const T& value);
    static T from_binary(std::span<const uint8_t>& data);
};

} // namespace cppx
```

### 便捷函数

```cpp
// JSON 序列化/反序列化
template<typename T>
std::string to_json(const T& value, int indent = -1);

template<typename T>
expected<T, std::string> from_json(std::string_view json_str);

// 二进制序列化/反序列化
template<typename T>
std::vector<uint8_t> to_binary(const T& value);

template<typename T>
expected<T, std::string> from_binary(std::span<const uint8_t> data);

// 文件操作
template<typename T>
expected<void, std::string> save_json(const std::filesystem::path& path, const T& value);

template<typename T>
expected<T, std::string> load_json(const std::filesystem::path& path);
```

### 辅助宏

```cpp
// 为类型注册序列化支持
#define CPPX_SERIALIZABLE(Type, ...) \
    namespace cppx { \
    template<> \
    struct serializer<Type> { \
        static void to_json(json& j, const Type& v) { \
            /* 自动生成 */ \
        } \
        static void from_json(const json& j, Type& v) { \
            /* 自动生成 */ \
        } \
    }; \
    }

// 注册枚举
#define CPPX_ENUM_SERIALIZABLE(Enum) \
    /* 自动生成枚举的序列化代码 */
```

## 📖 使用示例

### Example 1: 基础类型

```cpp
import std;
import cppx.serialization;
import cppx.logging;

using namespace cppx;

int main() {
    auto logger = get_logger("serialization_demo");
    
    // 内置类型自动支持
    int num = 42;
    auto json_str = to_json(num);
    logger->info("JSON: {}", json_str);  // "42"
    
    std::string text = "hello";
    auto json_str2 = to_json(text);
    logger->info("JSON: {}", json_str2);  // "\"hello\""
    
    std::vector<int> vec = {1, 2, 3, 4, 5};
    auto json_str3 = to_json(vec);
    logger->info("JSON: {}", json_str3);  // "[1,2,3,4,5]"
    
    return 0;
}
```

### Example 2: 简单结构体（自动反射）

```cpp
import std;
import cppx.serialization;

using namespace cppx;

// 简单聚合类型自动支持
struct Point {
    int x;
    int y;
};

int main() {
    Point p{10, 20};
    
    // 自动序列化（使用聚合初始化反射）
    auto json_str = to_json(p);
    // {"x": 10, "y": 20}
    
    // 自动反序列化
    auto result = from_json<Point>(R"({"x": 30, "y": 40})");
    if (result.is_ok()) {
        Point p2 = result.value();
        // p2.x == 30, p2.y == 40
    }
    
    return 0;
}
```

### Example 3: 复杂结构体（使用宏）

```cpp
import std;
import cppx.serialization;

using namespace cppx;

struct Person {
    std::string name;
    int age;
    std::vector<std::string> hobbies;
    std::optional<std::string> email;
};

// 一行注册
CPPX_SERIALIZABLE(Person, name, age, hobbies, email);

int main() {
    Person p{
        .name = "Alice",
        .age = 30,
        .hobbies = {"reading", "coding", "gaming"},
        .email = "alice@example.com"
    };
    
    // JSON 序列化
    auto json_str = to_json(p, 2);  // 2 空格缩进
    std::cout << json_str << std::endl;
    /*
    {
      "name": "Alice",
      "age": 30,
      "hobbies": ["reading", "coding", "gaming"],
      "email": "alice@example.com"
    }
    */
    
    // JSON 反序列化
    auto result = from_json<Person>(json_str);
    if (result.is_ok()) {
        Person p2 = result.value();
        assert(p2.name == "Alice");
        assert(p2.age == 30);
    }
    
    // 二进制序列化
    auto binary = to_binary(p);
    std::cout << "Binary size: " << binary.size() << " bytes" << std::endl;
    
    // 二进制反序列化
    auto result2 = from_binary<Person>(binary);
    
    return 0;
}
```

### Example 4: 嵌套结构

```cpp
struct Address {
    std::string street;
    std::string city;
    std::string country;
};
CPPX_SERIALIZABLE(Address, street, city, country);

struct Company {
    std::string name;
    Address address;
    std::vector<Person> employees;
};
CPPX_SERIALIZABLE(Company, name, address, employees);

int main() {
    Company company{
        .name = "ACME Corp",
        .address = {"123 Main St", "New York", "USA"},
        .employees = {
            {"Alice", 30, {"reading"}, "alice@acme.com"},
            {"Bob", 25, {"gaming"}, std::nullopt}
        }
    };
    
    // 自动处理嵌套结构
    auto json_str = to_json(company, 2);
    
    // 保存到文件
    auto result = save_json("company.json", company);
    
    // 从文件加载
    auto loaded = load_json<Company>("company.json");
    
    return 0;
}
```

### Example 5: 枚举类型

```cpp
enum class Status {
    Active,
    Inactive,
    Pending
};

// 注册枚举
CPPX_ENUM_SERIALIZABLE(Status, Active, Inactive, Pending);

struct User {
    std::string username;
    Status status;
};
CPPX_SERIALIZABLE(User, username, status);

int main() {
    User user{"alice", Status::Active};
    
    auto json_str = to_json(user);
    // {"username": "alice", "status": "Active"}
    
    return 0;
}
```

### Example 6: 容器类型

```cpp
int main() {
    // 标准容器自动支持
    
    // vector
    std::vector<int> vec = {1, 2, 3};
    auto json1 = to_json(vec);  // [1, 2, 3]
    
    // map
    std::map<std::string, int> map = {{"a", 1}, {"b", 2}};
    auto json2 = to_json(map);  // {"a": 1, "b": 2}
    
    // optional
    std::optional<int> opt = 42;
    auto json3 = to_json(opt);  // 42
    
    std::optional<int> empty;
    auto json4 = to_json(empty);  // null
    
    // variant
    std::variant<int, std::string> var = "hello";
    auto json5 = to_json(var);  // {"type": "string", "value": "hello"}
    
    return 0;
}
```

### Example 7: 手动特化（最大灵活性）

```cpp
// 自定义类型，完全控制序列化逻辑
class Custom {
private:
    int internal_data_;
public:
    Custom(int data) : internal_data_(data) {}
    int get_data() const { return internal_data_; }
};

// 手动特化
namespace cppx {
    template<>
    struct serializer<Custom> {
        static void to_json(json& j, const Custom& c) {
            j = {{"data", c.get_data()}};
        }
        
        static void from_json(const json& j, Custom& c) {
            c = Custom(j["data"].as<int>());
        }
    };
}

int main() {
    Custom obj(123);
    auto json_str = to_json(obj);  // {"data": 123}
    
    return 0;
}
```

## 🏗️ 实现计划

### 模块结构

```
src/serialization.cppm
├── json 类（轻量级 JSON 实现）
├── binary_serializer 类
├── serializer<T> 特化接口
├── 内置类型支持（int, double, string, bool, etc.）
├── 容器支持（vector, map, optional, variant, etc.）
└── 便捷函数（to_json, from_json, to_binary, from_binary）

src/serialization_macros.h
├── CPPX_SERIALIZABLE 宏
├── CPPX_ENUM_SERIALIZABLE 宏
└── 辅助宏定义
```

### 核心功能

#### 1. JSON 类型系统

```cpp
class json {
    // 支持的类型
    enum class type { null, boolean, number, string, array, object };
    
    // 内部存储使用 variant
    using value_type = std::variant<
        std::nullptr_t,
        bool,
        double,
        std::string,
        std::vector<json>,
        std::map<std::string, json>
    >;
};
```

#### 2. 序列化特化（编译时）

```cpp
// 基础类型
template<> struct serializer<int> { /* ... */ };
template<> struct serializer<double> { /* ... */ };
template<> struct serializer<std::string> { /* ... */ };
template<> struct serializer<bool> { /* ... */ };

// 容器类型
template<typename T> struct serializer<std::vector<T>> { /* ... */ };
template<typename K, typename V> struct serializer<std::map<K, V>> { /* ... */ };
template<typename T> struct serializer<std::optional<T>> { /* ... */ };

// 用户类型通过宏或手动特化
```

#### 3. 二进制格式（类似 MessagePack）

```
Format:
- 类型标记（1 byte）
- 数据长度（变长编码）
- 数据内容

类型标记:
0x00: null
0x01: false
0x02: true
0x10-0x1F: 小整数（0-15）
0x20: int8
0x21: int16
0x22: int32
0x23: int64
0x30: float32
0x31: float64
0x40: string (length + data)
0x50: array (length + elements)
0x60: object (length + key-value pairs)
```

## 📚 完整示例代码

### serialization_demo.cpp

```cpp
import std;
import cppx.serialization;
import cppx.logging;

#include "../src/serialization_macros.h"

using namespace cppx;

// 示例 1: 简单结构
struct Point {
    int x;
    int y;
};
CPPX_SERIALIZABLE(Point, x, y);

// 示例 2: 嵌套结构
struct Rectangle {
    Point top_left;
    Point bottom_right;
    std::string color;
};
CPPX_SERIALIZABLE(Rectangle, top_left, bottom_right, color);

// 示例 3: 枚举
enum class Priority {
    Low,
    Medium,
    High
};
CPPX_ENUM_SERIALIZABLE(Priority, Low, Medium, High);

// 示例 4: 复杂结构
struct Task {
    std::string title;
    std::string description;
    Priority priority;
    std::optional<std::string> assignee;
    std::vector<std::string> tags;
    std::map<std::string, std::string> metadata;
};
CPPX_SERIALIZABLE(Task, title, description, priority, assignee, tags, metadata);

int main() {
    auto logger = get_logger("serialization_demo");
    logger->info("=== cppx.serialization Demo ===\n");
    
    // 1. 基础类型
    logger->info("1. Basic types:");
    {
        int num = 42;
        logger->info("   int: {}", to_json(num));
        
        std::string str = "hello";
        logger->info("   string: {}", to_json(str));
        
        std::vector<int> vec = {1, 2, 3, 4, 5};
        logger->info("   vector: {}", to_json(vec));
    }
    logger->info("");
    
    // 2. 简单结构
    logger->info("2. Simple struct (Point):");
    {
        Point p{10, 20};
        auto json_str = to_json(p, 2);
        logger->info("   JSON:\n{}", json_str);
        
        auto result = from_json<Point>(json_str);
        if (result.is_ok()) {
            auto p2 = result.value();
            logger->info("   Deserialized: x={}, y={}", p2.x, p2.y);
        }
    }
    logger->info("");
    
    // 3. 嵌套结构
    logger->info("3. Nested struct (Rectangle):");
    {
        Rectangle rect{
            {0, 0},
            {100, 50},
            "red"
        };
        
        auto json_str = to_json(rect, 2);
        logger->info("   JSON:\n{}", json_str);
    }
    logger->info("");
    
    // 4. 枚举
    logger->info("4. Enum serialization:");
    {
        Priority p = Priority::High;
        auto json_str = to_json(p);
        logger->info("   Priority: {}", json_str);  // "High"
    }
    logger->info("");
    
    // 5. 复杂结构
    logger->info("5. Complex struct (Task):");
    {
        Task task{
            .title = "Implement serialization",
            .description = "Add JSON and binary serialization support",
            .priority = Priority::High,
            .assignee = "Alice",
            .tags = {"feature", "serialization", "important"},
            .metadata = {{"sprint", "2024-Q1"}, {"points", "8"}}
        };
        
        auto json_str = to_json(task, 2);
        logger->info("   JSON:\n{}", json_str);
        
        // 二进制序列化
        auto binary = to_binary(task);
        logger->info("   Binary size: {} bytes", binary.size());
        
        // 反序列化
        auto result = from_binary<Task>(binary);
        if (result.is_ok()) {
            logger->info("   ✓ Binary deserialization successful");
        }
    }
    logger->info("");
    
    // 6. 文件操作
    logger->info("6. File operations:");
    {
        Task task{
            .title = "File test",
            .description = "Test file I/O",
            .priority = Priority::Medium,
            .assignee = std::nullopt,
            .tags = {"test"},
            .metadata = {}
        };
        
        // 保存到文件
        auto save_result = save_json("task.json", task);
        if (save_result.is_ok()) {
            logger->info("   ✓ Saved to task.json");
        }
        
        // 从文件加载
        auto load_result = load_json<Task>("task.json");
        if (load_result.is_ok()) {
            auto loaded = load_result.value();
            logger->info("   ✓ Loaded from task.json");
            logger->info("   Title: {}", loaded.title);
        }
    }
    logger->info("");
    
    // 7. 容器类型
    logger->info("7. Container types:");
    {
        std::map<std::string, int> scores = {
            {"Alice", 95},
            {"Bob", 87},
            {"Charlie", 92}
        };
        
        auto json_str = to_json(scores, 2);
        logger->info("   Scores:\n{}", json_str);
    }
    logger->info("");
    
    logger->info("=== Demo completed ===");
    return 0;
}
```

Expected output:
```
=== cppx.serialization Demo ===

1. Basic types:
   int: 42
   string: "hello"
   vector: [1,2,3,4,5]

2. Simple struct (Point):
   JSON:
{
  "x": 10,
  "y": 20
}
   Deserialized: x=10, y=20

3. Nested struct (Rectangle):
   JSON:
{
  "top_left": {"x": 0, "y": 0},
  "bottom_right": {"x": 100, "y": 50},
  "color": "red"
}

5. Complex struct (Task):
   JSON:
{
  "title": "Implement serialization",
  "description": "Add JSON and binary serialization support",
  "priority": "High",
  "assignee": "Alice",
  "tags": ["feature", "serialization", "important"],
  "metadata": {"sprint": "2024-Q1", "points": "8"}
}
   Binary size: 187 bytes
   ✓ Binary deserialization successful

...
```

## 🔍 技术细节

### 编译时反射

使用 C++23 的特性实现编译时反射：

```cpp
// 使用聚合初始化和结构化绑定
template<typename T>
concept AggregateType = std::is_aggregate_v<T>;

// 对于聚合类型，自动推导字段
template<AggregateType T>
struct serializer<T> {
    static void to_json(json& j, const T& value) {
        // 使用魔法解包聚合类型
        auto [field1, field2, ...] = value;
        j = {field1, field2, ...};
    }
};
```

### 宏实现原理

```cpp
#define CPPX_SERIALIZABLE(Type, ...) \
    namespace cppx { \
    template<> \
    struct serializer<Type> { \
        static void to_json(json& j, const Type& v) { \
            j = json::object(); \
            CPPX_SERIALIZE_FIELDS(v, __VA_ARGS__) \
        } \
        static void from_json(const json& j, Type& v) { \
            CPPX_DESERIALIZE_FIELDS(v, j, __VA_ARGS__) \
        } \
    }; \
    }

// 辅助宏
#define CPPX_SERIALIZE_FIELDS(obj, ...) \
    CPPX_FOR_EACH(CPPX_SERIALIZE_FIELD, obj, __VA_ARGS__)

#define CPPX_SERIALIZE_FIELD(obj, field) \
    j[#field] = cppx::to_json(obj.field);
```

## ❓ 问题和决策

### Q1: JSON 库实现还是使用第三方？
**决策**: 自己实现轻量级 JSON，保持零依赖
- 实现基础功能足够
- 性能可能不如 nlohmann/json，但对于大多数场景够用
- 如需高性能，后续可以提供适配器

### Q2: 反射方式？
**决策**: 混合方案
- 简单聚合类型：自动反射（C++23 特性）
- 复杂类型：使用 CPPX_SERIALIZABLE 宏
- 特殊需求：手动特化 serializer<T>

### Q3: 二进制格式？
**决策**: 类似 MessagePack 的简单格式
- 紧凑高效
- 易于实现
- 足够通用

### Q4: 性能要求？
**决策**: 先保证正确性和易用性，后续优化性能
- 使用移动语义减少拷贝
- 预分配缓冲区
- 后续可添加流式处理

## 🎯 实现优先级

### Phase 1: 核心功能（MVP）
1. ✅ json 类基础实现
2. ✅ 内置类型序列化（int, double, string, bool）
3. ✅ 容器类型（vector, map）
4. ✅ CPPX_SERIALIZABLE 宏
5. ✅ to_json / from_json 函数

### Phase 2: 扩展功能
1. ✅ 二进制序列化
2. ✅ optional, variant 支持
3. ✅ 枚举序列化
4. ✅ 文件 I/O 函数

### Phase 3: 高级功能（可选）
1. ⏸️ 运行时反射
2. ⏸️ 自定义格式化选项
3. ⏸️ 流式处理
4. ⏸️ 性能优化

## 🤔 你的反馈

请确认以下设计决策：

1. **API 风格**是否符合预期？
   - `to_json(obj)`, `from_json<T>(str)` 风格
   - `CPPX_SERIALIZABLE` 宏注册
   
2. **支持的格式**是否足够？
   - JSON（文本格式）
   - 二进制（MessagePack 风格）
   
3. **无侵入性**是否满足？
   - 使用外部宏注册
   - 或者手动特化 serializer<T>
   
4. **功能范围**是否合适？
   - 先实现 Phase 1（核心功能）
   - 还是一次性实现 Phase 1 + Phase 2？

请给出你的反馈，确认后我将开始实现！🚀

