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

已提供 `python scripts/package.py --output build/release-candidate --verify`：生成确定性源码包、SHA-256 与固定版本私有 xrepo 配方，并在隔离安装目录验证完整消费者。详见 [发布政策与接入示例](RELEASE_POLICY.md)。

## 工具链能力探针

```sh
xmake check-toolchain --require=all
xmake check-toolchain --toolchain=msvc --mode=debug
xmake check-toolchain --toolchain=clang --sdk=/usr/lib/llvm-18 --runtimes=c++_shared --require=cpp23
```

探针分别配置、编译、运行以下项目：

1. `cpp23`：普通标准头文件中的 expected、format、ranges、ios::noreplace，并输出编译器/标准库版本宏。
2. `named_module`：自定义命名模块的导出、导入和链接；显式关闭标准库模块，独立检测语言模块能力。
3. `import_std`：标准库模块及其 expected、format、容器与算法。

`--require=all` 要求三项全部成功；`--require=cpp23` 只把常规 C++23 作为退出码门槛，另两项仍如实记录；`--require=none` 仅采集结果，不能作为支持平台的验收。命令使用本机工具链，也可通过显式 `--cc` / `--cxx` 和 `--sdk` 指定编译器（不能只凭 CI job 名称或 CC/CXX 环境变量认定实际版本）；当前不提供交叉编译后跳过运行的“通过”模式。

默认输出为 `build/toolchain-check/report.json`，使用 `--output=目录` 可修改。每次运行创建新工作目录，独立配置和构建；报告记录每项配置/构建/运行阶段的退出码与 stdout/stderr 日志路径。标准 API 探针的运行日志包含实际编译器和标准库版本信息。

## 独立消费者验证

```sh
xmake verify-consumer --mode=debug
xmake verify-consumer --mode=release
xmake verify-consumer --mode=release --toolchain=clang --sdk=/usr/lib/llvm-18 --runtimes=c++_shared
```

任务复制当前库的分发输入及消费者 fixture 到全新目录，显式设置子进程工作目录、配置目录与构建目录。这样不会依赖仓库原有对象文件或修改仓库构建配置。

源码消费者验证 `includes + add_deps`；包消费者只通过本地仓库和 `add_packages` 接入。两个消费者都导入全部 10 个模块，运行多翻译单元调用、模板、公开宏、简单并发和序列化检查，同时断言没有引入开发 target 或覆盖宿主项目名/版本。

默认报告为 `build/consumer-check/report.json`。配置或构建失败就停止该消费者后续步骤；任一消费者或打包失败，任务返回非零。每个配置/构建阶段超时五分钟，运行阶段超时一分钟。每次运行的目录与日志保留供排查，后续可按保留周期清理 build 下旧产物。

## 可选组件与开发工具

默认包含全部 10 个模块。`cppx_components=logging,serialization` 只构建选中的模块及其传递依赖；未知名字报错。生产消费者默认不构建 tests/examples/quality/benchmarks。直接在 cppx 根目录构建部分组件时：

```sh
xmake f -c -y --cppx_build_tests=n --cppx_build_examples=n --cppx_components=logging,serialization
xmake build
```

`cppx_build_quality` 和 `cppx_build_benchmarks` 默认为关闭。`cppx_fault_injection` 仅用于独立测试构建并禁止安装/打包；`cppx_sanitizer` 可选 none/address/undefined/thread。优先使用隔离的 [Python 验证命令](QUALITY.md)，避免混用不同构建模式的模块缓存。

## 当前验证范围

2026-10-12 本地环境为 Windows x64、VS 2022 17.14.41、MSVC 工具集目录 14.44.35207、xmake 3.1.1+HEAD.3ba37a0d4。实际编译器宏为 `_MSC_FULL_VER=194435229`、`_MSVC_STL_UPDATE=202503`。Windows 的源码、本地模块包和新增源码配方均已有独立消费者证据；最新命令、模式、测试数量和产物路径统一见 [STATUS.md](STATUS.md)。

远端旧基线 `f6103fd` 的 Clang 18 三项能力已通过；GCC job 实际运行 13.3，Apple Clang 的标准库模块失败。详情见 [编译器报告](COMPILER_NOTES.md)。本次增加 Linux Clang 完整 Debug/Release、消费者与 sanitizer CI，但尚不能将未运行的新增配置视为当前代码在 Linux 上通过。

升级/回滚演练对基线和候选分别从源码全量构建；源码包验证检查安装后的模块与发布清单一致。当前没有跨工具链二进制混用、共享库分发或真实下游产品的验收承诺。
