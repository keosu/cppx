# cppx 开发状态

检查点日期：2026-10-12。基于 `f6103fd` 的工作区，R01–R09 已完成；本轮继续落实跨平台验证、分发、动态检测、压力、性能和发布工程。

## 已确定的方向

- 跨平台通用 C++23 基础库，保留命名模块接口和 `import std`。
- 构建、测试、安装与分发统一采用 xmake/xrepo。
- 当前版本仍为 0.1.0，不扩大平台支持、稳定 ABI 或 1.0 就绪承诺。

## 阶段状态

| 阶段 | 状态 | 证据与剩余工作 |
|---|---|---|
| M0 可信验证基线 | 已完成 | 测试框架契约及独立自检持续通过 |
| M1 跨平台最小闭环 | 进行中 | Windows 源码/本地包/源码配方消费者；远端 Clang 18 探针已通过，新增完整 Linux CI 尚待运行 |
| M2 核心正确性 | R01–R09 已完成 | 错误模型、JSON、时间、并发、文件、日志和 CLI/数学修复；边界回归与迁移文档 |
| M3 交付与接入 | 本机交付闭环完成，平台验收继续 | 组件依赖闭包、确定性源码包、固定版本 xrepo 配方、独立安装源码校验 |
| M4 稳定性与性能 | 本机动态验证与工具已落地 | 故障注入、MSVC ASan、变异/差分、压力曲线、基准；24 小时及 Linux 动态检测待验收 |
| M5 发布工程与 1.0 | 发布机制和工程演练已落地 | 版本/弃用/ABI/CHANGELOG/升级回滚；真实下游、承诺平台与长期负载仍未满足 |

本次具体变化与使用限制见 [CORE_CONTRACTS.md](CORE_CONTRACTS.md)。

## 修复证据

新增 `tests/test_core_contract.cpp`，使用独立条件检查、编译期断言和进程退出码；没有依赖被测库的测试断言实现。

- 在旧实现上运行初始边界用例，复现 **30 项失败**；修复后清零。
- 扩展覆盖同型值/错误、只能移动的类型、抛异常的移动、void 映射；JSON 整数极值、UTF-8/代理对、非法输入和解析限制。
- 验证 UTC/正负偏移、毫秒、闰日、epoch 前时刻、不同本地时区、DST 空洞/回拨，以及非 NUL 结尾格式视图。
- 验证定时器停止/异常/重启状态、自回调停止与析构、最后一个共享捕获释放所有者；并行任务异常及部分启动时回调捕获失败的清理。
- 验证单线程池内子任务等待、只能移动的任务参数、关闭/排空/队列容量/超时及等待者唤醒；异步检查使用有界等待。
- 验证并发临时文件唯一性、移动清理、keep、非法前缀与创建失败、原子替换/失败清理；日志文件打开失败、轮转顺序/内容和析构期错误记录。
- 验证 CLI 未知选项、统一 choices、help 返回宿主、终止选项；数学整数极值、溢出和空输入。

## 本地验证

环境：Windows x64、VS 2022 17.14.41、MSVC 14.44.35207、xmake 3.1.1+HEAD.3ba37a0d4、clang-format 18.1.8。

核心修复阶段验收：Debug/Release 的库、12 个测试程序及 10 个示例全部构建成功，分别 **12/12 测试程序通过**，最终构建日志没有编译警告。两种模式的源码/本地包消费者全部通过，包内模块源码与工作区一致；CLI 帮助示例返回成功。40 个 C++ 文件通过格式检查，`git diff --check` 通过。验证日志保存在忽略的 `build/` 下：

| 检查 | 产物 |
|---|---|
| Debug 全量构建与测试 | `build/core-debug-build.log`、`build/core-debug-tests.log` |
| Release 全量构建与测试 | `build/core-release-build.log`、`build/core-release-tests.log` |
| Debug 源码与本地包消费者 | `build/core-consumer-debug/report.json` 及对应独立目录内阶段日志 |
| Release 源码与本地包消费者 | `build/core-consumer-release/report.json` 及对应独立目录内阶段日志 |
| 格式与差异检查 | 40 个 C++ 文件的 clang-format 18.1.8 检查、`git diff --check` |

本地第一次切换 Debug 时发现已有 BMI/IFC 缓存的 `_DEBUG` 不匹配，不能作为有效验证。执行 `xmake build -r -j 2` 重建全部模块、测试和示例后警告消失；独立消费者使用全新配置/构建目录。处理方法已加入 CONTRIBUTING。

## 2026-10-12 新增交付与验证

