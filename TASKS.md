# F1-F6 正式题总览

F 题不是 T 题的连续升级。每题都包含自己的输入、接口和测试，任选一题即可完成。

详细情景、最低行为契约和验收方式见 [`docs/F_TASKS.md`](docs/F_TASKS.md)。题面中的代码入口均指向本仓库的 starter 接口，不要求访问完整 `air_vision_27`。

| 题目 | 核心能力 | 数据 | 主要接口 |
|---|---|---|---|
| F1 | 多车遮挡下的装甲检测与配对 | `data/F1/*.mp4` | `src/f1_detector.hpp` |
| F2 | 丢失、预测、重捕获和释放 | `data/F2/F2_observations.csv` | `src/f2_tracker.hpp` |
| F3 | 连续多目标锁定、滞回和安全释放 | `data/F3/F3_target_snapshots.jsonl` | `src/f3_target_manager.hpp` |
| F4 | 弹道边界、无解和输入契约 | `data/F4/F4_ballistic_cases.jsonl` | `src/f4_ballistic_solver.hpp` |
| F5 | 端到端时间语义和命中点预测 | `data/F5/F5_latency_sequence.csv` | `src/f5_latency_predictor.hpp` |
| F6 | 日志对齐、故障定位和禁火检查 | `data/F6/` 三份日志 | `src/f6_replay_analyzer.hpp` |

## 共同提交物

- 修改后的 starter 模块；
- 自动化测试；
- 一页设计说明，写清状态、阈值、单位和失败处理；
- 一条可复制的构建/测试命令；
- 一份公开数据运行结果。

组织者会使用不同顺序、噪声或隐藏数据重新运行，不接受按公开样本写死答案的实现。
