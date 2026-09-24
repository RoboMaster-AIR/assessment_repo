# 公开输入数据

数据按题号分目录，候选人只读取自己所选题目的目录。

| 目录 | 文件 | 用途 |
|---|---|---|
| `F1/` | `F1_single_video.mp4`, `F1_multi_video.mp4`, `F1_lightbar_candidates.csv` | 已提取灯条候选后的单车/多车装甲板关联与遮挡判定；视频不是要求重写检测器 |
| `F2/` | `F2_observations.csv` | 多候选、丢失和重捕获跟踪 |
| `F3/` | `F3_target_snapshots.jsonl` | 连续多目标锁定和切换 |
| `F4/` | `F4_ballistic_cases.jsonl` | 弹道有效性和边界 |
| `F5/` | `F5_latency_sequence.csv` | 处理时延和飞行时间补偿 |
| `F6/` | `F6_detections.jsonl`, `F6_tracking.jsonl`, `F6_commands.jsonl`, `F6_public_issue_times.json` | 日志复盘 |

这些文件是公开输入，不包含算法组真值。隐藏测试会更换部分数值、顺序或故障窗口。

F1 的 CSV 只包含匿名灯条候选：`frame,timestamp,side,cx,cy,length,angle_deg,brightness,color`。其中 `color` 为 `0=red`、`1=blue`；同一帧的多行需要先分组，再交给 `ArmorAssociator`。公开文件不含 `target_index`、`armor_id` 或遮挡真值。
