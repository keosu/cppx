# cppx.serialization 宏系统说明

## 📋 当前状态

### ⚠️ MSVC C++20 模块的宏限制

MSVC 编译器在 C++20 模块环境下对宏展开存在已知 bug：
- 复杂的变参宏无法正确展开
- `__VA_ARGS__` 的参数计数宏在模块中失效
- 宏参数传递中的 `decltype` 推导失败

因此，**当前版本需要明确指定字段/枚举值的数量**。

### 当前使用方式（必须指定数量）

```cpp
// ❌ 不支持自动参数识别
// CPPX_SERIALIZABLE(Point, x, y)  // 编译失败！

// ✅ 必须明确指定字段数量
CPPX_SERIALIZABLE_2(Point, x, y)  // 2 个字段

// ✅ 枚举也一样
CPPX_ENUM_SERIALIZABLE_3(Priority, Low, Medium, High)  // 3 个值
```

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

### 宏定义原理

```cpp
// 单字段序列化助手
#define CPPX_SER_FIELD(obj, field) \
    j[#field] = cppx::serializer<std::remove_cvref_t<decltype(obj.field)>>::to_json(obj.field);

// 实际宏定义
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

### 为什么不能自动参数计数？

尝试过的方案：

**方案 A: 使用 CPPX_NARG 宏**
```cpp
#define CPPX_ARG_N(...) /* 计数宏 */
#define CPPX_SERIALIZABLE(...) \
    CPPX_SERIALIZABLE_##CPPX_NARG(__VA_ARGS__)(__VA_ARGS__)
```
❌ MSVC 在模块中展开失败：`warning C4003: 参数不足`

**方案 B: 使用 CPPX_GET_MACRO 选择**
```cpp
#define CPPX_GET_MACRO(_1, _2, _3, ..., N, ...) N
#define CPPX_SERIALIZABLE(...) \
    CPPX_GET_MACRO(__VA_ARGS__, ..., CPPX_SERIALIZABLE_2)(__VA_ARGS__)
```
❌ MSVC 在模块中无法正确匹配：`error C2065: 未声明的标识符`

**方案 C: Token pasting 技巧**
```cpp
#define CPPX_CONCAT(a, b) a##b
#define CPPX_DISPATCH(n) CPPX_CONCAT(CPPX_SERIALIZABLE_, n)
```
❌ 在模块环境下 `##` 连接符失败

### 根本原因

MSVC 的预处理器在处理 C++20 模块时：
1. 宏展开的时机和传统头文件不同
2. 模块边界会影响宏的作用域
3. `__VA_ARGS__` 的处理有 bug
4. Token pasting (`##`) 在某些上下文中失效

这是编译器的已知限制，Microsoft 尚未修复。

## 🎯 最佳实践

### ✅ 推荐做法

1. **小结构体（1-10字段）**: 使用 `CPPX_SERIALIZABLE_N`
```cpp
CPPX_SERIALIZABLE_3(Person, name, age, email)
```

2. **大结构体（>10字段）**: 手动特化
```cpp
namespace cppx {
    template<> struct serializer<LargeType> { /* ... */ };
}
```

3. **代码生成**: 考虑使用脚本生成序列化代码

### ❌ 避免的做法

- ❌ 不要尝试使用不带数字的 `CPPX_SERIALIZABLE` (会编译失败)
- ❌ 不要字段数和宏数字不匹配
  ```cpp
  // 错误：3 个字段但使用 _2 宏
  CPPX_SERIALIZABLE_2(Type, f1, f2, f3)  // 编译失败！
  ```

## 📚 参考

- **设计文档**: `docs/SERIALIZATION_DESIGN.md`
- **实现状态**: `docs/SERIALIZATION_STATUS.md`
- **示例代码**: `examples/serialization_demo.cpp`
- **测试代码**: `tests/test_serialization_simple.cpp`

## 💡 未来改进方向

### 可能的解决方案

1. **等待 MSVC 修复宏 bug**
   - Microsoft 已知此问题
   - 未来版本可能修复

2. **改用 C++26 反射**
   - C++26 将引入标准反射
   - 可以完全避免宏

3. **使用外部代码生成器**
   - 类似 protobuf 的方案
   - 从 IDL 文件生成序列化代码

4. **使用模板元编程**
   - 基于聚合初始化的反射
   - 限制：只支持简单聚合类型

---

**总结**: 虽然当前需要手动指定字段数量，但宏仍然大大简化了序列化代码。对于大多数场景（1-10 字段）已经足够使用。🎯

