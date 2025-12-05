# cppx.serialization 宏系统说明

## 📋 当前状态

### ✅ 使用符合标准的预处理器 (`/Zc:preprocessor`)

**已解决！** 通过启用 MSVC 的符合标准预处理器（VS2019 16.5+），宏系统现在**支持自动参数识别**！

### 当前使用方式（自动识别参数）

```cpp
// ✅ 自动识别参数个数！
CPPX_SERIALIZABLE(Point, x, y)  // 自动检测 2 个字段

// ✅ 枚举也支持自动识别
CPPX_ENUM_SERIALIZABLE(Priority, Low, Medium, High)  // 自动检测 3 个值

// ✅ 支持 1-10 个字段
CPPX_SERIALIZABLE(ComplexType, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10)
```

**编译器要求**：
- Visual Studio 2019 16.5 或更高版本
- 项目已配置 `/Zc:preprocessor` 标志

## 🔧 完整示例

### 示例 1: 2 字段结构体

```cpp
struct Point {
    int x;
    int y;
};

CPPX_SERIALIZABLE_2(Point, x, y)
```

### 示例 2: 3 字段结构体

```cpp
struct Person {
    std::string name;
    int age;
    std::vector<std::string> hobbies;
};

CPPX_SERIALIZABLE_3(Person, name, age, hobbies)
```

### 示例 3: 6 字段结构体

```cpp
struct ComplexType {
    std::string field1;
    int field2;
    double field3;
    std::vector<int> field4;
    std::optional<std::string> field5;
    std::map<std::string, int> field6;
};

CPPX_SERIALIZABLE_6(ComplexType, field1, field2, field3, field4, field5, field6)
```

### 示例 4: 枚举类型

```cpp
enum class Status {
    Active,
    Inactive,
    Pending,
    Deleted
};

CPPX_ENUM_SERIALIZABLE_4(Status, Active, Inactive, Pending, Deleted)
```

## 📊 支持的字段数量

| 宏 | 支持的字段数 |
|---|---|
| `CPPX_SERIALIZABLE_1` | 1 字段 |
| `CPPX_SERIALIZABLE_2` | 2 字段 |
| `CPPX_SERIALIZABLE_3` | 3 字段 |
| `CPPX_SERIALIZABLE_4` | 4 字段 |
| `CPPX_SERIALIZABLE_5` | 5 字段 |
| `CPPX_SERIALIZABLE_6` | 6 字段 |
| `CPPX_SERIALIZABLE_7` | 7 字段 |
| `CPPX_SERIALIZABLE_8` | 8 字段 |
| `CPPX_SERIALIZABLE_9` | 9 字段 *(新增)* |
| `CPPX_SERIALIZABLE_10` | 10 字段 *(新增)* |

| 宏 | 支持的枚举值数 |
|---|---|
| `CPPX_ENUM_SERIALIZABLE_1` | 1 值 |
| `CPPX_ENUM_SERIALIZABLE_2` | 2 值 |
| `CPPX_ENUM_SERIALIZABLE_3` | 3 值 |
| `CPPX_ENUM_SERIALIZABLE_4` | 4 值 |
| `CPPX_ENUM_SERIALIZABLE_5` | 5 值 |

## 🚧 替代方案

如果你的结构体字段超过 10 个，或者不想手动指定数量，可以使用**手动特化**：

```cpp
struct LargeStruct {
    std::string f1, f2, f3, f4, f5;
    std::string f6, f7, f8, f9, f10;
    std::string f11, f12;  // 超过 10 个字段
};

// 手动特化
namespace cppx {
    template<>
    struct serializer<LargeStruct> {
        static json to_json(const LargeStruct& value) {
            auto j = json::object();
            j["f1"] = value.f1;
            j["f2"] = value.f2;
            // ... 其他字段
            j["f12"] = value.f12;
            return j;
        }
        
        static LargeStruct from_json(const json& j) {
            LargeStruct value{};
            if (j.contains("f1")) value.f1 = j["f1"].as<std::string>();
            if (j.contains("f2")) value.f2 = j["f2"].as<std::string>();
            // ... 其他字段
            if (j.contains("f12")) value.f12 = j["f12"].as<std::string>();
            return value;
        }
    };
}
```

虽然手动特化代码稍长，但它：
- ✅ 支持任意数量的字段
- ✅ 完全控制序列化逻辑
- ✅ 可以自定义字段名称
- ✅ 可以添加验证和转换逻辑

## 🔬 技术细节

### 符合标准预处理器的作用

`/Zc:preprocessor` 标志启用了 MSVC 的符合 C++11 标准的预处理器，修复了：
- ✅ `__VA_ARGS__` 的正确展开
- ✅ Token pasting (`##`) 在复杂上下文中的工作
- ✅ 宏参数计数的正确性
- ✅ 嵌套宏调用的正确处理

