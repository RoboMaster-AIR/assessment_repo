# RoboMaster 算法组正式考核仓库

这是给候选人的独立 starter repository，不是 `air_vision_27` 的公开副本。

候选人只会拿到：

- 本仓库的题目接口、数据结构和最小测试入口；
- 本仓库 `data/F1` 到 `data/F6` 中与所选题目对应的公开输入；
- 所选 F 题的题面。

候选人不会拿到：

- `air_vision_27` 的完整源码；
- `assessment_data/organizer_only/`；
- 组织者的参考实现、真值和评分脚本。

## 选择题目

F1-F6 六道题完全独立，任选一道。先读 `TASKS.md`，再只打开所选题目的 `src/f*.hpp`、对应数据目录和测试目录。不要修改其他题目的模块。

## Fork、分支和 PR 流程

1. 在组织者提供的 Git 服务页面 fork 本仓库，不要直接向主仓库推送。
2. 从默认分支创建自己的题目分支，例如 `candidate/f3-target-manager`。
3. 只完成一题，提交源码、测试、设计说明和运行结果。
4. 将分支推送到自己的 fork，并向组织者主仓库发起 Pull Request。
5. PR 标题使用 `F3: target manager hysteresis` 这类格式；PR 描述必须写明题号、改动文件、测试命令和已知限制。
6. 组织者会在隐藏测试环境中重新运行 PR，不接受把隐藏答案、绝对路径或原始 `air_vision_27` 源码提交进来。

PR 模板位于 `.github/PULL_REQUEST_TEMPLATE.md`。如果只想讨论设计，可以先开 Draft PR；正式提交前必须让本地 `ctest` 通过。

## 构建

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j2
ctest --test-dir build --output-on-failure
```

题目允许使用 C++17 标准库；不得要求 ROS、GPU、工业相机 SDK 或 `air_vision_27` 才能运行。

## 数据位置

数据已经按题目放在本仓库的 `data/` 下。候选人的程序必须从命令行参数读取输入路径，不得写死组织者工作区路径。