- 修复序列化宏对非 MSVC 的误拒绝，并修复升级演练发现的 serialization/file_system `write_file` 同名重载编译冲突。全模块消费者已增加两参数写文件回归。
- `cppx_components` 自动补齐依赖，最小消费者验证正确的五个模块、宿主项目隔离和未知组件拒绝。
- `package.py` 创建确定性源码包、SHA-256、逐文件清单和固定版本私有 xrepo 配方；验证真实下载、独立构建/运行以及安装模块与源码清单一致。
- 13 个质量测试程序包含一次性线程局部故障注入；覆盖部分线程启动清理、定时器恢复、流错误/短读、原子替换失败清理和日志异常，验证注入构建不能安装或打包。
- 增加固定种子输入变异、2,004 个 Python 标准库 JSON 差分用例、并发压力与 RSS/句柄/线程曲线。
- 新增不启用 sanitizer/故障钩子的生产 Release 基准；五次测量记录吞吐、P50/P95/P99、C++ 分配与峰值工作集。当前机器有其他活动，数据波动较大，未设置发布门槛。
- `verify_upgrade.py` 已完成旧基线 → 当前工作区 → 旧基线的全量重建与持久化 JSON 状态推进。演练采用共同 API 和 ASCII/小整数 schema，不能代表真实产品的历史数据兼容。
- 新增 Linux Clang Debug/Release 完整消费者 CI、Linux ASan/UBSan/TSan 和夜间验证配置；远端旧 GCC job 实际使用 13.3，已改为显式选择 14。详情见 [编译器状态](COMPILER_NOTES.md)。

工程验收命令见 [QUALITY.md](QUALITY.md)，源码交付与发布规则见 [RELEASE_POLICY.md](RELEASE_POLICY.md)。更新后的 Windows 三项工具链探针（含 ios::noreplace）、Debug/Release 源码及本地包消费者均已通过；49 个 C++ 文件通过 clang-format 18.1.8，`git diff --check` 通过。

| 检查 | 证据位置 |
|---|---|
| Release 正常库、12 个测试与 10 个示例；12/12 测试通过 | `build/final-root-build.log`、`build/final-root-tests.log` |
| Debug 13 个质量测试、10,000 次变异、2,004 个差分、60 秒压力 | `build/final-debug-2/report.json`、`junit.xml`、`resources.jsonl` |
| Release 13 个质量测试、20,000 次变异、2,004 个差分、300 秒压力 | `build/final-release-2/report.json`、`junit.xml`、`resources.jsonl` |
| MSVC ASan 正/负对照、13 个测试、10,000 次变异、2,004 个差分、60 秒压力 | `build/asan-verified/report.json`、`junit.xml`、`resources.jsonl` |
| 最小组件、依赖闭包与错误配置 | `build/components-check-2/report.json` |
| Debug / Release 源码与本地包消费者 | `build/final-consumer-debug/report.json`、`build/final-consumer-release/report.json` |
| 更新后的 Windows 三项工具链探针 | `build/final-toolchain/report.json` |
| 五次生产构建基准，每个场景 20,000 次 | `build/final-performance/report.json` 与 `measurement-*.json` |
| 基线 → 工作区 → 基线升级/回滚 | `build/upgrade-check-3/report.json` 与 `state.json` |

机器可读索引见 [本次验证摘要](verification/2026-10-12.json)。三个质量构建中的 src 文件 SHA-256 均与最终工作区一致，最终构建日志没有 C++ 编译警告。Release 300 秒压力中的句柄数始终为 65，线程数从 4 回落到 1，驻留内存约 6.4–7.9 MiB；检查没有发现已接受工作丢失。该短时曲线不能证明长期无泄漏。ASan 构建包含检测器自身的内存开销，单独保留资源报告。

## 验证边界与下一步

通过率不是代码覆盖率。已有 MSVC ASan 正/负对照、动态故障分支、输入变异/差分和短时压力证据；尚无代码覆盖率、覆盖率引导 fuzz、Linux UBSan/TSan、24 小时专用运行或真实产品负载报告。流状态/EOF 注入不能代替真实磁盘满、设备故障或 OS 线程资源耗尽。

Linux/macOS 当前代码的完整构建与独立消费者尚待远端运行。新的 CI 配置已经准备好，未把旧提交的能力探针当作本次源码通过。接下来需要取得 Linux 完整 CI、修复 GCC/macOS 工具链缺口，执行隔离环境的真实故障和 24 小时验收，接入真实下游并完成其数据迁移。公开发布与 xmake-repo 审核仍需按发布清单执行。

源码、宏头文件、库与消费者需一起重新编译，不复用旧版本 IFC/BMI。生成日志和发布候选保存在忽略的 build/ 中；阶段状态不把未执行的外部验收勾选为完成。
