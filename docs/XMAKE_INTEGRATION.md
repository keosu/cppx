# xmake 接入、分发与验证

cppx 使用 xmake/xrepo 统一构建生态，最低 xmake 版本为 3.1.1。当前发行接口为 C++23 命名模块及两个公开宏头文件；静态库与模块源码一起交付。

## 构建选项

| 选项 | 单独构建 cppx | 通过 includes 嵌入 |
|---|---|---|
| `cppx_build_tests` | 默认开启 | 默认关闭 |
| `cppx_build_examples` | 默认开启 | 默认关闭 |

```sh
xmake f -c -m release --cppx_build_tests=n --cppx_build_examples=n
xmake build cppx
```

`-c` 清理旧配置；显式配置会覆盖默认值。嵌入时库不设置宿主项目名、全局版本或全局语言/优化选项。静态库 target 自行声明 C++23；消费者 target 也应声明所需语言标准。

## 源码接入

```lua
set_project("myapp")
add_rules("mode.debug", "mode.release")
includes("third_party/cppx")

target("myapp")
    set_kind("binary")
    set_languages("c++23")
    set_policy("build.c++.modules", true)
    add_cxxflags("/utf-8", "/Zc:preprocessor", {tools = "cl"})
    add_files("main.cpp")
    add_deps("cppx")
target_end()
```

公共模块由同一构建图生成。使用自带宏时包含 `test_macros.h` 或 `serialization_macros.h`，通过 target 的 include 路径查找。

## 本地包接入

在 cppx 根目录生成包：

```sh
xmake f -c -m release --cppx_build_tests=n --cppx_build_examples=n
xmake package -o build/local-repository cppx
```

输出结构为 `build/local-repository/packages/c/cppx/`，内含原生 xmake 配方与对应平台/架构/模式的产物。安装内容包含静态库、宏头文件、许可证，以及 xmake 模块目录中的 `.cppm` 与 `.meta-info`。

消费者的 xmake.lua：

```lua
set_project("myapp")
add_rules("mode.debug", "mode.release")
add_repositories("cppx-local .", {rootdir = "/absolute/path/to/cppx/build/local-repository"})
add_requires("cppx", {system = false, configs = {debug = is_mode("debug")}})

target("myapp")
    set_kind("binary")
    set_languages("c++23")
    set_policy("build.c++.modules", true)
    add_cxxflags("/utf-8", "/Zc:preprocessor", {tools = "cl"})
    add_files("main.cpp")
    add_packages("cppx")
target_end()
```

本地包必须匹配编译器工具集、标准库、架构、运行库和 Debug/Release 配置。需要 Debug 时先生成相应模式的包。BMI/IFC 由消费者生成，静态库仍受工具链 ABI 约束；不能把这个本地包视为跨编译器通用二进制。

通过 `rootdir` 传入仓库目录可支持带空格的路径；不要把带空格的绝对路径直接拼入 `add_repositories` 的字符串参数。

xmake 3.1.1 的 local package 流程跳过了规则安装钩子，且未向通用 installfiles 传入输出路径。cppx 的 `before_package` 为 local 格式补齐安装目录并调用已有规则的安装准备钩子，复用 xmake 的模块元数据生成机制。没有自行维护另一套模块元数据格式。

后续分发将提供带版本校验的 xrepo 源码配方、私有仓库示例和发布流程。

## 工具链能力探针

```sh
xmake check-toolchain --require=all
xmake check-toolchain --toolchain=msvc --mode=debug
xmake check-toolchain --toolchain=clang --sdk=/usr/lib/llvm-18 --runtimes=c++_shared --require=cpp23
```

探针分别配置、编译、运行以下项目：

1. `cpp23`：普通标准头文件中的 expected、format、ranges，并输出编译器/标准库版本宏。
2. `named_module`：自定义命名模块的导出、导入和链接；显式关闭标准库模块，独立检测语言模块能力。
3. `import_std`：标准库模块及其 expected、format、容器与算法。

`--require=all` 要求三项全部成功；`--require=cpp23` 只把常规 C++23 作为退出码门槛，另两项仍如实记录；`--require=none` 仅采集结果，不能作为支持平台的验收。命令使用本机工具链，也可通过 CC/CXX 和 SDK 指定编译器；当前不提供交叉编译后跳过运行的“通过”模式。

默认输出为 `build/toolchain-check/report.json`，使用 `--output=目录` 可修改。每次运行创建新工作目录，独立配置和构建；报告记录每项配置/构建/运行阶段的退出码与 stdout/stderr 日志路径。标准 API 探针的运行日志包含实际编译器和标准库版本信息。

## 独立消费者验证

```sh
xmake verify-consumer --mode=debug
xmake verify-consumer --mode=release
```

任务复制当前库的分发输入及消费者 fixture 到全新目录，显式设置子进程工作目录、配置目录与构建目录。这样不会依赖仓库原有对象文件或修改仓库构建配置。

源码消费者验证 `includes + add_deps`；包消费者只通过本地仓库和 `add_packages` 接入。两个消费者都导入全部 10 个模块，运行多翻译单元调用、模板、公开宏、简单并发和序列化检查，同时断言没有引入开发 target 或覆盖宿主项目名/版本。

默认报告为 `build/consumer-check/report.json`。配置或构建失败就停止该消费者后续步骤；任一消费者或打包失败，任务返回非零。每个配置/构建阶段超时五分钟，运行阶段超时一分钟。每次运行的目录与日志保留供排查，后续可按保留周期清理 build 下旧产物。

## 当前验证范围

2026-10-10 本地验证环境：Windows x64、VS 2022 17.14.41、MSVC 工具集 14.44.35207、xmake 3.1.1+HEAD.3ba37a0d4。探针输出 `_MSC_FULL_VER=194435229`、`_MSVC_STL_UPDATE=202503`。

| 检查 | 结果 |
|---|---|
| C++23 / 命名模块 / import std | 三项均配置、编译、运行成功 |
| 不存在的工具链 + `--require=all` | 三项均失败，任务非零退出，报告记录失败 |
| Debug / Release 全量构建与测试 | 库、11 个测试程序和 10 个示例构建通过；两种模式各 11/11 测试通过 |
| Release 独立源码 / 本地包消费者 | 两条接入路径均构建、运行通过 |
| Debug 独立源码 / 本地包消费者 | 两条接入路径均通过，验证目录包含空格 |
| 根项目配置隔离 | 探针及消费者运行前后，根配置文件 SHA-256 一致 |
| 格式与差异检查 | 39 个 C++ 文件通过 clang-format 18.1.8；`git diff --check` 通过 |

能力探针报告位于 `build/toolchain-check/report.json`，Release 消费者报告位于 `build/consumer-check/report.json`，本次 Debug 带空格路径验证报告位于 `build/consumer check debug/report.json`。这些生成文件不纳入版本控制；CI 按运行归档日志与报告。

Windows/MSVC 的上述结果属于本地验证。Linux/Clang 18、Linux/GCC 14、macOS/Apple Clang 已加入 CI 探针配置，但尚未获得远端运行结果；它们还不是完整 cppx 的支持平台。当前也未覆盖模块接口变更后的消费者增量重编译、不同工具链二进制混用或共享库分发。

CI 对 Windows 要求三项能力及 Debug/Release 消费者通过；其他平台现阶段仅要求常规 C++23 探针通过。CI 上传 JSON 与日志，不把非必需模块探针失败描述成支持。
