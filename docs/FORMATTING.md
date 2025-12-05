# 代码格式化指南

## 📋 格式化工具

本项目使用 **clang-format** 作为 C++ 代码格式化工具。

### 配置文件

`.clang-format` - 定义了项目的代码风格规则：
- 基于 Microsoft 风格
- 缩进：4 空格
- 列宽限制：100 字符
- 指针/引用对齐：左对齐 (`int* ptr`)
- C++20/23 模块支持

## 🛠️ 安装 clang-format

### Windows

**选项 1：通过 Visual Studio**
- 安装 Visual Studio 时选择 "C++ Clang tools for Windows"

**选项 2：通过 Chocolatey**
```powershell
choco install llvm
```

**选项 3：通过 Scoop**
```powershell
scoop install llvm
```

**选项 4：直接下载**
- 访问 https://releases.llvm.org/
- 下载最新的 LLVM Windows 安装包
- 安装后确保 `clang-format` 在 PATH 中

### 验证安装

```powershell
clang-format --version
```

应该显示类似：`clang-format version 17.0.0` 或更高版本

## 📝 使用方法

### 方法 1：使用 xmake task（推荐）✨

安装 LLVM/clang-format 后，直接使用 xmake：

```bash
# 格式化所有文件
xmake format

# 检查格式（不修改文件）
xmake format --check

# 或使用短选项
xmake format -c
```

**优点**：
- ✅ 跨平台（Windows、Linux、macOS）
- ✅ 集成在构建系统中
- ✅ 彩色输出，易于查看
- ✅ 自动查找所有源文件
- ✅ 脚本独立维护（`scripts/format.lua`）

### 方法 2：直接使用 clang-format

```bash
# 格式化单个文件
clang-format -i src/logging.cppm

# 查看格式化差异（不修改）
clang-format src/logging.cppm | diff - src/logging.cppm

# 格式化所有文件（Unix/Linux）
find src tests examples -name '*.cpp' -o -name '*.cppm' -o -name '*.h' | xargs clang-format -i
```

## 🔧 IDE 集成

### Visual Studio Code

1. 安装 C/C++ 扩展
2. 在 `settings.json` 中添加：

```json
{
    "C_Cpp.clang_format_fallbackStyle": "none",
    "C_Cpp.clang_format_style": "file",
    "editor.formatOnSave": true,
    "[cpp]": {
        "editor.defaultFormatter": "ms-vscode.cpptools"
    }
}
```

### Visual Studio

1. 工具 → 选项 → 文本编辑器 → C/C++ → 格式设置
2. 选择 "使用 clang-format"
3. 勾选 "自动格式化"

### CLion / Rider

1. Settings → Editor → Code Style → C/C++
2. 选择 "ClangFormat"
3. 启用 "Enable ClangFormat"

## 📐 格式化规则示例

### 缩进和大括号

```cpp
// ✓ 正确
class Example {
public:
    void foo() {
        if (condition) {
            doSomething();
        }
    }
};

// ✗ 错误
class Example 
{
  public:
    void foo() 
    {
      if(condition){doSomething();}
    }
};
```

### 指针和引用

```cpp
// ✓ 正确
int* ptr;
const std::string& str;

// ✗ 错误
int *ptr;
const std::string &str;
```

### 模块导入

```cpp
// ✓ 正确 - 模块导入在最前面
import std;
import cppx.logging;

#include "config.h"

#include <Windows.h>

// ✗ 错误 - 顺序混乱
#include <Windows.h>
import std;
#include "config.h"
```

### 函数声明

```cpp
// ✓ 正确 - 单行简短函数
inline int square(int x) { return x * x; }

// ✓ 正确 - 多行复杂函数
void complex_function(
    const std::string& arg1,
    int arg2,
    bool arg3
) {
    // 实现...
}

// ✗ 错误 - 格式不一致
inline int square(int x){return x*x;}
void complex_function(const std::string& arg1,int arg2,bool arg3){
// 实现...
}
```

### 命名空间

```cpp
// ✓ 正确 - 命名空间不缩进
namespace cppx {

class MyClass {
    void foo();
};

}  // namespace cppx

// ✗ 错误 - 命名空间内容缩进
namespace cppx {
    class MyClass {
        void foo();
    };
}
```

## 🔍 CI 集成

GitHub Actions 会在每次提交时自动检查代码格式：

```yaml
- name: Check formatting
  run: xmake format --check
```

如果格式不符合规范，CI 会失败。

**注意**：需要先安装 clang-format：
```yaml
- name: Install clang-format
  run: choco install llvm
```

## 💡 最佳实践

1. **提交前格式化**（推荐使用 xmake）：
   ```bash
   # 格式化
   xmake format
   
   # 验证
   xmake format --check
   
   # 提交
   git add -A
   git commit -m "your message"
   ```

2. **配置 IDE 自动格式化**：保存文件时自动格式化

3. **使用 pre-commit hook**（可选）：
   ```bash
   # .git/hooks/pre-commit
   #!/bin/sh
   xmake format --check
   ```

4. **定期运行格式化**：确保整个代码库风格一致
   ```bash
   xmake format
   ```

## 🚫 .clang-format-ignore

如果某些文件不需要格式化，创建 `.clang-format-ignore` 文件：

```
# 忽略第三方库
third_party/**
vendor/**

# 忽略生成的文件
build/**
*.generated.cpp
```

## 📚 参考资料

- [clang-format 官方文档](https://clang.llvm.org/docs/ClangFormat.html)
- [clang-format 选项参考](https://clang.llvm.org/docs/ClangFormatStyleOptions.html)
- [Microsoft C++ 代码风格](https://docs.microsoft.com/en-us/cpp/cpp/formatting-in-visual-studio)

## 🎯 快速命令参考

```bash
# 格式化所有文件
xmake format

# 检查格式
xmake format --check

# 查看帮助
xmake format --help
```

---

**总结**：使用 `xmake format` 保持代码风格一致，提交前运行 `xmake format --check` 验证格式！✨

