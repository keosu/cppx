# 编译器支持说明

## 当前支持

**仅支持: Windows + MSVC (Visual Studio 2022+)**

## 为什么不支持 Clang/GCC？

### C++23 `import std` 的现状

本项目大量使用 C++23 的 `import std` 特性。截至 2024 年底：

#### ✅ MSVC (推荐)
- **状态**: ✅ 完全支持，生产就绪
- **版本**: Visual Studio 2022 17.5+
- **配置**: 开箱即用，无需额外配置
- **体验**: 编译速度快，工具链成熟

#### ⚠️ Clang
- **状态**: ⚠️ 实验性支持，配置复杂
- **版本**: Clang 17+ (推荐 19+)
- **问题**:
  1. 需要 libc++ (不是默认的 libstdc++)
  2. 需要手动预编译 std 模块 (~34MB 的 .pcm 文件)
  3. xmake 对 Clang `import std` 的支持不完善
  4. 模块依赖检测存在问题
- **结论**: 虽然技术上可行，但实际使用困难重重

#### ❌ GCC
- **状态**: ❌ 不支持
- **原因**: libstdc++ 对 C++23 模块的支持还在开发中
- **预计**: 可能在 GCC 14/15 才会有较好的支持

### 我们的实验结果

在 WSL2 + Clang 22 上的测试表明：

1. **可以手动编译** `import std` 代码：
   ```bash
   # 预编译 std 模块
   clang++-22 --precompile /usr/lib/llvm-22/share/libc++/v1/std.cppm -o std.pcm
   
   # 编译程序
   clang++-22 -fprebuilt-module-path=. source.cpp std.pcm -lc++abi
   ```

2. **xmake 集成困难**：
   - xmake 的模块依赖检测无法识别预编译的 std 模块
   - 报错: `missing std dependency for module cppx.xxx`
   - 即使设置了 `-fprebuilt-module-path`，依赖检测仍然失败

3. **维护成本高**：
   - 需要维护复杂的构建脚本
   - 每次清理构建都要重新编译 std 模块（耗时）
   - 用户需要手动安装和配置 libc++

## 设计决策

**我们决定暂时只支持 MSVC**，原因：

1. **项目目标**: 这是一个现代 C++23 库，展示最新特性
2. **用户体验**: MSVC 提供了最佳的开箱即用体验
3. **稳定性**: 避免因工具链问题影响项目本身的开发
4. **聚焦**: 专注于库的功能，而不是构建系统的兼容性

## 未来展望

当以下条件满足时，我们会重新考虑支持 Clang/GCC：

- ✅ xmake 完善了对 Clang `import std` 的支持
- ✅ Clang/libc++ 提供了更简单的 std 模块集成方式
- ✅ GCC/libstdc++ 完成了 C++23 模块支持

## 替代方案

如果你必须在 Linux 上开发，可以考虑：

### 方案 1: WSL + Visual Studio (推荐)
使用 WSL2 配合 Visual Studio 的 Linux 开发工具

### 方案 2: 使用传统 `#include`
可以创建一个分支，将 `import std;` 替换为传统的 `#include` 头文件，但会失去模块的优势（编译速度、隔离性等）

### 方案 3: 等待工具链成熟
C++23 模块是新特性，工具链支持还在完善中，预计 2025 年会有显著改善

## 参考资料

- [Clang C++23 模块文档](https://clang.llvm.org/docs/StandardCPlusPlusModules.html)
- [libc++ 文档](https://libcxx.llvm.org/)
- [xmake C++ 模块支持](https://xmake.io/#/guide/project_examples?id=c20-modules)
- [MSVC C++23 支持](https://learn.microsoft.com/en-us/cpp/overview/cpp-conformance-improvements)

---

*最后更新: 2025-12*

