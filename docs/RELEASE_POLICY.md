# 版本、交付与发布验收

cppx 当前为 0.1.0 开发版本。正式支持仍限已经完成全库、测试与消费者验证的 Windows x64/MSVC 19.44 组合；Linux Clang 18 是正在验收的下一组合。具体工具版本以 [编译器状态](COMPILER_NOTES.md) 和每次报告为准。

## 版本与兼容性

- 使用语义版本。0.x 的 minor 版本可以包含不兼容变化，必须写入 CHANGELOG 和迁移说明；patch 版本只承载保持公开契约的修复。当前未发布改动归入 Unreleased，不覆盖已发布的同版本归档或哈希。
- 1.0 之后，破坏源码/API 契约的变化进入 major；兼容新增功能进入 minor。弃用使用 `[[deprecated]]` 和迁移示例，至少保留两个 minor 版本且不少于 90 天。
- 不承诺跨编译器、标准库、运行库或构建模式的二进制 ABI。升级时重新编译库、全部消费者和 BMI/IFC，不混用旧缓存。
- 交付源码包及静态库集成方式。共享库未开放，xrepo 配方将 `shared=false` 固定；共享导出与 ABI 验证完成前不得标记为支持。
- 明确使用 `save_json/load_json` 进行序列化文件操作，或导入 `cppx.file_system` 使用 `read_file_text/write_file`。原序列化模块的通用读写辅助函数移入 `json_detail`，消除同时导入两个模块时 `write_file` 的重载冲突。

## 生成与接入源码包

```sh
python scripts/package.py --output build/release-candidate --verify
```

产物包括确定性 `.tar.gz`、`SHA256SUMS`、逐文件 `manifest.json`、固定版本与 SHA-256 的私有 xrepo 配方，以及独立消费者验证日志。不含构建缓存、BMI/IFC 或工作区外文件。包管理器在消费者工具链上构建模块与静态库；验证使用独立下载/安装目录并逐项核对安装后的模块源码。`--mode=debug` 验证 Debug 包；`--toolchain`、`--sdk`、`--runtimes` 可选择其他已安装工具链。

`--base-url=https://your-host/cppx/vVERSION` 指定将来托管归档的目录。脚本只生成文件，不上传。默认配方 URL 指向相应 GitHub Release；上传并完成发布前该 URL 不保证存在。验证专用配方通过临时 loopback HTTP 服务下载同一归档，不能直接分发它。

将生成的 `repository/packages/c/cppx/xmake.lua` 加入你的私有 xrepo 仓库并上传同一归档后，消费者使用：

```lua
add_repositories("company https://your-host/company/xrepo.git")
add_requires("cppx 0.1.0", {configs = {components = "logging,serialization"}})
target("app")
    set_kind("binary")
    set_languages("c++23")
    add_files("main.cpp")
    add_packages("cppx")
```

示例版本需替换为实际已发布版本。依赖组件会自动补齐。所有组件默认启用，tests/examples/quality/benchmarks 在包构建与源码嵌入时均默认关闭。选择部分组件进行独立库构建时，应显式关闭 tests/examples。未知组件直接报错；故障注入构建禁止安装和打包。公开 xmake-repo 上架仍需独立提交与审核。

## 发布清单

1. 从准备发布的固定 commit 构建，确认源码/宏头/API 版本一致；完成 CHANGELOG、核心契约与迁移说明。
2. 对每个承诺平台运行 Debug/Release 全量测试、源码/本地包消费者和 `package.py --verify`；保存实际编译器版本与全部报告。
3. 通过 sanitizer 正/负对照、边界/故障测试、固定语料与差分检查；按 [质量验证](QUALITY.md) 完成专用环境的 24 小时运行和资源曲线分析。
4. 重复性能测量，给出机器、编译选项、输入规模与波动区间。共享 runner 结果不充当发布性能门槛。
5. 真实下游产品完成集成、数据迁移、升级及回滚验收。对 Unicode、整数精度、错误处理、线程停止等行为变化采用产品自己的历史数据。
6. 确认归档、版本、SHA-256 与报告对应；上传固定归档后才发布标签、更新包配方及提交 xmake-repo。保留上一版源码包和报告以便回滚。

## 升级与回滚演练

```sh
python scripts/verify_upgrade.py --baseline HEAD --output build/upgrade-exercise
```

该脚本从本地 Git 基线生成归档，依次在全新目录构建“旧版本 → 当前工作区 → 旧版本”的独立消费者。三次运行共享一个持久化 JSON 文件并校验状态推进；每次重编译模块，没有复用升级后的 BMI。报告记录基线 commit 和候选文件哈希。

内置演练只覆盖共同的 0.1 API 和 ASCII/小整数 JSON schema，证明工程上的重建与回退流程。它不能替代真实产品的数据兼容验收；旧实现不保证能正确读取修复后写出的完整 uint64/Unicode 数据。实际回滚应先停止写入、备份数据、恢复旧归档与匹配工具链、重建所有消费者，再用历史数据验收后恢复服务。出现不可逆 schema 变化时必须使用明确的数据恢复方案。

## 缺陷响应

缺陷报告应包含固定版本/commit、操作系统、编译器/标准库/xmake 版本、最小输入、复现命令和日志。内存破坏、数据损坏、死锁按 P0/P1 优先调查；保存复现输入并加入回归后再关闭。维护者在 issue 中记录负责人、影响版本、修复版本和临时规避方式；当前项目不承诺商业 SLA。涉及凭据或未公开漏洞细节时不要把它们放进公开日志。
