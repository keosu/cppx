# Scripts

本目录包含 xmake 任务的实现脚本。

构建生态验证任务：

- `xmake check-toolchain`：独立验证 C++23、命名模块、import std，保存 JSON 与日志。
- `xmake verify-consumer`：验证源码及本地模块包的独立消费者。
- `check_support.lua`：为以上任务提供独立工作目录、子进程超时和报告工具。

完整参数和接入方式见 [xmake 接入与验证](../docs/XMAKE_INTEGRATION.md)。

## 文件说明

### `format.lua`

代码格式化任务的实现。

**功能**：
- 使用 clang-format 格式化所有 C++ 源文件
- 支持检查模式（`--check`）和格式化模式

**使用**：
```bash
# 在项目根目录执行
xmake format          # 格式化所有文件
xmake format --check  # 检查格式
```

**在 xmake.lua 中引用**：
```lua
task("format")
    on_run("scripts.format")
task_end()
```

## 添加新脚本

如果需要添加新的任务脚本：

1. 在此目录创建 `your_task.lua`
2. 实现 `main()` 函数
3. 在 `xmake.lua` 中添加任务：
   ```lua
   task("your_task")
       on_run("scripts.your_task")
   task_end()
   ```

**示例**：
```lua
-- scripts/your_task.lua
function main()
    import("core.base.option")
    
    print("Your task is running...")
    -- 实现你的任务逻辑
end
```

