# AGENTS.md 文档权限更新实施计划

> **执行要求：** 使用 `superpowers:executing-plans` 按步骤实施。本任务只修改文档，不使用子代理（subagent）。

**目标：** 更新项目根目录的 `AGENTS.md`，允许在用户明确要求时修改 `Docs` 及其子目录中的全部 Markdown（`.md`）文件。

**方案：** 保留现有工作职责、禁止事项、进度判断标准和输出格式，只重写文档权限段落并规范 Markdown 标题缩进。通过文本搜索与 Git（版本控制）差异验证权限边界，没有编译步骤。

**技术栈：** Markdown、PowerShell、ripgrep（快速文本搜索工具，`rg`）、Git（版本控制）

---

### 任务 1：更新文档权限规则

**文件：**

- 修改：`D:\UE\project\C++Project\Lyra\AGENTS.md`
- 参考：`D:\UE\project\C++Project\Lyra\Docs\AGENTS-Document-Permissions-Design.md`

- [x] **步骤 1：保留既有规则并规范标题格式**

  删除文件开头及各节的多余列表缩进，使 `## 工作职责`、`## 禁止事项`、`## 文档权限`、`## 进度判断标准` 和 `## 建议输出格式` 成为正常的二级标题。

- [x] **步骤 2：用以下内容替换“文档权限”段落**

```markdown
## 文档权限

只有用户明确要求整理或更新文档时，才允许创建、修改和整理：

`D:\UE\project\C++Project\Lyra\Docs`

权限递归覆盖该目录及其子目录中的所有 Markdown（`.md`）文件。

以下内容仍不得修改：

- `Docs` 中的非 Markdown 文件
- `Docs` 目录之外的项目文件
- Unreal Engine（虚幻引擎）源码
- LyraStarterGame 原项目文件

除非用户另行明确授权，不得扩大上述可写范围。
```

- [x] **步骤 3：保存为 UTF-8（Unicode 转换格式 8 位）编码**

  重新读取文件，确认中文没有乱码。

### 任务 2：验证权限更新

**文件：**

- 验证：`D:\UE\project\C++Project\Lyra\AGENTS.md`

- [x] **步骤 1：确认旧限制已移除**

```powershell
rg -n "Docs\\LyraReplication|ROADMAP\.md|PROGRESS\.md|SOURCE_MAP\.md|DECISIONS\.md|OPEN_QUESTIONS\.md|LEARNING_NOTES\.md" AGENTS.md
```

预期结果：没有匹配项，命令退出码为 `1`。

- [x] **步骤 2：确认新权限完整存在**

```powershell
rg -n "D:\\UE\\project\\C\+\+Project\\Lyra\\Docs|所有 Markdown|非 Markdown|不得扩大" AGENTS.md
```

预期结果：四类规则均有匹配项。

- [x] **步骤 3：检查文件内容和 Git 差异**

```powershell
Get-Content -Raw -Encoding UTF8 AGENTS.md
git diff -- AGENTS.md
git status --short -- AGENTS.md Docs/AGENTS-Document-Permissions-Design.md Docs/AGENTS-Document-Permissions-Plan.md
```

预期结果：中文可正常读取；修改仅涉及授权的 Markdown 文档，没有源代码、配置或资产变更。

- [x] **步骤 4：报告结果，不创建 Git 提交**

  列出修改文件、权限变化及验证结果。除非用户另行要求，不执行 `git add` 或 `git commit`。
