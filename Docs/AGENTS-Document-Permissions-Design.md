# AGENTS.md 文档权限更新设计

## 目标

更新项目根目录的 `AGENTS.md`，使 Codex 可以在用户明确要求时创建、修改和整理 `Docs` 目录及其子目录中的全部 Markdown（`.md`）文件。

## 修改范围

- 将原可写路径 `Docs\LyraReplication` 扩展为 `Docs`。
- 删除仅允许处理六个固定文档名的限制。
- 明确文档权限递归覆盖 `Docs` 的子目录。
- 明确非 Markdown 文件仍不在默认可写范围内。
- 保留源代码、配置、资产、Lyra 原项目和 Unreal Engine（虚幻引擎）源码的只读约束。
- 修正现有 `AGENTS.md` 的多余列表缩进，不改变其他规则含义。

## 完成标准

- `AGENTS.md` 清楚区分可写文档与只读内容。
- 文档权限覆盖 `D:\UE\project\C++Project\Lyra\Docs` 下的全部 `.md` 文件。
- `Docs` 之外的文件不会因本次更新获得额外写权限。
- 文件保持 UTF-8（Unicode 转换格式 8 位）编码，中文可正常读取。

## 验证方式

- 重新读取 `AGENTS.md`，检查路径、扩展名和例外规则。
- 搜索旧路径 `Docs\LyraReplication` 和六个固定文档名，确认旧限制已经移除。
- 查看 Git 差异，确认没有修改源代码或其他项目文件。
