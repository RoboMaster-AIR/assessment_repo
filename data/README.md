# 公开输入数据

数据按题号分目录，候选人只读取自己所选题目的目录。

| 目录 | 文件 | 用途 |
|---|---|---|
| `F1/` | `F1_single_video.mp4`, `F1_multi_video.mp4` | 单车和完整简化车体遮挡下的装甲检测 |
| `F2/` | `F2_observations.csv` | 多候选、丢失和重捕获跟踪 |
| `F3/` | `F3_target_snapshots.jsonl` | 连续多目标锁定和切换 |
| `F4/` | `F4_ballistic_cases.jsonl` | 弹道有效性和边界 |
| `F5/` | `F5_latency_sequence.csv` | 处理时延和飞行时间补偿 |
| `F6/` | `F6_detections.jsonl`, `F6_tracking.jsonl`, `F6_commands.jsonl`, `F6_public_issue_times.json` | 日志复盘 |


