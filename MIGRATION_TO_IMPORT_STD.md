# Migration to `import std` with MSVC

本项目已成功迁移到使用 MSVC 编译器和 C++23 `import std` 特性。

## 主要变更

### 1. 构建系统配置 (xmake.lua)
- **仅支持 MSVC**: 移除了 GCC/Clang 的支持
- **编译器标志**: 添加 `/std:c++20` 以启用最新的 C++ 标准特性
- **UTF-8 支持**: 保留 `/utf-8` 标志确保源文件编码正确
- **统一配置**: 所有 target (核心库、测试、示例) 都使用相同的编译器配置

### 2. 模块文件修改
所有 `.cppm` 模块文件已更新：

#### 之前的模式:
```cpp
module;

#include <string>
#include <vector>
#include <iostream>
// ... 更多头文件

export module cppx.xxx;
```

#### 现在的模式:
```cpp
export module cppx.xxx;

import std;
import cppx.error;  // 导入其他模块依赖
```

### 3. 平台特定代码调整
- **时间函数**: 从 `gmtime_s`/`localtime_s` 改为使用标准的 `std::gmtime`/`std::localtime`
- **原子标志**: 从 `ATOMIC_FLAG_INIT` 改为默认初始化 `std::atomic_flag flag_;`

### 4. 已修改的模块
- ✅ `src/error.cppm` - 错误处理
- ✅ `src/string_utils.cppm` - 字符串工具
- ✅ `src/file_system.cppm` - 文件系统操作
- ✅ `src/logging.cppm` - 日志系统
- ✅ `src/time_utils.cppm` - 时间工具
- ✅ `src/concurrent.cppm` - 并发工具
- ✅ `src/math_utils.cppm` - 数学工具
- ✅ `src/cli.cppm` - 命令行解析

## 验证测试

所有测试和示例程序已通过编译和运行测试：

### 测试程序
- ✅ `test_error` - 错误处理测试
- ✅ `test_string_utils` - 字符串工具测试
- ✅ `test_file_system` - 文件系统测试
- ✅ `test_logging` - 日志系统测试
- ✅ `test_time_utils` - 时间工具测试
- ✅ `test_concurrent` - 并发测试
- ✅ `test_math_utils` - 数学工具测试
- ✅ `test_cli` - CLI 测试

### 示例程序
- ✅ `error_demo` - 错误处理示例
- ✅ `logging_demo` - 日志系统示例
- ✅ 以及所有其他示例程序

## 构建说明

### 前提条件
- Windows 操作系统
- Visual Studio 2022 (MSVC 19.44+)
- xmake 3.0+

### 构建命令
```bash
# 配置项目
xmake f -c

# 编译核心库
xmake build cppx

# 编译所有测试
xmake build test_error test_string_utils test_file_system test_logging test_time_utils test_concurrent test_math_utils test_cli

# 编译所有示例
xmake -g examples

# 运行特定测试
xmake run test_error
```

## 优势

1. **更快的编译速度**: `import std` 比传统头文件更快
2. **更清晰的依赖**: 模块系统明确声明依赖关系
3. **更好的隔离**: 减少命名空间污染
4. **现代 C++**: 使用最新的 C++23 特性

## 注意事项

- 本项目现在**仅支持 MSVC 编译器**
- 需要 Visual Studio 2022 或更新版本
- 标准库模块 (`import std`) 是 MSVC 的实验性特性，但已经相当稳定
- 编译时可能会看到警告 "正在重写 /std:c++20 (用 /std:c++20)"，这是正常的

## 迁移完成日期
2025-12-05

