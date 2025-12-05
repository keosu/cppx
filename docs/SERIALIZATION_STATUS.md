# cppx.serialization - 实现状态

## ✅ 已完成功能（Phase 1）

### 1. JSON 核心功能
- ✅ **json 类型系统**
  - 支持所有 JSON 类型：null, boolean, number, string, array, object
  - 类型安全的值访问（`as_bool()`, `as_int()`, `as_string()` 等）
  - 动态类型检查（`is_null()`, `is_array()` 等）

- ✅ **JSON 构造与操作**
  - 对象操作：`j["key"] = value`, `j.contains("key")`
  - 数组操作：`j.push_back(value)`, `j[index]`, `j.size()`
  - 初始化列表支持

- ✅ **JSON 序列化**
  - 紧凑格式：`j.dump()`
  - 美化格式：`j.dump(2)` // 2 空格缩进
  - 字符串转义处理

- ✅ **JSON 解析**
  - 完整的 JSON 解析器实现
  - 支持嵌套结构
  - 错误处理和验证

### 2. 类型序列化支持

#### 基础类型
- ✅ `bool`, `int`, `long`, `long long`
- ✅ `unsigned int`, `unsigned long`, `unsigned long long`
- ✅ `float`, `double`
- ✅ `std::string`

#### 容器类型
- ✅ `std::vector<T>`
- ✅ `std::map<K, V>` (key 自动转换为字符串)
- ✅ `std::optional<T>` (null 或值)

### 3. 便捷函数
- ✅ `to_json_string<T>(value, indent=-1)` - 序列化为 JSON 字符串
- ✅ `from_json_string<T>(json_str)` - 从 JSON 字符串反序列化
- ✅ `save_json(path, value, indent=2)` - 保存到文件
- ✅ `load_json<T>(path)` - 从文件加载

### 4. 示例和文档
- ✅ `examples/serialization_simple_demo.cpp` - 完整的功能演示
- ✅ `docs/SERIALIZATION_DESIGN.md` - 详细设计文档
- ✅ 运行成功，输出正确

## ⚠️ 已知问题

### 宏问题（MSVC + C++20 Modules）
`CPPX_SERIALIZABLE` 和 `CPPX_ENUM_SERIALIZABLE` 宏在 MSVC 的 C++20 模块环境下存在编译错误：

**错误表现**：
```
error C4003: 类函数宏的调用"CPPX_FOREACH_2"参数不足
error C2065: "y": 未声明的标识符
error C3553: decltype 应为表达式而不是类型
```

**原因分析**：
1. MSVC 的宏展开与模块系统交互存在 bug
2. `CPPX_FOREACH` 宏的参数传递在模块上下文中失败
3. `decltype(obj.field)` 无法正确推导类型

**影响范围**：
- ❌ `examples/serialization_demo.cpp` 无法编译
- ❌ `tests/test_serialization.cpp` 无法编译
- ✅ 手动特化 `serializer<T>` 仍然可用
- ✅ 基础类型和容器类型不受影响

## 📋 待实现功能

### Phase 2: 修复宏支持
- ⏸️ 修复 `CPPX_SERIALIZABLE` 宏的编译问题
  - 可能的解决方案：
    1. 改用模板元编程代替宏
    2. 使用 C++23 的反射特性（如果编译器支持）
    3. 简化宏实现，避免复杂的参数传递
    4. 移到单独的 `.cpp` 文件而非模块中

- ⏸️ 修复 `CPPX_ENUM_SERIALIZABLE` 宏

### Phase 3: 扩展类型支持
- ⏸️ `std::set<T>`, `std::unordered_map<K,V>`
- ⏸️ `std::variant<Ts...>` (tagged union)
- ⏸️ `std::unique_ptr<T>`, `std::shared_ptr<T>`
- ⏸️ `std::array<T, N>`
- ⏸️ `std::tuple<Ts...>`

### Phase 4: 二进制序列化
- ⏸️ `binary_serializer` 类
- ⏸️ MessagePack 风格的紧凑格式
- ⏸️ `to_binary<T>()` / `from_binary<T>()` 函数

### Phase 5: 高级功能
- ⏸️ 运行时反射
- ⏸️ 自定义格式化选项
- ⏸️ 流式处理
- ⏸️ 性能优化

## 🎯 当前可用的使用方式

### 方式 1: 直接使用 JSON API（推荐）

```cpp
import cppx.serialization;

// 创建 JSON
json person = json::object();
person["name"] = "Alice";
person["age"] = 30;
person["hobbies"] = json::array();
person["hobbies"].push_back("reading");
person["hobbies"].push_back("coding");

// 序列化
std::string json_str = person.dump(2);

// 解析
auto loaded = json::parse(json_str);
```

### 方式 2: 使用内置类型序列化

```cpp
import cppx.serialization;

// 基础类型
int num = 42;
auto json1 = to_json_string(num);  // "42"

// 容器
std::vector<int> vec = {1, 2, 3};
auto json2 = to_json_string(vec);  // "[1,2,3]"

// optional
std::optional<int> opt = 42;
auto json3 = to_json_string(opt);  // "42"
```

### 方式 3: 手动特化 serializer（灵活）

```cpp
struct Point {
    int x, y;
};

namespace cppx {
    template<>
    struct serializer<Point> {
        static json to_json(const Point& p) {
            auto j = json::object();
            j["x"] = p.x;
            j["y"] = p.y;
            return j;
        }
        
        static Point from_json(const json& j) {
            return Point{
                j["x"].as_int(),
                j["y"].as_int()
            };
        }
    };
}

// 使用
Point p{10, 20};
auto json_str = to_json_string(p);
auto loaded = from_json_string<Point>(json_str);
```

## 📊 性能特征

### JSON 解析器
- **实现**: 手写递归下降解析器
- **优点**: 零依赖，完全控制
- **缺点**: 性能不如 nlohmann/json 或 simdjson
- **适用场景**: 中小型 JSON（< 1MB）

### 序列化
- **时间复杂度**: O(n)，n 为数据大小
- **空间复杂度**: O(n)
- **优化**: 使用移动语义减少拷贝

## 🔧 故障排除

### Q: 为什么 `CPPX_SERIALIZABLE` 不能用？
A: 这是 MSVC + C++20 模块的已知问题。请使用方式 3（手动特化）或等待修复。

### Q: 如何序列化自定义类型？
A: 手动特化 `serializer<YourType>`，参见上面的示例。

### Q: 支持哪些容器？
A: 目前支持 `std::vector`, `std::map`, `std::optional`。更多容器类型将在 Phase 3 添加。

### Q: 如何处理嵌套结构？
A: 自动支持！只要每个嵌套类型都有 `serializer` 特化即可。

## 📚 参考资源

- 设计文档：`docs/SERIALIZATION_DESIGN.md`
- 示例代码：`examples/serialization_simple_demo.cpp`
- 模块源码：`src/serialization.cppm`

## 🚀 下一步计划

1. **优先级 1**: 修复宏支持（解除 10+ 个结构体的序列化限制）
2. **优先级 2**: 添加更多容器类型支持
3. **优先级 3**: 实现二进制序列化
4. **优先级 4**: 性能优化和基准测试

---

**更新时间**: 2025-12-05
**状态**: Phase 1 完成，Phase 2+ 待实现

