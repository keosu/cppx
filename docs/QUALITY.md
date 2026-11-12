# 质量与性能验证

所有脚本需要 Python 3.10+ 和 xmake 3.1.1+。输出目录必须不存在；脚本复制输入到独立目录，记录输入 SHA-256、实际命令、退出码、超时和日志。失败返回非零，途中也会更新报告。原工作区构建配置不被更改。

```sh
python scripts/verify.py --output build/quality-release --seconds 60 --iterations 10000
python scripts/verify.py --output build/quality-debug --mode debug --seconds 60
python scripts/verify.py --output build/quality-asan --sanitizer address --seconds 60
python scripts/verify_components.py --output build/quality-components
```

验证包含 13 个测试程序（12 个常规测试和一个故障测试）、固定语料/变异、2,004 个 Python 标准库 JSON 差分用例、并发压力，以及故障构建禁止安装/打包的反向检查。`report.json` 是逐阶段结果，`junit.xml` 可由 CI 消费。通过率不等于代码覆盖率。

## 动态检测与故障边界

sanitizer 首先运行正常输入，再执行一个明确的缺陷：ASan 堆越界、UBSan 有符号溢出或 TSan 数据竞争。只有进程异常退出且诊断与目标缺陷匹配，负对照才算通过。未能编译、缺失运行库或没有检出缺陷都视为失败，不记为通过或静默跳过。库模块和测试均使用所选检测器。

MSVC 支持本项目的 address 配置；UBSan/TSan 任务选择 Linux Clang/libc++。Windows ASan 已有本地运行证据，Linux sanitizer CI 的新增任务仍待当前改动的远端运行结果。

`cppx_fault_injection` 默认为关闭，仅独立测试构建可用。注入状态为调用线程局部、一次性，测试控制第 N 次线程创建前抛出 `resource_unavailable_try_again`；已经启动的真实线程仍需被正常回收。文件注入设置流错误位，二进制短读则在获得长度后移到 EOF 触发实际短读。测试验证失败返回、原目标文件保留、临时文件清理、定时器恢复与日志异常保留。

这些检查动态覆盖错误处理分支，但不声称制造了操作系统资源耗尽、真实磁盘满、断电或底层设备故障。相关端到端试验仍应在隔离环境进行。

## 输入检查

`quality/corpus/` 保存 UTF-8、代理对、整数极值、深度、非法 JSON、数值、时间和 CLI 种子。默认固定 seed 为 20261012，文件按名字排序，插入/删除/替换字节的过程可复现。JSON 成功解析后必须满足规范输出的再次解析/输出一致；数值和时间检查往返契约。差分测试用 Python `json` 生成可表示的整数、浮点数、Unicode、对象和数组，再与 cppx 输出逐项比较，避免只依靠自身往返。

每次执行变异输入前把原始字节写入 `fuzz-reproducer.bin`；成功完成后删除。失败时保留文件和 seed。把失败文件单独放入一个 corpus 目录，再运行 `xmake run fuzz_inputs CORPUS 1 SEED FAILURE_FILE` 可以重现种子处理；确认修复后将文件以描述性名称加入固定语料。该驱动提供 `LLVMFuzzerTestOneInput`，但目前自动化使用确定性变异，未声称已经完成覆盖率引导的 libFuzzer 验证。

## 并发与长期运行

压力负载反复检查 channel 多读写者关闭/背压后的接收计数与校验和、线程池提交和关闭竞态、双线程停止 timer、日志级别切换/轮转和原子写。脚本为进程设置有界超时，超时会终止本次进程树并标记失败。

每秒向 `resources.jsonl` 记录时间、完成轮数、工作集/RSS、句柄/文件描述符数和线程数。Windows 和 Linux 采集当前资源；其他平台返回 0 表示尚未采集，不能据此作出资源结论。Linux 文件描述符数量包含枚举目录时打开的描述符。ASan 自身的隔离区会显著影响资源曲线，不能与普通构建直接比较。

在专用环境运行至少 24 小时：

```sh
python scripts/verify.py --output build/soak-24h --seconds 86400 --iterations 200000
```

保持进程连续运行且禁止机器休眠，记录机器/系统/编译器、后台负载、开始结束时间、输入规模、崩溃/超时与资源曲线。检查稳态后的 RSS/句柄/线程数是否持续增长，并解释所有丢失或拒绝的工作。短时运行及每日 1 小时 CI 不替代 24 小时验收；当前没有把该项目标为完成。

## 基准

```sh
python scripts/benchmark.py --output build/performance --operations 20000 --repeats 5
```

使用 Release 的生产构建，关闭 sanitizer 和故障钩子。JSON parse+dump、两线程池的单个在途任务 submit/get、双 channel 往返、文件日志 write+flush 分别预热 100 次，报告吞吐、逐次延迟 P50/P95/P99、分配次数/请求字节数和进程峰值驻留内存。报告同时包含输入字节数、编译器和硬件线程数；完整构建选项保存在日志。

分配统计覆盖进程中经可替换 C++ `new/new[]`（含对齐版本）发生的分配，不含直接 malloc 或系统分配；统计本身存在开销。峰值内存为整个进程累计峰值，不是每个函数的独占内存。日志 flush 指用户态流刷新，不代表持久化到介质。

脚本报告多次运行的 min/median/max 与相对范围。现阶段先测量噪声，不设置未经专用机器基线校准的回归阈值，也不把微基准视为真实产品吞吐。
