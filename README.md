# cppx - c++23 Module 基础库

现代化、零依赖的 c++23 模块库，使用 MSVC 和 `import std` 特性。

## 特性

- ✅ **现代 C++**: 使用 C++23 `import std` 特性
- ✅ **零依赖**: 仅依赖标准库
- ✅ **8 个核心模块**: 错误处理、字符串、文件系统、日志、时间、并发、数学、CLI
- ✅ **类型安全**: expected<T,E> 错误处理
- ✅ **完整测试**: 所有模块都有完整的测试套件
- ✅ **完整文档**: docs/API.md

## 快速开始

### 1. 构建核心库

```bash
# 配置项目 (需要 Windows + MSVC)
xmake f -c

# 编译核心库
xmake build cppx

# 产物: build/windows/x64/release/cppx.lib
```

### 2. 编译和运行测试

```bash
# 编译单个测试
xmake build test_error
xmake run test_error

# 编译所有测试
xmake build test_error test_string_utils test_file_system test_logging test_time_utils test_concurrent test_math_utils test_cli
```

### 3. 编译和运行示例

```bash
# 编译所有示例
xmake -g examples

# 运行示例
xmake run error_demo
xmake run logging_demo
```

## 模块列表

| 模块 | 功能 |
|------|------|
| **cppx.error** | expected<T,E>, error_info |
| **cppx.string_utils** | split, join, trim, 大小写转换 |
| **cppx.file_system** | 文件读写, 临时文件 |
| **cppx.logging** | 分级日志, 彩色输出 |
| **cppx.time_utils** | ISO8601, 计时器 |
| **cppx.concurrent** | 线程池, channel |
| **cppx.math_utils** | 统计, 插值, 随机数 |
| **cppx.cli** | 命令行解析 |

## 使用示例

```cpp
import cppx.error;
import cppx.string_utils;

int main() {
    // 字符串操作
    auto parts = cppx::split("a,b,c", ",");
    
    // 错误处理
    auto result = cppx::expected<int, std::string>::ok(42);
    if (result.is_ok()) {
        int value = result.value();
    }
    
    return 0;
}
```

## 在项目中使用

```cpp
import cppx.error;
import cppx.string_utils;
import cppx.logging;

int main() {
    // 使用 logger
    auto logger = cppx::get_logger("myapp");
    logger->info("Application started");
    
    // 字符串操作
    auto parts = cppx::split("a,b,c", ",");
    
    // 错误处理
    auto result = cppx::expected<int, cppx::error_info>::ok(42);
    if (result.is_ok()) {
        int value = result.value();
        logger->info("Result: {}", value);
    }
    
    return 0;
}
```

## 文档

- **docs/API.md** - 完整 API 文档

## 系统要求

- **编译器**: MSVC 19.44+ (Visual Studio 2022)
- **构建工具**: xmake 2.8+
- **C++ 标准**: C++23 (使用 `/std:c++23`)
- **平台**: Windows

**重要**: 
- 本项目仅支持 MSVC 编译器
- 使用 `import std` 特性，需要 Visual Studio 2022 或更新版本
- 之前的 Clang/GCC 支持已移除，因为它们在 c++23 modules 方面存在问题

## 迁移说明

如果你之前使用的是 Clang/GCC 版本，请查看 `MIGRATION_TO_IMPORT_STD.md` 了解详细的迁移说明。

## 许可证

MIT License

## 当前状态

- ✅ 核心库 (cppx.lib): 完全可用
- ✅ 所有测试: 通过
- ✅ 所有示例: 运行正常

核心库功能完整，已在 MSVC 上完全测试通过，可用于生产环境。