**参考**：[Microsoft Docs - /Zc:preprocessor](https://docs.microsoft.com/en-us/cpp/build/reference/zc-preprocessor)

### 宏定义原理

```cpp
// 1. 参数计数（排除 Type 参数）
#define CPPX_COUNT_IMPL(_1, _2, _3, ..., _11, N, ...) N
#define CPPX_FIELD_COUNT(...) CPPX_COUNT_IMPL(__VA_ARGS__, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)

// 2. Token pasting 连接宏名
#define CPPX_CONCAT(a, b) a##b
#define CPPX_SERIALIZABLE_DISPATCH(N) CPPX_CONCAT(CPPX_SERIALIZABLE_, N)

// 3. 自动分发到对应宏
#define CPPX_SERIALIZABLE(...) \
    CPPX_SERIALIZABLE_DISPATCH(CPPX_FIELD_COUNT(__VA_ARGS__))(__VA_ARGS__)

// 4. 单字段序列化助手
#define CPPX_SER_FIELD(obj, field) \
    j[#field] = cppx::serializer<std::remove_cvref_t<decltype(obj.field)>>::to_json(obj.field);

// 5. 实际宏定义
#define CPPX_SERIALIZABLE_2(Type, f1, f2) \
    namespace cppx { \
    template<> \
    struct serializer<Type> { \
        static json to_json(const Type& value) { \
            auto j = json::object(); \
            CPPX_SER_FIELD(value, f1)  // 展开为：j["f1"] = ...
            CPPX_SER_FIELD(value, f2)  // 展开为：j["f2"] = ...
            return j; \
        } \
        static Type from_json(const json& j_param) { \
            Type value{}; \
            CPPX_DESER_FIELD(value, j_param, f1)
            CPPX_DESER_FIELD(value, j_param, f2)
            return value; \
        } \
    }; \
    }
```

### ~~为什么不能自动参数计数？~~ （已解决！）

**历史问题**：传统的 MSVC 预处理器在 C++20 模块中有这些问题：
- ❌ 宏展开失败：`warning C4003: 参数不足`
- ❌ Token pasting (`##`) 在某些上下文中失效
- ❌ `__VA_ARGS__` 处理有 bug

**解决方案**：使用 `/Zc:preprocessor` 标志！

这个标志启用了**符合标准的预处理器**（conforming preprocessor），完全解决了上述问题：
```cpp
// 现在这个可以工作了！
#define CPPX_CONCAT(a, b) a##b  // ✅ Token pasting 正常工作
#define CPPX_FIELD_COUNT(...) /* ... */  // ✅ __VA_ARGS__ 正确展开
#define CPPX_SERIALIZABLE(...) \
    CPPX_SERIALIZABLE_DISPATCH(CPPX_FIELD_COUNT(__VA_ARGS__))(__VA_ARGS__)
```

**性能提示**：符合标准的预处理器可能稍微增加编译时间，但换来的是正确的宏行为。

## 🎯 最佳实践

### ✅ 推荐做法

1. **所有结构体（1-10字段）**: 使用自动参数计数！
```cpp
CPPX_SERIALIZABLE(Point, x, y)  // 自动！
CPPX_SERIALIZABLE(Person, name, age, email, phone, address)  // 自动！
CPPX_ENUM_SERIALIZABLE(Status, Active, Inactive, Pending)  // 自动！
```

2. **大结构体（>10字段）**: 手动特化或增加宏支持
```cpp
namespace cppx {
    template<> struct serializer<LargeType> { /* ... */ };
}
```

3. **确保启用符合标准预处理器**:
```lua
-- xmake.lua
add_cxxflags("/Zc:preprocessor", {tools = "cl"})
```

### ❌ 避免的做法

- ❌ 不要忘记添加 `/Zc:preprocessor` 标志（会导致宏展开错误）
- ❌ 不要超过 10 个字段（当前限制，可扩展）

## 📚 参考

- **设计文档**: `docs/SERIALIZATION_DESIGN.md`
- **实现状态**: `docs/SERIALIZATION_STATUS.md`
- **示例代码**: `examples/serialization_demo.cpp`
- **测试代码**: `tests/test_serialization_simple.cpp`

## 💡 未来改进方向

### 可能的扩展

1. **支持更多字段**
   - 当前支持 1-10 个字段
   - 可以轻松扩展到 20、30 个字段

2. **改用 C++26 反射**（长期）
   - C++26 将引入标准反射
   - 可以完全避免宏

3. **其他序列化格式**
   - 二进制格式（MessagePack）
   - XML、YAML 等

4. **性能优化**
   - 编译时字符串哈希
   - Zero-copy 序列化

---

**总结**: 使用 `/Zc:preprocessor` 后，宏系统已经非常完善！自动参数识别让序列化代码简洁优雅。🎯✨

