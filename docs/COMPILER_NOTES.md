# 编译器与平台状态

更新日期：2026-10-10。

cppx 的产品目标是跨平台通用 C++23 基础库。目前的源码使用命名模块和 `import std`，当前验证平台仍为 Windows x64 + MSVC。

## 已验证组合

| 组件 | 本地审查使用的版本 |
|---|---|
| Visual Studio | 2022 Community 17.14.41 |
| MSVC 工具集 | 14.44.35207（19.44 系列） |
| 标准库模块 | 同一工具集提供的 `std.ixx` |
| xmake | 3.1.1+HEAD.3ba37a0d4 |
| 构建模式 | Debug、Release，x64 |
| clang-format | 18.1.8 |

CI 配置使用 `windows-2022` 和 xmake `3.1.1`。托管 runner 中的 MSVC 会更新；`xmake check-toolchain` 输出实际编译器/标准库版本宏、阶段日志与 JSON 报告，作为构建证据保存。

## 待验证组合

| 平台 | 拟验证工具链 | 状态 |
|---|---|---|
| Linux x64 | Clang + libc++ | 尚未建立支持基线 |
| Linux x64 | GCC + libstdc++ | 尚未建立支持基线 |
| macOS arm64 | Apple Clang + SDK libc++ | 尚未建立支持基线 |
| Windows arm64 | MSVC | 后续扩展 |

当前已配置 Linux/Clang 18 + libc++、Linux/GCC 14、macOS/Apple Clang 的独立探针 job；还未取得远端运行结果。探针和消费者用法见 [xmake 接入文档](XMAKE_INTEGRATION.md)。

“未验证”描述 cppx 的项目状态，不代表对应编译器普遍不支持 C++23 或 modules。具体版本和功能支持应通过最小探针与完整测试确定，不能沿用旧文档的年份预测。本次本地环境没有 WSL/Linux 或 macOS，未执行这些平台的构建。

## 模块与分发约束

命名模块、`import std`、标准库功能和构建系统的依赖扫描是不同的兼容性维度。需要同时验证：

1. `std::expected`、`std::format` 等所需 C++23 API。
2. 标准库模块来源及对应编译选项。
3. 多个 cppx 模块的依赖顺序、增量重编译。
4. 独立消费者能否使用安装后的库与模块接口。

BMI/IFC 依赖编译器、标准库、编译选项和构建模式，不能当作跨工具链的通用分发格式。裸 `cl main.cpp cppx.lib` 不包含模块依赖配置，不能作为完整接入说明。

跨平台实现与可选模块接口的方向见 [ADR-0001](decisions/0001-cross-platform-baseline.md)，执行顺序见 [ROADMAP](ROADMAP.md)。

