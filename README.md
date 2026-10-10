# cppx

面向跨平台产品的 C++23 通用基础库，提供错误处理、字符串、文件系统、日志、时间、并发、数学、命令行、JSON 序列化与测试工具。

当前版本为 **0.1.0，工程化建设阶段**。现有实现采用 C++ modules 和 `import std`，当前验证平台是 Windows x64 + MSVC；Linux、macOS 的支持正在规划中。API、线程安全和分发能力仍在完善，尚未达到 1.0 的生产验收标准。

本项目没有第三方运行时依赖。测试通过率不等于代码覆盖率，当前尚无经测量的覆盖率报告。

**构建与分发统一使用 xmake/xrepo 生态。** 目前保留命名模块接口，通过工具链探针验证跨平台能力。

## 建设方向

- [当前开发状态](docs/STATUS.md)：最近检查点、已完成验证和恢复工作的顺序。
- [项目评估](docs/PROJECT_ASSESSMENT.md)：架构、已复现缺陷、风险优先级与验证边界。
- [分阶段路线](docs/ROADMAP.md)：每个阶段的交付物、依赖和验收条件。
- [跨平台技术方向](docs/decisions/0001-cross-platform-baseline.md)：工具链矩阵、公共接口与模块分发的选择。
- [贡献与验证](CONTRIBUTING.md)：构建、回归测试和提交要求。
- [xmake 接入与验证](docs/XMAKE_INTEGRATION.md)：源码接入、本地模块包、工具链探针和独立消费者。

## 当前构建基线

| 项目 | 当前基线 |
|---|---|
| 平台 | Windows x64 |
| 编译器 | MSVC 19.44 / Visual Studio 2022 17.14 系列 |
| 标准库 | 对应 MSVC STL，含 `std.ixx` |
| 构建工具 | xmake 3.1.1 或更新版本；CI 固定 3.1.1 |
| 语言 | C++23；具体编译选项由 xmake 选择 |
| 格式工具 | clang-format 18 |

较新的工具链需要通过相同验证后才能加入支持矩阵。有关平台状态，参见 [编译器说明](docs/COMPILER_NOTES.md)。

## 构建与测试

安装 Visual Studio 的“使用 C++ 的桌面开发”工作负载及 Windows SDK，然后在仓库根目录执行：

```powershell
xmake f -c -p windows -a x64 -m debug -y
xmake build -j 2
xmake test

xmake f -c -p windows -a x64 -m release -y
xmake build -j 2
xmake test
```

默认构建库、11 个测试程序和 10 个示例。每个测试程序超时为 60 秒，测试结果以进程退出码判定。`test_runner_contract` 通过独立检查验证测试框架自身的失败检测、SECTION 隔离和断言保留行为。

源码作为子项目嵌入时，测试和示例默认关闭。单独配置库也可使用：

```powershell
xmake f -c --cppx_build_tests=n --cppx_build_examples=n
xmake build cppx
xmake check-toolchain --require=all
xmake verify-consumer --mode=release
```

探针与消费者检查使用独立的配置、构建目录，并生成 JSON 报告和配置/构建/运行日志。

```powershell
# 只构建核心库
xmake build cppx

# 运行一个测试或示例
xmake run test_error
xmake run error_demo
xmake run serialization_demo
```

## 模块

| 模块 | 主要能力 |
|---|---|
| `cppx.error` | `expected<T, E>`、错误上下文、异常转换 |
| `cppx.string_utils` | 分割、拼接、修剪、数值转换和正则 |
| `cppx.file_system` | 文件读写、路径操作、临时文件与目录 |
| `cppx.logging` | 日志级别、控制台、文件输出和轮转 |
| `cppx.time_utils` | 时间格式、解析、计时和定时器 |
| `cppx.concurrent` | 线程池、channel、阻塞队列和并行算法 |
| `cppx.math_utils` | 统计、插值、随机数和向量运算 |
| `cppx.cli` | 参数、选项、子命令和帮助信息 |
| `cppx.serialization` | JSON、结构体与枚举序列化 |
| `cppx.test` | 测试注册、断言、SECTION 和报告 |

各模块的已知限制见项目评估；尤其应先修复 JSON 数据保真、时间解析和并发生命周期问题，再将其作为产品基础设施。

## 使用示例

```cpp
import std;
import cppx.error;
import cppx.string_utils;
import cppx.logging;

int main() {
    auto log = cppx::get_logger("myapp");
    auto parts = cppx::split("a,b,c", ",");
    log->info("Parts: {}", cppx::join(parts, " | "));

    auto result = cppx::expected<int, cppx::error_info>::ok(42);
    log->info("Result: {}", result.value());
}
```

支持通过 `includes + add_deps` 源码接入，以及 `xmake package` 生成本地包后通过 `add_repositories + add_requires + add_packages` 接入。后者会使用包内的模块源码和元数据生成消费者需要的接口；单独复制 `cppx.lib` 不能满足 `import cppx.*` 的需求。

```lua
-- 在父项目中包含本仓库，路径按实际布局调整。
includes("third_party/cppx")

target("myapp")
    set_kind("binary")
    set_languages("c++23")
    add_files("main.cpp")
    add_deps("cppx")
    set_policy("build.c++.modules", true)
    add_cxxflags("/utf-8", "/Zc:preprocessor", {tools = "cl"})
target_end()
```

该源码接入方式默认只引入 `cppx` 库 target。完整本地包示例和匹配工具链/构建模式的要求见 [xmake 接入文档](docs/XMAKE_INTEGRATION.md)。当前提供静态库，不提供稳定的 DLL ABI 或跨编译器通用 BMI/IFC 文件。

## 参考文档

- [API 参考](docs/API.md)
- [测试框架](docs/TEST_FRAMEWORK.md)
- [序列化宏](docs/SERIALIZATION_MACROS.md)
- [格式约定](docs/FORMATTING.md)
- [示例源码](examples/)

部分早期 API/设计文档仍需逐项校验；能力与支持范围以当前代码、测试和项目评估为准。

MIT License，见 [LICENSE](LICENSE)。
