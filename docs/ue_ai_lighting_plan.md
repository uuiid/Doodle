# LLM 驱动的 UE 灯光工具 —— 方案设计与软件架构

> 状态：**方案设计 v3.4**（全部前提已锁定，无阻塞性待确认，见 §15）
> 环境：UE 源码树 `E:\UnrealEngine`（5.7.4，已完整编译）、UE 工程 `E:\doodle_plug_dev_5`、Doodle 插件源码 `E:\Doodle\script\uePlug\Doodle`
> 输入：**人工提示词**（氛围 / 角色性格 / 故事上下文 / 背景设定）+ **多张图片参考**（同场景不同角度、经 AI 或 PS 调整过的目标效果图）+ 可选参考视频
> 输出：对场景灯光、环境光与后处理的增 / 删 / 移动 / 改参，使渲染氛围接近输入意图
> 交互面：**浏览器前端（Kitsu fork）**；UE 侧为**无头执行器**

---

## 1. 核心判断

**判断一：纯 LLM 开环做不到，必须多轮迭代。**
LLM 能可靠决定「要有主光、来自左前方 45°、冷色、硬光、夜景霓虹」这类**语义与结构**，但无法预测「强度 2.3 还是 5.7」这类**连续数值**。渲染链路上还有自动曝光、Lumen 间接光、材质反照率等强非线性环节，任何数值猜测都会被打偏。

**判断二：不让 LLM 直接生成 UE Python/C++ 或直接调 API，而是定义 IR。**
否则不可复现、不可校验、不可 diff、不可回归测试，也无法在没有 LLM 的情况下单测执行器。

**判断三：只有一条执行路径——自动模式；但每一轮的判断由美术人工给出。**
没有 UE 编辑器内的实时调灯。每一次迭代都是：服务器生成候选 → 派发任务 → 工作机跑 UE 渲染 → 结果回到浏览器 → **美术判断** → 下一轮。因此：
- UE 侧是**无头执行器**（commandlet），不需要常驻连接、不需要实时 op 应用、不需要撤销栈
- 浏览器前端是**唯一交互面**，也顺带解决了「浏览器看不到 UE 视口」这个约束——因为不再需要实时视口，美术看的是渲染出来的静帧
- 迭代是**异步、跨会话、可能间隔数小时**的（美术要开会、审片），所以轮次状态必须全部落库

**判断四：参考是「美术调过的目标效果图」，不是物理真值；用显示空间的感知统计去匹配它，是正解而不是妥协。**
参考图是原始渲染经 AI 或 PS 调整后的效果，局部加减光、调色、AI 重绘都破坏了 `E ≈ πL/ρ` 的前提，物理逆渲染会解出错误的辐照度；工程里也没有对应相机，位姿要自己解且同样被 AI 编辑破坏。**但美术调整的恰恰就是「调色板、亮度分布、对比度、色温」这些感知量**——匹配这些量，正是匹配美术的意图。

**判断五：后处理与调色必须在可改范围内，否则参考的一部分目标不可达。**
参考的目标里有一部分（暗角、局部对比、色调曲线、光晕）**只能靠调色实现，靠灯永远够不到**。执行器必须能同时写灯光、环境光、雾、后处理与调色参数。

**判断六：人工判断要变成可用的搜索信号，靠「候选沿语义轴变化」。**
如果每轮只给美术一个结果、只问「行不行」，人工判断只是 yes/no，收敛会慢到不可用。做法是每轮沿**语义可解释的轴**（主光方向 / 光比 / 色温 / 硬软 / 环境光水平 / 调色强度）生成 **6 个**候选，让美术**挑**而不是**调**。美术的选择直接翻译成下一轮的搜索方向——这才是把人工判断变成梯度的关键。

---

## 1.1 输入形态（已澄清）

| 输入 | 实际内容 | 性质 |
|---|---|---|
| **人工提示词**（必有） | 氛围、**人物角色性格**、故事上下文、基本背景与设定 | **权威意图**（人写的，是"要什么"） |
| **图片参考**（必有多张） | 原始渲染图**经 AI 或 PS 调整后的效果**；同场景**不同角度**；**工程里没有对应相机**；**基本没有角色** | **目标效果图**（是"具体什么样"） |
| 参考视频（可选） | 视为「关键帧序列」，并入多图聚合流程 | 同图片 |

---

## 2. 已确认的设计决策

| # | 决策 | 架构影响 |
|---|---|---|
| 1 | 参考为**同场景异视角、经 AI/PS 调整、无相机、无角色、必有多张** | **不尝试物理反解**；走多图感知统计聚合（§5） |
| 2 | **取消 UE 编辑器内实时交互**；统一走自动模式 | 执行器只有一个宿主（commandlet）；无 Slate 面板、无 WebSocket 常驻、无撤销栈 |
| 3 | **每一轮的判断由美术人工给出** | 人工判断是**唯一收敛判据**；指标降级为决策辅助；轮次状态必须落库、可跨会话 |
| 4 | **浏览器前端是唯一交互面** | 前端承担 Spec 裁决、候选挑选、反馈、历史对比、签核（§14） |
| 5 | **LLM 使用云端 deepseek** | 文本与视觉同一把 key（`deepseek-flash` 支持图片输入）；无需本地模型 |
| 6 | **允许修改环境中所有灯光** | 去掉「只动自己管理的灯」护栏；环境光/雾/天空全在可改范围内 |
| 7 | **后处理/调色允许 AI 修改**（已确认） | 执行器需有 grade 写入原语；Spec 要区分"灯光负责"与"调色负责" |
| 8 | **主判据在显示空间**（色调映射后） | 修正 v2 的「线性 EXR 用于指标」规则；EXR 降为工程检查用途 |
| 9 | **每轮固定 6 个候选** | 一次 UE 启动渲染 6 个；渲染时间与美术浏览负担的平衡点 |
| 10 | **只做候选级挑选**，不做逐 op 接受/拒绝 | 判断粒度 = 候选；省掉 N 倍预览渲染 |
| 11 | **反馈滑块与自然语言都支持，滑块为默认** | 滑块给精确轴增量；自然语言给自由表达（由 LLM 结构化） |
| 12 | **美术 4 小时以上不判断算过期** | **不改状态**，只让读模型带出 `is_judgement_overdue`；判断入口永远可用 |
| 13 | **不允许改动几何/材质** | 可改范围锁定：灯光 + 环境光/雾/天空 + 后处理/调色 |
| 14 | **取消 Tier B 多视角联合反解** | 唯一务实的位姿来源（人工对位）被否，只剩自动重定位，收益/成本比不成立（§5.6） |
| 15 | **预设库首批由美术整理** | 三点布光 / 伦勃朗 / 剪影 / 夜景霓虹…；`ADoodleAssetsPreview` 三档作起点 |
| 16 | **参考以全局调色为主** | 全局调色变换可被反解 → grading 类目标有解析热启动，不必靠搜索（§9.2） |
| 17 | **性能护栏暂不设硬上限** | 灯数与单轮渲染时间先不限制；护栏保留为可配置 + 记录实际耗时并告警（§6.6、§12） |

---

## 3. 总体架构

```
┌──────────────────────────────────────────────────────────────────────────┐
│  浏览器前端（唯一交互面）                                                  │
│  提交：人工提示词 + 多张参考图                                             │
└────────────────────────────┬─────────────────────────────────────────────┘
                             │
┌────────────────────────────▼─────────────────────────────────────────────┐
│ ① 感知与目标（主服务器）                                                  │
│  · 提示词 → intent（LLM 归一化 + 角色性格→灯光语言映射）                   │
│  · 多图 → 显示空间感知统计 → 聚合 + 冲突检测 → targets                     │
│  · 合并 → LightingSpec                                                    │
└────────────────────────────┬─────────────────────────────────────────────┘
                             │
              ┌──────────────▼───────────────┐
              │ 人工闸门 1：Spec 确认/冲突裁决 │  ← 浏览器
              └──────────────┬───────────────┘
                             │
┌────────────────────────────▼─────────────────────────────────────────────┐
│ ② 规划与候选生成（主服务器）                                              │
│  · 预设库检索 → 候选骨架                                                   │
│  · 沿语义轴生成 N 个候选 rig（主光方向/光比/色温/硬软/环境光/调色）        │
└────────────────────────────┬─────────────────────────────────────────────┘
                             │  一个任务内含 N 个候选
┌────────────────────────────▼─────────────────────────────────────────────┐
│ ③ 分布式派发（主服务器 → 工作机）                                          │
│  · server_task_info{type_=ai_light, command_={baseline, candidates[]}}     │
└────────────────────────────┬─────────────────────────────────────────────┘
                             │
┌────────────────────────────▼─────────────────────────────────────────────┐
│ ④ UE 无头执行器（工作机，commandlet）                                      │
│  UnrealEditor-Cmd -run=DoodleLighting -Params=<json>                       │
│  · 从基线复制关卡（源资产永不被修改）                                       │
│  · 一次启动，循环：apply_ops(候选 i) → render_preview → 输出                │
│  · 输出 N 组：PNG（显示空间）+ EXR（工程检查）+ 指标                        │
└────────────────────────────┬─────────────────────────────────────────────┘
                             │  回传 N 张渲染图 + 指标 + put_job_info
                             │
              ┌──────────────▼───────────────────────────────┐
              │ 人工闸门 2：候选挑选 / 反馈 / 接受             │  ← 浏览器
              │  · 候选接触表（并排 + 指标辅助）                │
              │  · 接受 → 定稿；选一个继续 → 收缩搜索；给方向    │
              └──────────────┬───────────────────────────────┘
                             │  未接受 → 回到 ②，用反馈生成下一轮候选
                             │  接受 → 定稿
┌────────────────────────────▼─────────────────────────────────────────────┐
│ ⑤ 定稿（唯一有副作用的步骤）                                              │
│  · 把接受的 rig 应用到正式关卡/序列                                        │
│  · 此步必须 snapshot + 可回滚                                             │
└──────────────────────────────────────────────────────────────────────────┘
```

### 3.1 关键形态：异步轮次，不是进程内循环

因为每轮都等人工，迭代**不能是一个进程内的 while 循环**：

- 每一轮是一个**独立的分布式任务**（跑完就 `completed`）
- 「等待人工判断」是**会话状态**（`awaiting_judgement`），**不是任务状态** —— 避免污染 `server_task_info_status` 状态机
- 轮次状态全部落库，可跨会话、跨天恢复
- 需要超时/放弃机制（美术长期不判断 → 会话被标记为过期，前端据此催办）

---

## 4. 数据契约

### 4.1 场景事实 SceneFacts（由 UE 侧 `inventory` 原语产出，喂给 LLM）

```jsonc
{
  "schema_version": "1.0",
  "level": "/Game/.../RenderMap",            // 基线关卡（只读，永不修改）
  "sequence": "/Game/.../LS_xxx",
  "frame_range": [1001, 1200],
  "camera": {
    "name": "Camera_01",                      // 交付机位
    "transform": { "location": [x,y,z], "rotation": [p,y,r] },
    "fov_deg": 35.0,
    "filmback_mm": [36.0, 20.25]
  },
  "subjects": [
    { "id": "char_Ch220B", "kind": "character",
      "bbox_world": {"min":[..],"max":[..]},
      "head_world": [x,y,z] }
  ],
  "scene_bounds": { "min": [..], "max": [..] },
  "lights": [                            // 全场景灯光清单（含美术手工灯）
    { "id": "art_key_01", "type": "spot",
      "actor": "SpotLight_2", "role_guess": "key",
      "intensity": { "unit": "candela", "value": 12.0 },
      "color": { "mode": "temperature", "kelvin": 5600 },
      "transform": { "location": [..], "rotation": [..] },
      "cast_shadows": true, "channels": [1,0,0],
      "mobility": "movable", "source_radius_cm": 20.0 }
  ],
  "exposure": { "method": "manual", "ev100_bias": 0.0, "min": 1.0, "max": 1.0 },
  "postprocess": { "bloom": 0.0, "sharpen": 1.5, "lumen_final_gather": 4,
                   "color_grading": { /* 可写范围 */ } },
  "environment": { "sky_light": {...}, "sky_atmosphere": {...},
                   "height_fog": {...}, "volumetric_cloud": {...} },
  "units": { "length": "cm", "color_space": "linear_srgb" }
}
```

### 4.2 灯光目标 LightingSpec（一等工件）

参考不直接流向评判，而是先被蒸馏成一份显式、可核对、可版本化、可回归测试的目标。

```jsonc
{
  "schema_version": "1.3",
  "spec_id": "uuid", "session_id": "uuid",

  "intent": {                        // 来自人工提示词，权威
    "atmosphere": ["压抑", "暖", "高对比"],
    "time_of_day": "night", "weather": "clear",
    "source_motivation": ["practical_lamp", "window"],
    "character_personality": [
      {"subject": "char_Ch220B", "trait": "阴郁内向",
       "lighting_language": {"key_fill_ratio": [6,10], "hardness": "hard",
                             "position_bias": "side_high", "kelvin_bias": "cool"}}
    ],
    "story_context": "角色在雨夜独自等待，情绪低落",
    "setting": "老式公寓室内，木质家具，窗外霓虹"
  },

  "targets": {                       // 来自多张参考图的显示空间统计，聚合后
    "space": "display_srgb",
    "luma_percentiles": {"p10": [0.02,0.06], "p50": [0.10,0.18], "p90": [0.55,0.75]},
    "palette": [ {"color": [r,g,b], "share": [0.20,0.35]}, ... ],
    "contrast": [0.55, 0.75],
    "white_balance_kelvin": [3400, 4200],
    "clip_rate": {"high": [0,0.02], "low": [0,0.05]},
    "grading_hints": { "vignette": "strong", "lifted_blacks": true, "bloom": "moderate" },
    "ownership": {                   // 每个目标由谁负责达成
      "lighting":  ["luma_percentiles", "white_balance_kelvin", "clip_rate"],
      "grading":   ["palette", "contrast", "grading_hints"],
      "shared":    []
    }
  },

  "reference_stats": {
    "image_count": 4, "aggregation": "median",
    "per_dimension_confidence": {"luma_percentiles": 0.85, "white_balance_kelvin": 0.5},
    "conflicts": ["white_balance_kelvin"],   // 冲突维度 → 交人工裁决
    "retouch_detected": true,
    "usable_for_physical_solve": false       // 显式声明：不做物理反解
  },

  "evidence": { /* 每个字段的来源与置信度 */ },
  "human_confirmed": false,          // 人工闸门 1
  "confirmed_by": null, "confirmed_at": null,
  "degrade_level": "stats_only"      // stats_only | prompt_only
}
```

> **1.2 → 1.3 的变更**：删掉 `tier`，只保留 `degrade_level`。
> Tier B 取消后两者信息等价 —— `degrade_level` 原本的 `full` 值依赖 Tier B 的位姿反解，已不可达，
> 所以它的取值集合塌缩成与 `tier` 相同的 `stats_only | prompt_only`。留两个字段只会让人误以为有区别。

### 4.3 灯光方案 LightingRig（候选 / 定稿 / 会话记录 / 数据库存储）

```jsonc
{
  "schema_version": "1.1",
  "rig_id": "uuid",
  "session_id": "uuid",
  "shot_task_id": "uuid",
  "spec_id": "uuid",                     // 追溯：本 rig 服务于哪份 Spec
  "iteration": 3,
  "candidate_index": 2,                  // 本轮第几个候选
  "variant_axis": {                      // 本候选沿哪条语义轴变化（判断六）
    "axis": "key_fill_ratio", "value": 8.0, "label": "高光比"
  },
  "provenance": { "model": "deepseek-flash", "prompt_hash": "...",
                  "reference": ["ref_01.png","ref_02.png"], "seed": 12345,
                  "tier": "stats_only" },   // 自由文本标注，不参与判定；降级等级以 Spec.degrade_level 为准
  "baseline_level": "/Game/.../RenderMap",   // 本轮起点（只读）
  "globals": {
    "exposure": { "method": "manual", "ev100_bias": -0.3 },
    "white_balance": { "kelvin": 5600, "tint": 0.0 },
    "environment": { "sky_light_intensity": 0.6, "fog_density": 0.02 },
    "grading": { "contrast": 1.08, "saturation": 0.95, "lift": 0.01,
                 "vignette": 0.25, "bloom": 0.3 }
  },
  "lights": [
    {
      "id": "key_01",                    // 稳定语义 id，不用 UE actor 名
      "role": "key",                     // key|fill|rim|ambient|practical|bounce|fx
      "type": "spot",
      "anchor": { "space": "camera", "target": "char_Ch220B",
                  "look_at": "subject_head" },
      "offset": { "azimuth_deg": -45.0, "elevation_deg": 30.0, "distance_m": 3.0 },
      "intensity": { "unit": "candela", "value": 12.0 },
      "color": { "mode": "temperature", "kelvin": 4300 },
      "shape": { "source_radius_cm": 25.0, "ies": null,
                 "inner_cone_deg": 25.0, "outer_cone_deg": 45.0 },
      "shadows": { "cast": true, "softness": 0.6 },
      "channels": [0, 1, 0],             // 项目约定：主光走 Channel1
      "mobility": "movable",
      "anim": [ { "frame": 1001, "intensity": 12.0 },
                { "frame": 1060, "intensity": 18.0 } ],
      "origin": "ai"                     // ai | artist | inherited
    }
  ],
  "ops": [                               // 相对基线（baseline_level）的操作序列
    { "op": "add",    "light": "key_01",  "rationale": "提示要求左前方冷主光" },
    { "op": "modify", "light": "art_fill_01",
      "patch": { "intensity": { "unit": "candela", "value": 3.0 } },
      "rationale": "压缩 key:fill 比到 8:1（本轮变体轴）" },
    { "op": "set_grading", "patch": { "vignette": 0.25 },
      "rationale": "参考有明显暗角，属调色责任（Spec.targets.ownership.grading）" },
    { "op": "remove", "light": "art_rim_02", "rationale": "与参考图无轮廓光" }
  ]
}
```

**为什么 `anchor.space=camera`**：绝对世界坐标对 LLM 无意义（它不知道场景尺度），而「相机左前方 45°、抬高 30°、距离 3m」是电影布光的通用语言，且对场景尺寸鲁棒。执行器负责换算成世界坐标。

**为什么 `id` 与 UE actor 名解耦**：美术会重命名、复制、删除 actor。语义 id 是稳定主键，映射表存在 UE 侧（`Tags` 标记）或 Doodle 数据库。

### 4.4 轮次与人工判断（新增）

```jsonc
{
  "iteration_id": "uuid", "session_id": "uuid", "iteration": 3,
  "spec_id": "uuid",
  "candidates": [
    {"rig_id": "...", "candidate_index": 0, "variant_axis": {...},
     "render": {"png": [...], "exr": "...", "metrics": {...}}}
  ],
  "state": "awaiting_judgement",       // generated|dispatched|rendering|awaiting_judgement|judged
  "judgement": {                       // 人工闸门 2
    "by": "person_uuid", "at": "...",
    "decision": "accept|continue_iteration|reject_all",
    "selected_candidate_id": "...",    // 选哪个作为下一轮基准
    "feedback_text": "主光再低一点，色温再冷一点",
    "feedback_axes": {                 // 结构化反馈（由 LLM 从 feedback_text 提取）
      "key_elevation_delta": -15, "kelvin_delta": -600
    }
  }
}
```

> **两处命名调整**（为了让 DB 存的值与 JSON 一致）：
> - `decision` 的 `"continue"` → `"continue_iteration"`：`continue` 是 C++ 关键字，
>   枚举项只能叫 `continue_iteration`；而本库把枚举按 C++ 名存成 TEXT，
>   所以 JSON 字符串必须跟着改，否则同一份数据在 DB 与 JSON 里是两个值。
> - `selected_rig_id` → `selected_candidate_id`：它指向的是候选的主键
>   （rig 的身份就是候选的身份，没必要两个名字）。

---

## 5. ① 感知层（主服务器）

### 5.1 主路径（Tier A，必须做，**不依赖相机位姿**）

```
多张参考图（已调色）
  ↓ ① 感知统计提取（显示空间）
     调色板分布与占比、亮度分位 p10/p50/p90、对比度、
     色温倾向、高光/暗部裁剪率、暗角/渐变等后处理痕迹
  ↓ ② 多图聚合
     逐维度取中位数/投票 → 目标区间
     维度间冲突大 → 标记为「不确定」，交人工裁决
  ↓ ③ 提示词 → intent（LLM 归一化）
     氛围 / 角色性格 / 故事 / 设定 → 受控词表 + 性格→灯光语言映射（§5.5）
  ↓ ④ 合并 → LightingSpec
  ↓ ⑤ 人工闸门 1（浏览器）
  ↓ 输出 LightingSpec
```

### 5.2 统计量清单

| 统计量 | 用途 | 稳健性 |
|---|---|---|
| 调色板分布与占比（k-means，显示空间） | 主目标之一 | 高（美术调的就是它） |
| 亮度分位 p10 / p50 / p90 | 曝光与对比目标 | 高 |
| 对比度（分位差或标准差） | 对比目标 | 高 |
| 白平衡 / 色温倾向 | 色温目标 | 中（调色会改） |
| 高光/暗部裁剪率 | 曝光余量目标 | 中 |
| 暗角 / 提黑 / 光晕等痕迹 | 归入 `grading_hints` | 中 |
| 受光方向倾向 | 只作**软提示**，不作约束 | **低**（PS 可造假的渐变） |

**已落地**（`doodle_lib/http_method/kitsu/ai_light_stats.{h,cpp}`）：上表除「受光方向倾向」与
「光晕」之外都算了，逐图产出 `reference_image_stats_t` + `stats_confidence_`，随上传写入
`ai_light_reference.stats_` / `stats_confidence_`。实现时定下的三个口径：

- **对比度 = p90 − p10**，不是标准差。锁定的默认目标 `lighting_targets_t::contrast_{0.55, 0.75}`
  只在分位差下成立：同一画面的 p90 − p10 典型是 0.55~0.8，而标准差只有 0.15~0.25，
  用标准差会让这个默认区间把每张图都判成「对比不足」。§5.2 的措辞本就是「分位差或标准差」。
- **调色板 k = 5**，`cv::setRNGSeed(0)` 固定种子。不固定的话 k-means++ 的初始化每次不同，
  同一张图会算出不同调色板 —— 「同一输入同一结果」是 Spec 版本对比与单测能成立的前提。
  缩到 ≤256×256 再聚类：k-means 的代价与像素数成正比，而占比的统计误差在 1% 量级，
  远小于「丢掉 share < 1% 的尾部簇」那个阈值。
- **`retouch_score_` 是下界**：只实现了暗角一类签名（判据是「四条边的中点都比中心暗」），
  所以它只会**低估**后处理痕迹，不会误伤；它只按下限 0.7 倍的系数影响置信度，不做门控。
  与「中央亮、四周暗的布光」不可区分 —— 这是单图信息的固有极限，§5.2 本就把这类痕迹定为「中」。

**色温是显示空间规则的唯一例外**：`white_balance_kelvin_` 内部先线性化 sRGB 再算 CCT。
Kelvin 是线性光色度坐标上的物理尺度，直接在 sRGB 编码值上算会偏几百 K，而色温区间只有
~800K 宽（默认 `[3400, 4200]`），偏差会直接把「落点」判错。详见 §8.2。

**受光方向为什么只作软提示**：PS 的加减光和渐变会伪造方向线索。方向主要由提示词与预设库决定，或由美术在候选挑选中隐式选定。

**已确认参考以全局调色为主**，这提升了统计量的可信度：全局调色不改变空间结构，调色板、亮度分位、对比度这些量都能被完整迁移；相反，若参考含大量局部改图（加减光、重绘），这些量会被局部编辑污染。因此当前默认对统计量给予较高置信度，同时保留外点剔除能力以防局部编辑。

**单图统计（本节的①）已落地，多图聚合（②）还没有**：所以 `is_outlier_` / `excluded_reason_`
暂时恒为 `false` / 空 —— 外点判定必须跨图比较，属于 §5.3。

### 5.3 多图聚合与冲突检测

1. 逐图提取统计量
2. 逐维度取中位数（或按面积/清晰度加权）
3. **冲突检测**：维度内离散度超阈值 → 标记 `conflicts`，**不自动平均掉**
4. 冲突维度进入人工闸门 1 裁决，或降级为「该维度不设目标」

### 5.4 视频 → 关键帧序列

```
分镜检测 → 每镜关键帧(首/中/尾 + 内容变化帧) → 并入多图聚合流程
        → 若关键帧间统计量单调变化 → 输出时序曲线（用于灯光动画）
```

视频不设独立路径，**作为多图来源**统一处理。

### 5.5 人物角色性格 → 灯光语言映射表

提示词里最有价值、也最容易落不到灯上的字段。必须结构化。

| 性格维度 | key:fill 比 | 硬软 | 光位倾向 | 色温倾向 |
|---|---|---|---|---|
| 阴郁 / 内向 | 高（6:1–10:1） | 硬 | 侧高、少正面 | 偏冷 |
| 开朗 / 外向 | 低（2:1–3:1） | 软 | 正面偏上 | 偏暖 |
| 威严 / 压迫 | 很高（10:1+） | 硬 | 顶光 / 底光 | 中性偏冷 |
| 温柔 / 脆弱 | 很低（1.5:1–2:1） | 很软 | 正面、包覆 | 暖 |
| 危险 / 不稳定 | 高且不稳定 | 硬 | 侧后、明暗交界 | 冷暖冲突 |
| 神圣 / 超然 | 中 | 很软 | 顶光 | 冷白 |

### 5.6 Tier B 多视角联合反解 —— 已取消

原设计（多视角联合约束：一套灯光必须同时解释所有视角；把 AI/PS 的全局调色吸收进 per-image nuisance 参数；局部改图作为外点走鲁棒损失；位姿由 render-and-compare 或人工对位求解）**不再实施**。

**取消原因**：它的位姿来源里唯一务实可靠的一条是「人工对位相机」，而这条已被明确否掉（不值得让美术为每张参考花 1–2 分钟）。剩下的自动重定位既要面对 AI 编辑破坏特征匹配，又要在收益上胜过已经能用的 Tier A —— 收益/成本比不成立。

**连带影响**（已同步清理）：

| 项 | 变化 |
|---|---|
| `LightingSpec.tier` | `stats_only \| multiview \| prompt_only` → **字段删除**（取值塌缩成与 `degrade_level` 相同，Spec schema 升 1.3） |
| 降级等级 | 三级 → 两级（§12.1） |
| 里程碑 | M7 从路线图移除 |
| `reference_stats.usable_for_physical_solve` | 恒为 `false`，成为常量而非判断结果 |

---

## 6. ② 规划层与候选生成（主服务器）

### 6.1 LLM 的职责边界

| LLM 做 | LLM 不做 |
|---|---|
| 决定灯的**数量、角色、类型、相对方位** | 决定最终精确强度/色温/距离 |
| 决定**环境光/雾/天空/曝光/调色**的整体倾向 | 直接写 UE API 调用或代码 |
| 决定**增删改**哪些灯（结构决策） | 逐帧关键帧的精确数值 |
| 把美术的反馈文本**结构化为轴上的 delta** | 预测渲染结果 |
| 选择/组合**预设模板** | 绕过 schema 自由输出 |

### 6.2 候选生成：沿语义轴变化（判断六的核心）

每轮固定 **6 个候选**，沿**语义可解释的轴**变化（每轴 2–3 档）：

| 轴 | 档位示例 | 影响的目标 |
|---|---|---|
| `key_direction` | 方位角 −30° / 0 / +30°，仰角低/中/高 | 受光方向、对比分布 |
| `key_fill_ratio` | 3:1 / 6:1 / 10:1 | 对比度、亮度分位 |
| `kelvin` | 暖 / 中 / 冷 | 白平衡 |
| `hardness` | 软 / 中 / 硬 | 阴影边缘、对比 |
| `ambient_level` | 低 / 中 / 高 | 暗部抬升、p10 |
| `grading_strength` | 弱 / 中 / 强 | 调色板、对比、暗角 |

**为什么这很重要**：美术的选择（"第 3 个"）直接翻译成轴上的一个区域 → 下一轮在该区域收缩并细化。这把人工判断从 yes/no 变成**有效的搜索梯度**。没有这一步，人工在环的收敛会慢到不可用。

第一轮用较大的轴跨度（探索），后续轮次收缩（利用）。

### 6.3 提示词与参考的冲突规则

**提示词优先于参考图。** 参考只提供「环境与场景的结构与数值」，提示词提供「意图与语义」。

| 情况 | 处理 |
|---|---|
| 提示词与参考一致 | 直接合并，置信度提升 |
| 提示词明确、参考冲突 | 提示词胜出，冲突写入 op 的 `rationale` |
| 提示词未提及、参考明确 | 参考胜出 |
| 提示词未提及、参考冲突 | 不设目标，交人工裁决 |

### 6.4 知识注入：灯光预设库（Rig Library）

起点是项目已有资产 `ADoodleAssetsPreview` 的 `LowContrast / MidContrast / HighContrast` 三档灯光环境；**首批正式预设由美术整理**（三点布光 / 伦勃朗 / 剪影 / 夜景霓虹…），已确认安排。

```jsonc
{
  "preset_id": "three_point_studio",
  "name": "标准三点布光",
  "tags": ["人像", "中性", "低对比", "影棚"],
  "embedding": [/* name+tags+示例图 CLIP 编码 */],
  "rig_fragment": { /* 一段 LightingRig */ },
  "applicable": { "subject_count": [1,3], "scene_type": ["interior"] },
  "stats_target": { "key_fill_ratio": 3.0, "kelvin": 5600 }
}
```

流程：**intent + 参考统计目标 embedding → top-k 预设 → 候选骨架 → LLM 裁剪/组合/微调**。

### 6.5 工具面（function calling schema）

| 工具 | 作用 |
|---|---|
| `inventory_lights()` | 读当前灯光与环境清单 |
| `query_scene(anchor)` | 取对象包围盒/位置/朝向 |
| `list_presets(query)` | 检索预设 |
| `build_spec(prompt, refs)` | 生成 LightingSpec |
| `generate_candidates(spec, axes, n)` | 沿轴生成 N 个候选 rig |
| `parse_feedback(text)` | 把美术反馈文本结构化为轴上的 delta |
| `commit(rig)` | 定稿（唯一有副作用） |

### 6.6 护栏（硬约束，schema 层面强制）

- 灯数量与动态阴影灯数量：**暂不设硬上限**（已确认），但护栏保留为可配置项；同时**记录每轮实际灯数与渲染耗时**，超阈值告警
- **候选之间结构必须一致**：6 个候选的灯的集合与角色必须相同，只有轴上的参数不同 —— 否则指标差异会被灯数变化淹没，美术的选择无法归因到轴上，梯度失效
- 强度/色温/角度/距离的取值边界
- 光照通道约定（沿用现有 `OnCreateDirectionalLight` 的 `SetLightingChannels(false, true, false)` = Channel1 给主光）
- 禁止破坏已有子序列绑定（`AddSequenceWorldToRenderWorld` 建立的层级）
- 曝光必须为手动（见 §7.5）
- **调色不得吞掉灯光差异**：若调色大改而灯光几乎没变，视为未达成，提示美术
- **候选必须沿轴可解释**：不接受随机采样产生的候选（无法翻译成梯度）
- **禁止改动几何与材质**（已确认）：可改范围严格限定为灯光 + 环境光/雾/天空 + 后处理/调色；执行器遇到涉及几何或材质的 op 必须拒绝，不做静默忽略

---

## 7. ④ 执行层：UE 无头执行器

### 7.1 唯一的执行形态

```
UnrealEditor-Cmd.exe <uproject> -windowed -log -Unattended \
    -run=DoodleLighting -Params=<json>
```

由现有 `import_and_render_ue.cpp:221-225` 的命令行组装点扩展一个分支。

**与 v3.1 相比取消的部分**（因为决策 2）：

| 取消项 | 原因 |
|---|---|
| Slate 面板 | 无实时交互 |
| 编辑器内 WebSocket 常驻客户端 | commandlet 用 JSON 参数文件接收输入，用文件/HTTP 回传结果 |
| 实时 op 应用 | 每轮整体执行，不是逐 op 应用 |
| 撤销栈 | 无状态执行，不修改源资产（见 7.2） |
| `RemoteControl` | 仅外部驱动编辑器的场景才需要，本方案不需要 |

### 7.2 无状态、不碰源资产（重要简化）

沿用现有 `DoodleAutoAnimationCommandlet` 已有的模式（`DuplicateAsset(OriginalMapPath, RenderMapPath)`）：

```
从基线关卡复制 → 在工作副本上 apply_ops → 渲染 → 输出到该轮次目录
```

- **源资产永不被修改** → 天然安全，不需要撤销栈
- 快照/回滚在迭代阶段**退化为「丢弃该轮输出」**
- `snapshot/restore` 原语**只在定稿步骤（§3 的 ⑤）需要**，那是唯一有副作用的步骤

### 7.3 一次启动，渲染 6 个候选（性能关键）

UE 进程启动成本高（数十秒到数分钟）。所以：

- **一个任务内含 6 个候选**，一次启动 UE 循环处理
- 每个候选：`apply_ops` → `render_preview` → 写输出目录 → 清理到基线
- 输出目录按 `session/iteration/candidate` 分层
- 灯数与总耗时**暂不设硬上限**（决策 17），但每轮记录实际值，超阈值告警

### 7.4 执行原语

| 原语 | 说明 |
|---|---|
| `inventory` | 扫描基线关卡 → 输出 SceneFacts（含 `postprocess.color_grading`） |
| `apply_ops` | 按 op 列表增删改（含 `set_grading`），全程 `Modify()` |
| `render_preview` | 低分辨率预览：**PNG（显示空间，主判据）** + EXR（线性，工程检查） |
| `compute_metrics` | 就地计算显示空间指标（可选，服务端也可复算） |
| `export_rig` | 导出当前状态为 IR（支持从美术手工灯反向生成 IR → 作为预设来源） |
| `snapshot` / `restore` | **仅定稿步骤使用** |
| `commit` | 把接受的 rig 应用到正式关卡/序列（唯一有副作用） |

### 7.5 实现要点（血泪项）

1. **幂等**：同一 rig 从同一基线重复执行，结果必须一致（因为每轮都是独立进程，这条比 v3.1 更关键）。
2. **`ClearAllLight()` 不能直接复用**：现有实现（`DoodleAutoAnimationCommandlet.cpp:535`）无条件遍历销毁所有 `ALight` + `APostProcessVolume`。在"从基线复制的工作副本"上，这会把美术手工灯也清掉。改为按 op 精确增删。
3. **曝光必须手动锁定**：现有 `PostProcessVolumeConfig()` 已把 `AutoExposureMinBrightness/MaxBrightness` 固定为 1.0、`AutoExposureBias=0.0`。这是对的，**迭代期间绝不能放开** —— 否则调强度会被自动曝光吃掉，指标失去区分度。
4. **Lumen 参数必须固定**：`LumenFinalGatherQuality=4` 等已在代码里固定，迭代期间不要变动，否则指标漂移。
5. **单位显式**：`ULightComponent::SetIntensityUnits(ELightUnits::Candela/Lux/EV)`，IR 里也带单位。
6. **环境光也算灯**：`SkyLight / SkyAtmosphere / ExponentialHeightFog / VolumetricCloud` 纳入 IR `environment`。
7. **调色是可写区域**（决策 7）：`globals.grading` 要能落到 PostProcessVolume 的 Color Grading / Bloom / Vignette。
8. **时间维度**：Level Sequence 光照轨道写关键帧；可复用项目已有的 `DoodleLightningPost` / `DoodleCluster` 轨道序列化能力。
9. **失败隔离**：单个候选渲染失败不应让整轮失败；回传部分结果 + 失败原因。

### 7.6 模块落点

新增两个模块，**不污染现有 `DoodleEditor`**：

- `DoodleLighting`（Runtime）：IR + Spec 数据结构、JSON 序列化、校验器、op 语义、指标计算（可脱离 UE 单测）
- `DoodleLightingEditor`（Editor）：**commandlet + 执行器 + 渲染**（无面板）

> 命令类模块必须是 Editor 类型（`UCommandlet` 需要），所以 `DoodleLightingEditor` 仍然存在，但内容比 v3.1 少得多。

---

## 8. 评判层：决策辅助，不是自动判据

### 8.1 人工判断是唯一收敛判据（决策 3）

| 判据 | 位置 | 内容 | 角色 |
|---|---|---|---|
| **人工判断** | 浏览器（人工闸门 2） | 挑选候选 / 反馈 / 接受 | **唯一收敛判据** |
| A 目标区间满足 | 服务端 | 显示空间感知指标 vs `Spec.targets` 区间 | **决策辅助**（帮美术快速判断） |
| B 语义判分 | 服务端（VLM） | 氛围 / 角色性格 / 故事是否成立 | **决策辅助** |

**没有自动接受。** A 和 B 都只作为「渲染图旁边的信息」呈现，帮助美术判断，不自动决定任何事。

### 8.2 指标空间

- **主指标在显示空间（色调映射后）计算**：参考是美术在显示空间里调出来的成片，它的亮度分布、对比、调色板都是显示空间的量。拿线性空间的渲染去比显示空间的参考，必然对不上。
- **EXR（线性）只用于工程检查**：曝光余量、裁剪、灯光强度是否超出物理合理范围。
- **唯一例外：色温**。`white_balance_kelvin_` 必须先把 sRGB 线性化，再走 XYZ 色度坐标求 CCT
  （`ai_light_stats.cpp` 的 `estimate_kelvin`，用 McCamy 近似）。
  理由：Kelvin 是**线性光**色度上的物理尺度，直接在 sRGB 编码值上算会偏几百 K ——
  而色温区间只有 ~800K 宽（默认 `[3400, 4200]`），这点偏差足以把「落点在不在区间内」判反。
  其余指标（分位、对比、调色板、削波率、痕迹）都留在显示空间。
  这一条必须写明：不然下一个实现者会在显示空间上算 CCT，而错得很安静。

### 8.3 指标清单

| 指标 | 目标形式 | 责任方 |
|---|---|---|
| 调色板分布 | EMD / 占比区间 | 调色为主，灯光为辅 |
| 亮度分位 p10/p50/p90 | 区间 | 灯光（曝光与强度） |
| 对比度 | 区间 | 调色 + 灯光 |
| 白平衡 / 色温 | 区间 | 灯光 |
| 高光/暗部裁剪率 | 区间 | 灯光 |
| 暗角 / 提黑 / 光晕 | 定性痕迹 | 调色 |

呈现方式：**区间满足率 + 每项的落点位置**（在区间内 / 偏高 / 偏低），让美术一眼看出"差在哪、差多少"。

### 8.4 VLM 判官

输入：`intent` 描述 + 参考图 + 候选渲染 → 输出 `score 0–10` + **可执行的自然语言差异**。自由文本再经一次结构化映射（LLM）转成 `intent` 字段的 delta。

**工程优化**：参考图先用 DeepSeek [Files API](https://api-docs.deepseek.com/zh-cn/guides/vision/) 上传一次拿 `file_id`，闭环内每轮引用而不重复上传。

### 8.5 回归基线（M6）

不能再用「与参考图的像素差」。改为四条：

1. **确定性单测**：同一 Spec + 同一轴 → 同一组候选 rig（规划层可脱网复现，黄金用例）
2. **区间满足率**：固定场景 + 固定 Spec → 渲染后指标落入目标区间的比例
3. **人工盲测偏好**：固定场景下多版本 A/B，与指标做相关性校准（用于验证"辅助信息是否真的有用"）
4. **退化测试**：故意给不相关参考（纯色块、无关照片），系统必须**不崩且能给出低置信度 Spec**，而不是硬凑

---

## 9. 优化器：候选生成器 + 指令求解器

因为收敛判据是人工（决策 3），优化器的角色从「自动收敛」改为两件事：

### 9.1 候选生成器（主要职责）

- 沿语义轴（§6.2）生成差异化候选
- 用灵敏度标定保证候选**真的在目标上有差异**（避免生成一堆看起来差不多的候选）
- 第一轮大跨度探索，后续轮次在美术选中的区域收缩

### 9.2 全局调色反解（grading 类目标的热启动）

已确认**参考以全局调色为主**，这给出一个不需要位姿、不需要几何的解析机会：

```
参考图统计分布  ←→  当前渲染统计分布
   ↓ 分布匹配（逐通道直方图匹配 / 最优传输），不是逐像素回归
   ↓ 得到全局变换 T：display_srgb → display_srgb
   ↓ 分解为 UE 可写的参数
      slope / offset / gamma（逐通道） → Color Grading 的 Gain / Gamma / Offset
      saturation、contrast             → Color Grading 的 Saturation / Contrast
      暗角强度                          → Vignette
   ↓ 作为 grading 类目标的初值
```

- **为什么是分布匹配而不是逐像素回归**：参考是异视角，逐像素对应不存在；但「调色板占比、亮度分位、对比度」这些**分布**是可比的，而且正是 `Spec.targets.ownership.grading` 里那几个目标。
- **收益**：`palette` / `contrast` / `grading_hints` 这三个目标从「靠搜索逼近」变成「一步反解 + 1 次验证渲染」，直接把候选生成的起点抬到接近目标的位置。
- **边界**：这只解决 grading 类目标，**不解决 lighting 类目标**（亮度分位、色温、裁剪率由灯负责，仍走 §9.3 的灵敏度标定）。
- **失败退路**：若反解出的 T 映射不到 UE 参数（映射误差大），退化为灵敏度标定搜索，不阻塞流程。

### 9.3 指令求解器（次要职责）

美术给方向性反馈（"再冷一点、主光再低一点"）→ 结构化（§4.4 `feedback_axes`）→ 解成具体参数改动：

```
① 少量探针渲染 → 局部雅可比 J = ∂指标/∂参数
② 解线性系统：在 J 的线性近似下，求满足 delta 的最小参数改动
③ 1–2 次验证渲染
④ 残余偏差 → 坐标下降微调
```

按责任方分阶段：先全局（曝光/白平衡/调色）→ 再逐灯（按 `role`，主光优先）→ 最后细节（source radius / cone angle）。

### 9.4 其余要点

- **warm start**：上一轮美术选中的 rig 直接作下一轮基准
- **并行候选**：一个任务内渲染 6 个候选（见 §7.3）
- **不要用黑箱搜索当主算法**：区间目标 + 灵敏度标定比 CMA-ES 快一个量级；黑箱只用于残余

---

## 10. Doodle 服务侧集成

### 10.1 现有可复用资产

| 现有资产 | 复用方式 |
|---|---|
| `exe_warp/ue_exe.cpp::async_run_ue` | UE 进程启动、日志回灌、超时、插件版本安装 |
| `exe_warp/import_and_render_ue.cpp:221-225` | 命令行组装点，新增 `-run=DoodleLighting` 分支 |
| `DoodleAutoAnimationCommandlet.cpp` | 灯光执行逻辑起点；**其 `DuplicateAsset(OriginalMapPath, RenderMapPath)` 模式正是 7.2 需要的** |
| `ADoodleAssetsPreview` | 预设库第一批条目（三档对比度环境） |
| `http_method/kitsu/auto_task.h::shot_render_light_builder` | 镜头灯光资产路径装配 |
| `kitsu_ctx_t::deepseek_keys_` + `/api/doodle/deepseek/key` | LLM 密钥来源（文本 + 视觉同一把 key） |
| `http_client/ai_client_base.h` | AI 客户端抽象基类 → 新增 `deepseek_client`（chat/vision） |
| `server_task_info_type` + 分布式契约 | 新任务类型派发 |

### 10.2 新增任务类型与数据表

- 任务类型：`server_task_info_type::ai_light`（追加在枚举末尾）
- 资产路径**不再**复用 `get_shots_auto_lighting_upload_path`（那条会随项目/剧集/镜头命名漂移，
  且没建资产扩展就算不出目录）——会话产物是 uuid 标识的生成物，与 `sd2`/`pictures`/`jobs` 同性质，
  所以挂在**资产根** `kitsu_ctx_t::root_` 下、按 uuid 命名，见 §15.4「产物路径」：

  ```
  <root>/ai_light/<会话 uuid[0:3]>/<会话 uuid[3:6]>/<会话 uuid>/ref/<参考图 uuid>.png
  <root>/ai_light/<会话 uuid[0:3]>/<会话 uuid[3:6]>/<会话 uuid>/it_001/<候选 uuid>.png
  <root>/ai_light/<会话 uuid[0:3]>/<会话 uuid[3:6]>/<会话 uuid>/it_001/<候选 uuid>.exr
  <root>/ai_light/backup/<YYYY_MM_DD_HH>/<会话 uuid>/          ← 删会话时整体搬到这儿
  ```

  **文件名一律是各自的 uuid**（不是序号）：序号落盘会撞车（同一目录两张图都叫 `ref_00.png`），
  而 uuid 天然唯一，序号退回为纯展示顺序。**只有会话目录分桶**（`split_uuid_path`，与 `sd2`/`jobs` 一致）；
  一个会话的参考图是几十张、单轮候选固定 6 张，内层再分桶只有噪声。
  代价：产物不再跟着项目目录归档，改跟资产根的生命周期走；收益：路径不再依赖
  episode/shot 元数据，`session_dir` 与编排层算候选路径都**不用查库**，「镜头没建资产扩展就传不了参考图」
  这一整类失败随之消失。

**六张表**（`user_version` 30 → 31，全部是新表，迁移只需 `sync_schema()`）：

| 表 | 职责 |
|---|---|
| `ai_light_session` | 一次灯光工作：提示词原文、状态机、当前轮次、4 小时判断时限。归属三键 `project_id` / `entity_id`(镜头) / `task_id`(该镜头的灯光任务) **全部由服务端解析**，见 §13.1 |
| `ai_light_reference` | 多张参考图，**每张单独留显示空间统计**（多图冲突要能回溯到具体哪张图） |
| `ai_light_spec` | 灯光目标，会话内按 `revision` 版本化；含 `degrade_level` / `human_confirmed` / `conflict_resolution` |
| `ai_light_iteration` | 一轮：轴安排、派发任务、渲染耗时、人工判断记录 |
| `ai_light_candidate` | 每轮 6 个候选：rig 全量、显示空间指标、VLM 判分、是否被选中/定稿 |
| `ai_light_preset` | 预设库 + embedding |

**相对早期方案的三处调整**（理由见下）：

1. **`ai_light_job` 合并进 `ai_light_session`** —— 两者职责完全重叠（都是「一次灯光工作的输入 + 状态」），
   拆两张只会带来同步负担。
2. **`ai_light_rig` 合并进 `ai_light_candidate`** —— rig 是候选的属性而非独立实体；
   拆开后「rig 属于哪一轮哪个候选」还要靠关联表，纯属多余。
3. **新增 `ai_light_reference`** —— 早期方案漏了：参考图必有多张，
   且每张图的原始统计必须留存，否则美术无法看到「到底是哪张图把色温拉偏了」，§5.3 的人工裁决无从下手。

**落库约定**（沿用既有 ORM 事实，见 `doodle_core/metadata/ai_light/`）：

- 枚举按 `magic_enum` 的 **C++ 名** 存 TEXT，所以枚举项名必须与 JSON 字符串一致，且绝不重命名已有项
  （JSON 那一侧还有 `NLOHMANN_JSON_SERIALIZE_ENUM` 认不出就**静默回落**的坑，见 §15.6 坑 7；
  本节说的「下发派发任务类型」`server_task_info_type::ai_light` 与推理任务的
  `seedance2::task_type::inference` 是**两个不同的枚举**，别混）
- 嵌套结构（`rig` / `metrics` / `intent` / `targets` / `judgement` …）一律以 `nlohmann::json` 落库 ——
  ORM 的列类型不覆盖嵌套结构体与 `std::map`
- **不做冗余指针、不做循环外键**：「当前 Spec」= 会话内 `revision` 最大的一行，
  「本轮起点」= 上一轮 `is_selected` 的候选，「定稿候选」= `is_accepted` 的候选，
  全部靠查询得出。每会话至多一个定稿由 `accepted_session_id` 上的普通唯一索引在库层强制
  （定稿才填 session_id、否则为 NULL；SQLite 的唯一索引把 NULL 视为互不相等，于是等价于
  `WHERE is_accepted = 1` 的 partial index —— 为什么不能直接用 partial index 见 §15.3）。
- 「等人工判断」是**会话/轮次状态**，不进 `server_task_info_status`

**`preview_file` 的复用边界**：

| 产物 | 进 `preview_file`？ | 取图方式 |
|---|---|---|
| 参考图（多张） | 否 | `/api/doodle/ai/light/reference/{uuid}/image` |
| 中间候选渲染（6 × N 轮） | 否 | `/api/doodle/ai/light/candidate/{uuid}/image` |
| **定稿候选渲染** | **是** | 复用现有预览接口，`source = auto_light_generate` |

这样镜头的预览列表只呈现交付结果，不会被 6×N 张中间候选淹没，
也不需要给所有现有预览列表页加 `source` 过滤。


### 10.3 轮次编排（新增，核心）

服务器侧的编排逻辑：

```
生成候选 → 建任务 → 派发 → 等 worker 回传 → 落 iteration
  → 置 session.state = awaiting_judgement → 通知前端
  → 等人工判断（可能数小时/数天）
  → 收到判断：
       accept            → 走定稿（⑤）
       continue_iteration → 据 feedback_axes 生成下一轮候选 → 回到派发
       reject_all        → 回到规划层重新出方案（或结束）
```

- 「等待人工判断」是**会话状态**，不是任务状态（避免污染 `server_task_info_status`）
- **美术 4 小时以上不判断 → 会话被标记为「已过期」**：`judgement_due_at_` 已过 ⇒ 读模型的
  `is_judgement_overdue` 为真。**状态不变**，会话仍是 `awaiting_judgement`。
- 过期判定用会话上专门的 `judgement_due_at` 列，**不用 `updated_at`** ——
  `updated_at` 会被任何一次更新刷新，拿它当超时基准会让「美术看了但没判断」的会话永远不过期
- **为什么过期不是一个状态**（曾经叫 `suspended`，已删除）：过期是
  `state == awaiting_judgement && now > judgement_due_at_` 的纯派生事实，实时算得出来。
  物化成状态的代价是实的：`apply_judgement` 只接受 `awaiting_judgement`，而没有任何路径
  从 `suspended` 回到 `awaiting_judgement`（`mark_awaiting_judgement()` 只在渲染回填里调用，
  而渲染早就完成了）—— 于是美术超时回来后，**已经渲染好的 6 个候选再也判断不了**，
  只能走 `/spec` 重出方案，白扔一整轮渲染。更刺眼的是 `is_judgement_overdue()` 当时也要求
  状态是 `awaiting_judgement`，所以挂起后这个标志会从 true 翻成 false：
  恰恰在「真的过期了」那一刻，过期信号消失了。
- 挂起原计划里的收益是「不占资源」，但那是空头支票：会话进入 `awaiting_judgement` 的前提
  就是渲染已完成、worker 已归还，此时没有农场槽位、没有 UE 授权、没有 GPU 被占，
  6 张 PNG 只是躺在磁盘上（挂起也不删它们）。
- 因此**没有后台扫描器**：过期不需要任何人去物化，前端按 `is_judgement_overdue` 催办即可。
  这也顺带消掉了一个「常驻后台任务挂了没人发现」的失败面。

### 10.4 分布式派发必须遵守的既有契约

来自既有实现的教训（`computers.cpp` / `work.cpp`）：

- 派发判据 = 客户端上报 `online` **AND** 服务端自有事实「库里没有 `status_=running` 且 `run_computer_id_` 指向该机的任务」
- 客户端用 `running_task_count_` 计数，归零才报 `online`
- **任务结束前必须 `put_job_info` 上报终态**，否则任务永远留在 running 挡住该机器

### 10.5 前端

见 §14。浏览器是唯一交互面。

---

## 11. 分阶段里程碑

| 阶段 | 内容 | 完成判据 |
|---|---|---|
| **M0 地基** | IR + Spec schema、双端序列化、commandlet 骨架 + `inventory`/`apply_ops`（含 `set_grading`）/`render_preview` | 手工写一份 Spec + 一个候选 → 跑 commandlet → 出图；幂等性单测通过 |
| **M1 单轮管线** | 提交（提示词+多图）→ Spec → 候选生成 → 派发 → 渲染 → 前端展示 → 人工判断 | 完整跑通一轮：美术在浏览器上看到 6 个候选并能给出判断 |
| **M2 多轮迭代** | 反馈结构化 + 轴收缩 + 轮次持久化 + 跨会话恢复 | 3 轮内收敛到美术接受；中途关掉浏览器再回来能继续 |
| **M3 参考驱动** | 多图统计聚合 + 冲突检测 + 人工裁决 + `degrade_level` 判定（**①单图统计已落地**，见 §15.5；②聚合与冲突检测待做） | 目标区间稳定、冲突能被人正确裁决 |
| **M4 决策辅助** | 区间满足率 + VLM 判分呈现在候选旁 | 美术盲测认可辅助信息有用 |
| **M5 定稿** | `snapshot`/`commit` 应用到正式关卡/序列 | 定稿可回滚，结果与候选一致 |
| **M6 生产化** | 批量派发（整集/整序列）、性能预算、回归基线 | 基线纳入 CI |

> 与 v3.1 相比：原 M1「交互闭环（Slate 面板）」取消，替换为「单轮管线」；前端工作提前到 M1；M2 新增跨会话恢复；原 M7（Tier B 多视角联合反解）因人工对位被否而移除（§5.6）。

---

## 12. 关键风险与对策

| 风险 | 对策 |
|---|---|
| LLM 数值不可靠 | 多轮迭代 + 候选生成；LLM 只给结构与初值；schema 硬边界 |
| **人工判断往返慢、轮次少** | 每轮固定 6 个沿轴变化的候选（一次往返换更多信息）；一次 UE 启动渲染全部 6 个；warm start |
| **人工判断只是 yes/no → 收敛慢** | 候选沿语义轴变化（判断六），把选择变成梯度 |
| **美术长期不判断** | `awaiting_judgement` 状态 + 读模型的 `is_judgement_overdue` 标志（纯派生，不改状态） |
| **参考被 AI 改到物理不可解** | **不尝试物理反解**，显式记录 `usable_for_physical_solve=false`；用显示空间统计 |
| **多图统计互相冲突** | 逐维度置信度 + 冲突标记 → 人工裁决；**不自动平均掉冲突** |
| **目标里的调色部分靠灯够不到** | 后处理/调色纳入必需原语；Spec 里区分 `ownership.lighting` / `ownership.grading` |
| **用调色"作弊"达成指标** | 惩罚项 + 提示美术：灯光几乎没变而调色大改视为未达成 |
| 自动曝光 / Lumen 使强度不可预测 | 强制手动曝光、固定 Lumen 参数（已有代码基础） |
| 每轮都是独立进程 → 状态漂移 | 幂等性单测；固定 Lumen/曝光参数；从同一基线出发 |
| 提示词与参考冲突 | 明确规则：提示词优先；冲突记入 rationale 供人工核对 |
| 语义歧义（"电影感"） | 受控词表归一化 + 预设检索 + 候选挑选 |
| VLM 判分主观 | VLM 只作决策辅助，不参与收敛判定；A/B 盲测校准 |
| 单候选渲染失败拖垮整轮 | 失败隔离，回传部分结果 + 失败原因 |
| **灯数与渲染时间不设限 → 单轮可能极慢** | 护栏保留为可配置 + 记录实际耗时并告警；**候选之间结构必须一致**（否则指标差异被灯数变化淹没，梯度失效）；必要时 M6 拆到多机并行 |

### 12.1 两级降级

| 等级 | 条件 | 行为 |
|---|---|---|
| `stats_only` | 多图统计聚合可用（**默认，唯一主路径**） | 用显示空间目标区间 + 提示词 |
| `prompt_only` | 参考图的**统计**不可用（参考读不出、或全是老库里没统计的历史行），或参考之间完全冲突 | 只用提示词 + 预设库检索 |

> **`degrade_level` 描述的是「统计可用性」，不是「有没有参考图」。**
> 这一条必须说清：蒸馏 Spec 时参考图**必然存在**（`advance_session` 会校验，为空直接 400），
> 所以 `prompt_only` 只可能表示「有图但没有可用的统计量」。
>
> **统计层已落地**（`ai_light_stats`），`reference_route` 现在会在上传时算统计并写
> `stats_confidence_` —— 它有一个 **0.05 的地板**，所以「分析成功」与
> `references_have_stats()` 的判据 `stats_confidence_ > 0.0` 是等价的：
> 低置信度是一个**数值**，不是**缺失**。纯色块这类退化参考必须走 §8.5 第 4 条的
> 「不崩 + 低置信度」，而不是伪装成「统计层没落地」。
> 于是 `references_have_stats()` 对任何「有图且都能解码」的会话为真、`stats_only` 可达。
>
> 但它**暂时不可观测**：`advance_session` 把 `degrade_level_` 填进
> `spec_generation_request_t` 之后就交给 `generate_spec`，而后者仍是 501、什么都不落库。
> 也就是说这条现在只是「把输入准备对了」，真正能看出降级级别要等接缝落地。
> **多图聚合（§5.3 的②）也还没有**，所以 `stats_only` 的「聚合可用」严格说只到了
> 「单图统计可用」这一步；②落地后才是本表字面上的含义。

---

## 13. 改动归属：四类程序分别负责什么

### 13.0 一张表看懂

| 角色 | 入口 | 一句话职责 | 是否持有 deepseek key | 灯光改动的性质 |
|---|---|---|---|---|
| ① 主服务器 | `kitsu_supplement`（无参数） | 真库 + 全量 API + 任务派发 + **LLM/VLM + 感知/Spec/候选生成 + 轮次编排** | **是** | 新增：任务类型、提交端点、AI 路由、数据表、轮次编排 |
| ② 客户端 | `kitsu_supplement --local` | 工位本地后端（内存库 + 本地 API + socket.io） | 否 | 基本无改动（仅插件版本分发被动受影响） |
| ③ 工作程序 | `--local` + POST `/api/actions/local/task/run` | 连服务器 WebSocket、领任务、跑任务、报终态 | 否 | 新增：`run_task` 分派分支 + 新任务类 + allowed 类型 |
| ④ UE 插件 | `E:\Doodle\script\uePlug\Doodle` | **无头**：解析 IR → 应用 → 渲染 → 输出 | 否 | 新增两个模块（**无面板**）+ commandlet |

**核心结论：改动集中在 ①③④，②几乎不动。** 感知、Spec、候选生成、轮次编排全在 ①。

---

### 13.1 ① 主服务器

**现在负责什么**（`launch/kitsu_supplement.cpp:191-222`）

- 打开真实 sqlite 库（`l_args.db_path_`，默认 `C:/kitsu_new.database`）
- `get_register_info()` 从注册表 `SOFTWARE\Doodle\MainConfig` 读 **deepseek keys**、ji_meng 授权、jwt secret、domain；从 `SOFTWARE\Doodle\Email` 读邮件配置
- 构造**全参** `kitsu_ctx_t`（含 `deepseek_keys_`）—— 全系统唯一持有 LLM 凭据的地方
- 注册全量路由 `create_kitsu_route_2`：`/api/data/computers`（WebSocket）、seedance2、ue-plugins、前端
- 持有 `l_set.computers_assign_task_ptr_`（服务端派发器单例）

**要改什么**

1. **任务类型**：`server_task_info_type` 加 `ai_light`（`doodle_core/metadata/server_task_info_type.h:20` 附近）。
2. **派发**：**不单开提交端点**（早期方案里的 `/api/actions/projects/{}/shots/{}/ai-light` 已废弃）。
   派发逻辑内嵌在编排层 `advance_session()` → `generate_candidates()` 里：构造 `server_task_info`
   （`type_ = ai_light`，`command_ = {session_id, iteration, spec_id, candidates[], baseline_level}`，
   `command_` 是自由 JSON，无需改表结构），**事务提交之后**再调
   `computers_assign_task::get_instance().run_next_task()` + `socket_io::broadcast`。
   放在事务之后是有意的：任务行已落库，派发失败也只是「暂时没人领」，
   `computers_assign_task` 下次轮询会再捡起来，不会丢。
   不单开端点的理由：建会话入口与 `/iterate` 都要派发，抽在编排层里只有一份实现，
   而独立的提交端点会变成第二个可以绕过归属校验的入口。
3. **派发逻辑**：**零改动**。`computers.cpp:376-378` 已按 `computer.allowed_task_types_` 通用过滤。
4. **LLM/VLM 客户端**：原计划是「新增 `deepseek_client`（继承 `http_client/ai_client_base.h`，OpenAI 兼容，
   支持 chat + vision）」。**实施时改成了复用 `seedance2_client` + 火山方舟 Responses API**（§15.6），
   理由是四条实地约束：

   - 仓库里已经有**同一个厂商**的 Ark 客户端（`seedance2_client`，`https://ark.cn-beijing.volces.com`），
     连 TLS 上下文、超时、`Bearer` 取 key、`ai_client_base` 的错误约定都是现成的；
     新写一个 client 等于把这套东西抄一遍，且多一份要维护的凭据读取路径；
   - Responses API 一个请求里同时吃文本与图片（`input_text` + `input_image`），
     正好是 `generate_spec` / `structure_feedback` 需要的形态（参考图统计 + 提示词）；
   - 走 `ai_client_base::run_task(const sd2::task&)` 的**同一条**管线，
     于是 token 计费、`person_token` 日统计、任务行与前台展示全部复用既有实现，
     不需要为 LLM 调用另起一套记账；
   - 凭据仍在 `kitsu_ctx_t`（`seedance2_key_` / `ai_studio`），**必须在主服务器**这条约束不变。

   接缝本身（`generate_spec` / `generate_candidates` / `structure_feedback`）**仍返回 501**：
   本轮交付的是「可用的推理通道 + 数据模型 + 鉴权」，把它们接上去是下一步
   （调 `make_inference_client()` + `run_inference_task()` 即可，参数与返回值已经定好）。

5. **AI 路由**：**已落地（本轮）**，12 条，全部注册在 `create_kitsu_route_2`（主服务器），
   声明在 `doodle_lib/http_method/kitsu/ai_light.h`，处理函数在 `ai_light.cpp`：

   | 方法与路径 | 说明 |
   |---|---|
   | `GET  /api/doodle/ai/light/shot/{entity_id}/session` | **该镜头**下的会话列表（含历史，新的在前） |
   | `POST /api/doodle/ai/light/shot/{entity_id}/session` | **唯一建会话入口**：为该镜头建会话（`draft`，**不**蒸馏 Spec），返回六表聚合详情 |
   | `GET  /api/doodle/ai/light/session/{id}` | 详情（六表聚合读模型） |
   | `DELETE /api/doodle/ai/light/session/{id}` | 删会话（产物先进备份区；已定稿禁止删） |
   | `GET  /api/doodle/ai/light/session/{session_id}/reference` | 参考图列表 |
   | `POST /api/doodle/ai/light/session/{session_id}/reference` | 多图上传（multipart，统一转 PNG） |
   | `DELETE /api/doodle/ai/light/reference/{id}` | 删单张参考图（含文件） |
   | `GET  /api/doodle/ai/light/reference/{id}/image` | 参考图取图 |
   | `GET  /api/doodle/ai/light/candidate/{id}/image` | 中间候选取图（只有定稿才进 `preview_file`） |
   | `GET  /api/doodle/ai/light/session/{session_id}/spec` | Spec 版本列表 |
   | `POST /api/doodle/ai/light/session/{session_id}/spec` | 生成 Spec：`draft` 出第一版，`awaiting_spec_confirm` 重出（**要求已有参考图**，否则 400）—— 本轮返回 501（见 §15.4） |
   | `POST /api/doodle/ai/light/session/{session_id}/spec/confirm` | 人工闸门 1：确认 Spec + 冲突裁决，随后触发首轮 |
   | `POST /api/doodle/ai/light/session/{session_id}/iterate` | 生成一轮候选并派发 —— **本轮返回 501**（见 §15.4）；无需请求体 |
   | `POST /api/doodle/ai/light/session/{session_id}/judge` | **人工闸门 2**（accept / continue_iteration / reject_all） |
   | `GET  /api/doodle/ai/light/preset` | 预设库列表（`?enabled_only=false` 取全部） |
   | `POST /api/doodle/ai/light/preset` | 新建预设（美术整理的第一批走这里导入） |
   | `GET/PUT/DELETE /api/doodle/ai/light/preset/{id}` | 预设单条 |

   **镜头 id 进路径，不是查询参数。** `ai_light_session.entity_id_` 是必填（NOT NULL）的镜头 id，
   会话的归属就是「某个镜头」，所以「这个镜头的会话集合」本身是一等资源，
   前端镜头页只面对一个 URL（原先的 `GET /session?entity_id=|project_id=` 已废弃）。

   **同理，会话作用域的动作一律挂 `/session/{session_id}/<动作>`。**
   `spec` / `spec/confirm` / `iterate` / `judge` 都是「对某个会话做一件事」，
   会话身份就该在 URL 上。改之前 `session_id` 在三个地方出现过：`GET /spec` 走查询参数、
   各 `POST` 走请求体、`/reference` 走路径 —— 同一个资源三种写法，前端每接一个接口都要先确认一次。
   放请求体还有第二个问题：体里带 `session_id` 时，服务端要么校验「体和路径是不是同一个」，
   要么悄悄覆盖其中一个，两种做法都留下「两个真相」。**路径为准，体里的 `session_id` 忽略。**

   这套嵌套是安全的，不会和 `/session/{id}`（会话详情）撞，也不会互相前缀吞并：
   `{}` 对 uuid 展开成**只含十六进制与连字符**的正则（`http_function.h` 的 `g_uuid_regex`），
   而路由匹配用的是 `std::regex_match` —— **整串匹配**。所以 `/session/<uuid>/spec` 与
   `/session/<uuid>/spec/confirm` 互为前缀也不会互相吞掉。副产品是路径里给一个畸形 uuid
   会**直接不匹配**（落到 404 兜底），而不是进到 `from_uuid_str` 去抛异常 —— 比查询参数更好。

   顺带一个具体后果：`/iterate` 的请求体因此变成空的。而 `get_json()` 在 content-type 不是
   `application/json` 时会抛 400「body 不是 json」（`http_session_data.cpp:443-450`），
   所以这个动作端点**必须不再调用 `get_json()`** —— 否则「空体调用 `/iterate`」会莫名其妙地失败。

   之所以要多加一层 `shot/` 而不是直接 `/session/{entity_id}`：
   `/api/doodle/ai/light/session/{id}` 已经是「按**会话 id** 取详情 / 删会话」，
   而镜头 id 与会话 id 都是 uuid，**形状上无法区分**，路由只能任选其一 ——
   症状是「有时 404 有时 200」而且取决于注册顺序，属于最难查的一类 bug。
   `POST` 的 `entity_id` 以路径为准（请求体里带了也会被覆盖）：
   同一个请求不能有两个「真相」，否则库里的值取决于谁后写。

   **归属三键全部由服务端解析，客户端一个都不用传**（`project_id` / `task_id` 都不在请求体里）：

   | 键 | 来源 | 为什么不由客户端给 |
   |---|---|---|
   | `entity_id_` | 路径 `{entity_id}` | 它就是这个 URL 标识的资源 |
   | `project_id_` | `entity.project_id_` | 客户端给 project_id 时，服务端只能校验「用户对那个项目有没有权限」，**管不了镜头和项目是否一致** —— 于是能把 A 项目的镜头挂到 B 项目下（用户对 B 有权限即可），造出归属矛盾的会话，之后所有按项目的列表都会显示它 |
   | `task_id_` | 该镜头的**灯光任务**：`get_tasks_for_entity_and_task_type(entity_id, task_type::get_lighting_id())` | 见下 |

   **`task_id_` 为什么必须解析而不是留空**：它不只是个标签 ——
   `build_accepted_preview_file` 用它填 `preview_file.task_id_`，而
   `get_preview_files_for_entity` 是 `from<task>().join<preview_file>(task.uuid_id_ == preview_file.task_id_)`
   的 **inner join**。留 NULL 的话已定稿候选图**永远不会出现在镜头的预览面板里**，
   而且不报错 —— 属于「看起来成功、实际丢结果」。所以找不到灯光任务时直接 400。

   用 `task_type::get_lighting_id()` 这个常量按 **id** 精确匹配，而不是「按 `task_type.priority_`
   挑优先级最高的任务」：`priority_` 是**预览面板的展示顺序**，拿它当「哪个任务是这次灯光工作的」
   是两件事 —— 只要动画的优先级更高，定稿图就会被挂到动画任务下，而按 task_id 分组**不会报错**，
   只会静静地出现在错误的分类里。按 id 匹配同时不依赖名称文本（可本地化）。

   `POST /api/actions/projects/{project_id}/shots/{id}/ai-light` **已删除**。
   它和 `POST /shot/{entity_id}/session` 建的是同一个会话，
   而它多带一个 `project_id` 在路径里，反而给了「镜头与项目不一致」的伪造空间 ——
   现在归属三键全部从镜头解析，没有第二个入口可以绕开这道校验。

   **建会话不蒸馏 Spec**，蒸馏由 `POST /spec` 触发。原因不是「少一次 LLM 调用」，而是顺序问题：

   Spec 的输入是「提示词 + 参考图」，而参考图只能在会话存在**之后**上传 ——
   `POST /session/{id}/reference` 要 `session_id`，而 `session_id` 正是建会话接口的返回值。
   在建会话的同一个请求里蒸馏，`get_ai_light_references()` **必然为空**，
   蒸出来的是一份「只用提示词」的方案，参考图这个主输入被静默丢掉。

   早先的实现确实在这里立刻跑第一步（`shot_entry_route` 合并进来时带过来的，
   理由写成「镜头页点一次按钮就该拿到一份待确认的 Spec」）。它只在接缝抛 501 时看起来没事：
   失败回滚把状态退回 `draft`，于是后续 `/spec` 恰好还能走通。
   **一旦接缝实现，会话就会停在 `awaiting_spec_confirm`，而那时 `/spec` 已经进不去了**
   （`advance_session` 当时没有这个状态的分支），参考图从此永远无法影响方案。
   也就是说这个缺陷被安排在「工具刚开始能用」的那一刻才出现。

   正确顺序：建会话（`draft`）→ 传参考图 → `POST /spec` 蒸馏 → `POST /spec/confirm` → 派发。
   这与 §14 提交页的流程一致（提示词与参考图是**输入**，出方案是**动作**）。

   `advance_session` 在蒸馏前校验参考图非空，为空直接 **400**（不写 `failure_reason_`、
   不走回滚）。这样「没传图」不会被静默降级成 `prompt_only`：
   `degrade_level` 描述的是**统计可用性**，不是「有没有参考图」，两者混在一个取值里就查不出来了。
   响应返回**六表聚合详情**（`ai_light_session_detail_t`，含 `session_` 在内，是会话本体的超集），
   这样前端一次就能拿到 `advance_session` 刚写入的 Spec，不必再补一发 GET。

   **两个 501 端点不是占位桩**：权限、状态校验、会话查找全部真实执行，只有最里层的
   LLM 接缝抛 `not_implemented`（→ HTTP 501，见 §15.4）。所以现在调它们得到的是
   「参数错了明确报错、参数对了告诉你能力还没到」，而不是一个假装成功的 200。
   `/judge` 的滑块通道（`feedback_axes`）**本轮即可用**：文本结构化是补充通道，
   接缝未实现时降级为「文本原样留存 + 只用滑块轴」，判断本身完全成立。
6. **轮次编排**：`awaiting_judgement` 会话状态、4 小时过期标志、跨会话恢复（§10.3）。
   **已落地**：`doodle_lib/http_method/kitsu/ai_light_orchestrator.{h,cpp}`
   （`advance_session` / `apply_judgement` / `mark_iteration_rendered` / `build_session_detail`）。
   编排层**不依赖 HTTP**（不 include `http_jwt_fun` / `session_data`），所以单测
   不需要伪造 JWT 或起服务器，能直接构造「处于某状态的会话」再调它。
7. **数据表**：`ai_light_session` / `ai_light_reference` / `ai_light_spec` / `ai_light_iteration` /
   `ai_light_candidate` / `ai_light_preset`，进 `sqlite_database` 的 upgrade 与 ORM 注册（§10.2）。
   **结构体与注册已落地**（`doodle_core/metadata/ai_light/`、`user_version` 30 → 31）。
8. **参考文件接收**：复用 `http_method/up_file.h` 的 `doodle_data_shots_file_auto_light` 模式；DeepSeek Files API 的 `file_id` 缓存也放这里。
   **文件接收已落地**（`POST /session/{id}/reference`，多图 + 统一转 PNG + 序号 `max+1` 不覆盖）；
   DeepSeek Files API 的 `file_id` 缓存随 `deepseek_client` 一起做。
   **参考图统计已落地**（`doodle_lib/http_method/kitsu/ai_light_stats.{h,cpp}`）：上传时在
   **已落盘**的 PNG 上算 `reference_image_stats_t` 的七个字段（亮度分位 / 色板 / 对比 /
   色温 / 削波率 / 修图痕迹）与 `stats_confidence_`，写进 `ai_light_reference`。
   算法口径见 §5.2，色温的显示空间例外见 §8.2。**多图聚合与冲突检测（§5.3 的②）未落地** ——
   它的唯一消费者 `generate_spec` 还是 501，所以单独一步做。
9. **反馈结构化**：`/judge` 收到的自然语言反馈经 LLM 转成 `feedback_axes`。
   接缝 `structure_feedback()` 已就位（本轮返回 501），调用方捕获后降级为纯滑块反馈。

---

### 13.2 ② 客户端（`kitsu_supplement --local`）

**现在负责什么**（`launch/kitsu_supplement.cpp:151-169`）

- `root = D:/sy_maigc`，**内存数据库**（`database_->open()` 无参）
- 授权上下文 + `create_kitsu_local_route()`（`http_method/kitsu.cpp:431-484`）
- 端口默认 0（随机分配）
- **注意：`kitsu_ctx_t` 两参构造，没有 deepseek keys** → 本地无法直接调 LLM

**要改什么**

1. **基本无改动**。
2. **被动受影响的唯一一处**：`exe_warp/ue_exe.cpp:84-128` 的 `installUePath()` 会在每次跑 UE 前从主服务器拉插件包、覆盖 `Engine/Plugins/Doodle`。**新增 `DoodleLighting` / `DoodleLightingEditor` 模块后必须发布新的插件版本包**，否则工作机上模块缺失、`-run=DoodleLighting` 找不到 commandlet。这是最容易漏的一环，症状是运行期才暴露。

---

### 13.3 ③ 工作程序（`--local` + POST `/api/actions/local/task/run`）

**现在负责什么**（`http_method/local/event.cpp` + `http_client/work.cpp`）

- `actions_local_task_run::post` 创建 `http_work`，`allowed_task_types` **默认 `{export_fbx, auto_light}`**（`event.cpp:32-34`）
- `http_work::async_run()`：连主服务器 `ws://{server_ip}/api/data/computers`，上报 `hardware_id`(SMBIOS) / `name` / `status` / `allowed_task_types`
- 收到任务 → `run_task()` 按 type 分派；不认识的类型直接 `report_task_unsupported` 回 failed
- `running_task_count_` 计数，**归零才报 online**；`base_distributed_task` 析构时 `task_finished()`
- 每个任务必须在 `run()` 结束前 `put_job_info` 上报终态

**要改什么**

1. **`run_task()` 加分支（必须）**：`server_task_info_type::ai_light` → `run_ai_light_distributed`。漏了会被判 unsupported 直接失败。
2. **`allowed_task_types` 默认值**：**不要**加进 `event.cpp:32-34` 的默认集合，改由调用方显式传 —— 不是每台机器都有 UE 授权和 GPU。
3. **新任务类**：`run_ai_light_distributed`。复用 `run_ue_assembly_base` 的**公共设施**（`create_logger()` 带日志回传 sink、`create_kitsu_client()`、`async_run_ue` 的进程启动/日志回灌/超时/插件安装），但**不能复用 `run_ue_assembly_base::run()` 的整个流程**——它的 command 是装配参数，灯光任务的是候选列表。建议抽公共基类。
4. **不要在工作机上放 key**：VLM 判分走「工作机回传渲染图 → 主服务器评分」。
5. **回传体积**：N 个候选 × 多帧 PNG 可能很大，需要压缩与增量上传（复用现有 `upload_shot_animation_auto_light` 模式）。

---

### 13.4 ④ UE doodle plug（`E:\Doodle\script\uePlug\Doodle`）

**现在负责什么**

六个模块：`doodle`(Runtime) / `DoodleCluster`(Runtime) / `DoodleClusterSequencer`(Editor) / `doodleEditor`(Editor) / `doodleUI`(Editor) / `BatchRender`(Editor)。C++ 侧驱动入口是 `DoodleAutoAnimationCommandlet`（`Main()` 目前只认 `Params` / `ImportRig` 两个 key）。

**要改什么**

1. **新增两个模块**（改 `Doodle.uplugin`）：`DoodleLighting`(Runtime) / `DoodleLightingEditor`(Editor，**仅 commandlet + 执行器 + 渲染，无面板**)。
2. **IR 与 Spec 双端一致**：UE 侧 `FJsonObject` 解析必须与 C++ 侧 nlohmann 严格对齐（单位枚举、`anchor.space`、`origin`、`ownership`）。建议落一份 JSON Schema，两端各写校验器，CI 跑同一组 golden 用例。
3. **commandlet 入口**：建议在 `Params` 的 JSON 里加字段，而不是新增命令行 key。
4. **执行流程**：复制基线关卡 → 循环 N 个候选（apply_ops → render_preview → 输出）→ 清理到基线 → 退出。**源资产永不修改**。
5. **必须改的现有实现**：
   - `ClearAllLight()`（`DoodleAutoAnimationCommandlet.cpp:535`）无条件销毁所有灯 + PostProcess，会把美术手工灯清掉；改为按 op 精确增删
   - `PostProcessVolumeConfig()` 的手动曝光（AutoExposure 固定 1.0/1.0）必须保持
   - `OnCreateDirectionalLight()` 的 `SetLightingChannels(false, true, false)`（Channel1 主光）成为 IR 默认约定
6. **调色写入能力**（决策 7）：`globals.grading` 落到 PostProcessVolume 的 Color Grading / Bloom / Vignette。
7. **`render_preview` 双输出**：PNG（显示空间，主判据）+ 线性 EXR（工程检查）。
8. **失败隔离**：单候选失败不影响其他候选。
9. **插件版本发布**：模块变更后必须发新版本包（对应 13.2 第 2 条）。

---

### 13.5 一轮完整调用链

```
[浏览器] 美术提交：提示词 + 多张参考图
  → [①] 提示词 → intent；多图 → 统计聚合 + 冲突检测 → LightingSpec
  → [浏览器] 人工闸门 1：核对 intent + 裁决冲突维度 → confirm
  → [①] 预设库检索 + 沿轴生成 N 个候选 rig
  → [①] 建 ai_light 任务{baseline, candidates[]} → insert + run_next_task
  → [③] WebSocket 收到任务 → run_ai_light_distributed
  → [③] async_run_ue: -run=DoodleLighting -Params=<json>
  → [④] 复制基线关卡 → 循环 N 次（apply_ops → render → 输出）
  → [③] 回传 N 张渲染图 + 指标 + put_job_info
  → [①] 落 iteration，置 session.state = awaiting_judgement，通知前端
  → [浏览器] 人工闸门 2：候选接触表（并排 + 指标辅助）→ 挑选/反馈/接受
       ├─ accept     → [①] 定稿：snapshot → 应用到正式关卡/序列
       ├─ continue   → [①] 反馈结构化 → 轴收缩 → 下一轮候选 → 回到派发
       └─ reject_all → [①] 回规划层重新出方案
```

### 13.6 跨会话恢复

```
美术关掉浏览器 → session.state 保持 awaiting_judgement
  → 次日打开 /lighting/session/{id} → 候选、指标、历史轮次全部还在
  → 继续判断 → 流程照常推进
```

这是把「等待人工」做成**会话状态**而非进程内循环的直接收益。

---

## 14. 浏览器前端（唯一交互面）

### 14.1 现状：它是什么

- 前端是 **Kitsu 的 fork**（cgwire 开源动画协作平台），Vue 3 + Pinia + vue-router + Bulma + Vite。
- 由主服务器作为**静态资源**托管：`http_method/kitsu/kitsu_front_end.cpp` 从 `root_path_`（= `kitsu_ctx_t::kitsu_front_end_path_`，生产 `D:/kitsu/dist`）读文件，非 `/api` 路径回落到 `index.html`（SPA 兜底）。注册在 `create_kitsu_route_2` 的**最后一行**（`kitsu.cpp:425`）。
- 已经有一层很厚的 Doodle 扩展（构建产物里 `/api/doodle/*` 出现 71 处）：`ai_image`、`model_library/*`、`pictures/*`、`task/*`（含 log / inspect / restart）、`computing_time/*`、`attendance/*`、`deepseek/key`、`key/ji_meng`、`tool/version`。
- **已经会提交 UE 装配任务**：`/api/actions/projects/{project_id}/shots/{id}/run-ue-assembly`。
- **已经在浏览器里直接调 LLM**：产物里打包了 OpenAI JS SDK（`OpenAI` 出现 68 处），key 从 `/api/doodle/deepseek/key` 取（对应 `http_method/other/other.cpp:21`）。

### 14.2 决策 2/3/4 带来的变化

取消 UE 实时交互后，前端从「方案与评审」升级为**唯一交互面**，而且**不再受「看不到 UE 视口」的约束**——因为不需要实时视口，美术看的是渲染出来的静帧，这正是 Kitsu 已经擅长的事（preview files、对比、评论、状态流转）。

### 14.3 浏览器前端负责的内容

| 能力 | 内容 | 复用 / 新增 |
|---|---|---|
| 任务提交 | 选镜头 → 填提示词 → 传多张参考图 → 提交 | 复用 `run-ue-assembly` 提交页与上传组件；新增 lighting action |
| 参考素材管理 | 多张参考图上传、关联、缩略图、标注角度 | 复用 `/api/data/entities/{}/preview-files`、`/api/doodle/pictures/*` |
| **人工闸门 1：Spec 确认** | 展示 intent + 目标区间 + 冲突维度，人工裁决 | **新增** |
| **人工闸门 2：候选判断** | 候选接触表（并排 + 指标辅助）→ 挑选 / 反馈 / 接受 | **新增，核心页** |
| 反馈输入 | 选一个继续 + 反馈（**滑块为默认**，自然语言可选） | 新增；滑块映射到轴增量，自然语言经 LLM 结构化；复用 AIScript 的对话式交互形态 |
| 轮次与历史 | 每轮的候选、渲染、指标、判断记录、时间线 | 新增 `ai_light_iteration` 页面 |
| A/B 对比 | 参考 vs 候选并排、直方图/调色板叠加（显示空间） | 新增对比视图 |
| 指标看板 | 区间满足率 + 每项落点（在区间内/偏高/偏低） | 新增 |
| 预设库管理 | 灯光预设 CRUD、标签、示例图、embedding 重建 | **直接照抄 `/api/doodle/model_library/*` 的页面模式** |
| 批量派发 | 一集/一个序列批量生成、进度、失败重试 | 复用 `/api/doodle/task/*`（log / inspect / restart） |
| 评审与审批 | 美术/总监签核、评论、附件、状态流转 | **复用 Kitsu 的 task status / comment / attachment 机制** |
| 通知 | 一轮渲染完成 / 等待判断 → 通知美术 | 复用 Kitsu 通知机制 |

### 14.4 明确不属于前端的部分

- 实时视口反馈（**已取消，不再需要**）
- 逐 op 接受/拒绝（改为**候选级**判断，见判断六）
- SceneFacts 采集（由 UE commandlet 的 `inventory` 做）

### 14.5 三条必须守住的接口约定

1. **EXR 不下发浏览器**。浏览器显示不了线性 EXR。服务端给两份：PNG（显示空间，显示 + 判据 + VLM 判分）+ 线性 EXR（工程检查，留在服务端）。前端只拿 PNG + 数值指标。
2. **规划逻辑不要放到前端**。现有 AIScript 的模式是浏览器直连 DeepSeek，但灯光规划**不能照抄**：轮次编排必须在服务端，前端再实现一份会让提示词模板、Spec 词表、IR schema 校验在 JS 与 C++ 各存一份，必然漂移。前端只调 `/api/doodle/ai/light/*`。
   （`/api/doodle/deepseek/key` 那条把 key 交给浏览器的接口，灯光功能不要复用。）
3. **对比视图不要做差分图**。参考是异视角且被调过色，逐像素差分没有意义、还会误导美术。用**并排 + 统计指标对照 + 目标区间标记**。

### 14.6 需要新增的前端页面/路由

- `/lighting/new` — 提交：提示词 + 多张参考图
- `/lighting/spec/:specId` — **人工闸门 1**：Spec 确认与冲突裁决
- `/lighting/session/:sessionId` — **人工闸门 2 + 主工作台**：候选接触表、指标辅助、挑选/反馈/接受、轮次时间线
- `/lighting/presets` — 预设库管理
- `/lighting/jobs` — 批量任务与农场进度
- 镜头页（Shot）增加「AI 灯光」入口按钮，与现有 `run-ue-assembly` 并列

---

## 15. 已锁定的边界与实测待办

### 15.1 已锁定

| 项 | 结论 | 落点 |
|---|---|---|
| 每轮候选数量 | **6 个** | §2 决策 9、§6.2 |
| 判断粒度 | **只做候选级挑选**，不做逐 op 接受/拒绝 | §2 决策 10、§8.1 |
| 反馈形式 | **滑块与自然语言都支持，滑块为默认** | §2 决策 11、§14.3 |
| 会话过期 | **美术 4 小时以上不判断算过期**（不改状态，只置标志） | §2 决策 12、§10.3 |
| 人工对位相机 | **不做** → Tier B 多视角联合反解随之取消 | §2 决策 14、§5.6 |
| 几何/材质 | **不允许改动**；范围锁定为灯光 + 环境光/雾/天空 + 后处理/调色 | §2 决策 13、§6.6 |
| 预设库首批 | **由美术整理一批**（三点布光 / 伦勃朗 / 剪影 / 夜景霓虹…） | §2 决策 15、§6.4 |
| 后处理/调色 | **允许 AI 修改**（决策 7 确认，不再是待确认项） | §2 决策 7 |
| 参考性质 | **以全局调色为主** → 统计量可信度高 + 全局调色可反解 | §2 决策 16、§5.2、§9.2 |
| 性能护栏 | **暂不设硬上限**（灯数、单轮渲染时间） | §2 决策 17、§6.6、§12 |

**已无阻塞性待确认。** 方案前提全部闭合，可以开工。

### 15.2 实施中需实测确认（不阻塞开工）

1. **性能基线**：灯数与单轮渲染时间已确认暂不设限，但需要在 M1 实测记录——6 个候选 × 实际灯数的单轮耗时，作为 M6 是否拆分多机并行的依据，也是后续要不要加回硬上限的依据。
2. **全局调色反解的实际精度**（§9.2）：需在真实参考上验证「分布匹配求得的全局变换」能否干净地落到 UE 的 Color Grading 参数上。若映射误差大，退化为 §9.3 的灵敏度标定搜索——流程不受影响，只是少了一个热启动。
3. **统计量的实际可信度**：虽然已确认以全局调色为主，仍应在首批真实参考上抽查是否存在局部改图污染，据此决定是否需要启用外点剔除。
4. **方舟 Responses 的四条实测项**（§15.6 的推理通道已按文档实现并离线单测，但以下四条只能拿真 key 打一次才知道）：
   - **模型名**：`deepseek-v4-1-flash-260910` 这类纯 Model ID 是否被本账号接受
     （AK/SK 鉴权才必须用 `ep-*` 接入点；我们走 API Key，理由见 §15.6）；
   - **`text.format` 的形状**：文档正文是平铺 `{type,name,schema,strict}`，而 Python SDK 的示例是
     chat 那种嵌套变体 —— 两边冲突。实现默认发**平铺**形状，若真机 400 就降级（降级点已收敛在
     `build_inference_body()` 一处）；
   - **`input_image` 的可用性**：本账号的该模型是否真的接受图片输入（参考图统计要喂图）；
   - **`thinking: disabled`** 是否被接受（有的模型只允许 `enabled`/`auto`）。
5. **FK 前向引用已实测通过（无需再验）**：`seedance2_task_2` 注册在 `ai_light_iteration` **之前**，
   新加的 `ai_light_iteration_id` 外键在 CREATE TABLE 时指向了尚未注册的表 —— SQLite 允许这样做，
   实测建表/重建/外键语义用例（`sqlite_schema` / `sqlite_rebuild` / `sqlite_fk_*` / `ai_light_schema`）
   全绿。这一条列在这里是为了**关掉**它，不必再怀疑。

### 15.3 已落地（数据库层）

| 项 | 落点 |
|---|---|
| 六张表的结构体 | `src/doodle_core/metadata/ai_light/`（`common.h` / `rig.h` / `spec.h` / `session.h` / `reference.h` / `iteration.h` / `candidate.h` / `preset.h` / `ai_light.h`） |
| ORM 注册 | `sqlite_database.cpp` 的 6 个 `reg_table` 块（含定稿唯一索引） |
| 迁移 30 → 31 | `sqlite_upgrade.cpp` 的 `upgrade_3_t` + `upgrade_3()`；版本常量拆出 `g_version_30`，避免 `upgrade_2_t` 把库直接标成最新而让这一步被静默跳过 |
| 查询方法 | `sqlite_database.h/.cpp` 的 19 个 `get_ai_light_*` |
| 产物路径 | `doodle_core/metadata/kitsu_ctx_t.h` 的 `get_ai_light_session_dir` / `_reference_dir` / `_reference_file` / `_iteration_dir` / `_candidate_file` / `_backup_dir`（**唯一**布局真相来源；原先在 `core/entity_path.h/.cpp` 的四个 `get_ai_light_*_path` 已删除，见坑 6） |
| 任务类型 | `server_task_info_type::ai_light` |
| 测试 | `src/test/core/ai_light_schema.cpp`（建表、版本门控、JSON 往返无损、定稿唯一性、过期时限经 TEXT 列往返后仍判得对） |

**实现期发现、值得记住的三个坑**：

1. **ORM 的列类型不覆盖嵌套结构体与 `std::map`** —— 只有 `nlohmann::json` 可以。所以 `rig` / `metrics` / `intent` / `targets` / `judgement` 全部以 JSON 列落库，
   靠 `get_xxx()` / `set_xxx()` 做类型化访问。这不是偷懒：直接把 `lighting_rig_t` 当列成员会编译不过。
2. **`user_version` 与「每一步各写自己的目标版本」必须配套** —— 若 `upgrade_2_t` 仍写 `g_current_version`，
   29 版库会被直接标成 31，`upgrade_3_t` 的 `> 30` 门控随即跳过建表，而且**版本号上看不出任何异常**。
   `src/test/core/sqlite_upgrade.cpp` 里专门有一组用例盯住这种「静默跳过」。
3. **本库的 ORM 表达不出带 `= 值` 条件的 partial index** —— SQLite 明确禁止 partial index 的 `WHERE` 里出现绑定参数
   （`parameters prohibited in partial index WHERE clauses`），而 `create_unique_index().where(c(&T::col) == true)`
   生成的正是带 `?` 的比较式；只有 `== nullptr` 会生成无参数的 `IS NULL`（`entity` 表的 `parent_id` 先例靠的就是这一点）。
   所以「每会话至多一个定稿候选」改用**可空列 + 普通唯一索引**表达：
   `ai_light_candidate.accepted_session_id` 定稿时写入 `session_id`、否则保持 NULL，
   写入必须走 `mark_accepted()`（它同时维护 `is_accepted_`，两列一旦分叉约束就失效）。
   选这条路而不是去改 ORM 的 `to_str_value_t` 让它内联字面量：那要顺带重写 string / uuid / 枚举 / optional
   各自的 SQL 字面量规则（等于把 binder 的存储格式再实现一遍），而 `insert_t` 的触发器路径也在用那套代码 ——
   为了一个索引去动全库查询共用的序列化，代价与风险都不成比例。

### 15.4 已落地（路由层 + 编排层 + 状态机）

| 项 | 落点 |
|---|---|
| 路由声明 | `src/doodle_lib/http_method/kitsu/ai_light.h`（12 个路由类） |
| 路由处理 | `src/doodle_lib/http_method/kitsu/ai_light.cpp` |
| 编排 / 状态机 | `src/doodle_lib/http_method/kitsu/ai_light_orchestrator.{h,cpp}` |
| 路由注册 | `http_method/kitsu.cpp` 的 `create_kitsu_route_2`（**只注册主服务器**，客户端不跑 LLM） |
| 图片变体导出 | `http_method/kitsu/preview.{h,cpp}` 的 `preview::save_variants`（从匿名命名空间提到 `preview` 命名空间，供定稿复用；不另写一套尺寸，否则预览列表页读到的图与既有预览文件不一致） |
| 读模型 | `ai_light_session_detail_t`（`doodle_core/metadata/ai_light/view.h`），六表一次聚合，并带出 `candidate_count_per_iteration`（前端不自己写死 6） |
| 测试 | `src/test/core/ai_light_state.cpp`（17 用例 / 167 断言，含 JSON 形状回归、「超时会话仍可判断」回归、「无参考图不蒸馏」、「`awaiting_spec_confirm` 可重出」，以及钉住「预分配 uuid 不被 ORM 改写」的延迟插入回归 —— 见坑 6）+ `src/test/core/sqlite_not_found.cpp`（5 用例 / 11 断言，钉 ORM 的 404 语义）+ `src/test/core/ai_light_paths.cpp`（8 用例 / 23 断言，钉产物落盘布局，见坑 6）+ §15.5 的统计层套件；全量回归 109 用例 / 926 断言通过 |

**状态机**（`advance_session` 是唯一的推进入口，`/spec`、`/spec/confirm`、`/iterate`、
镜头页入口都调它，所以「同一步骤从不同入口进来行为一致」是结构性保证的）：

| 触发 | 起始状态 | 结果状态 |
|---|---|---|
| 建会话（`POST /session`、镜头页入口） | — | `draft`（**不到** `awaiting_spec_confirm`：参考图只能在会话存在后上传，蒸馏由 `/spec` 触发） |
| `POST /spec`（改提示词 + 可选排除参考图；**要求已有参考图**） | `draft` / `awaiting_spec_confirm` | `awaiting_spec_confirm`（重出则 revision+1，旧的留在历史里） |
| `POST /spec/confirm` | `awaiting_spec_confirm` | `generating`（接缝失败则回滚，Spec 确认**保留**） |
| `POST /iterate` | `generating` | 仍 `generating`（已派发，等渲染） |
| `mark_iteration_rendered()`（worker 回填，M3 接入） | `generating` | `awaiting_judgement`（**此时才起算 4 小时**） |
| `/judge` accept | `awaiting_judgement` | `accepted`（终态） |
| `/judge` continue_iteration | `awaiting_judgement` | `generating` |
| `/judge` reject_all | `awaiting_judgement` | `rejected`（终态） |

本表的 `POST /spec`、`POST /spec/confirm`、`POST /iterate`、`/judge` 都是**简写**，
完整路径一律是 `/api/doodle/ai/light/session/{session_id}/<动作>` ——
`session_id` 走**路径**，不在查询参数、也不在请求体里（路由表见 §13.1）。

**没有「过期」这一行**：4 小时超时**不是一次状态转换**，而是 `awaiting_judgement` 上的一个
实时派生标志（`is_judgement_overdue`）。曾经它是 `suspended` 状态 + 常驻扫描器，已删除 ——
理由见 §10.3 与决策 12：那个状态会让超时后的候选再也判断不了。

`advance_session` 的分派**只看状态**，所以「同一个状态只能有一个后继步骤」是结构性约束。
`awaiting_judgement` 没有可推进的步骤（该做的是 `/judge`，不是再推一步）。
这也是删掉 `suspended` 的另一个理由：它让「重出方案」和「出候选」两种恢复意图挤进同一个
状态，而状态本身区分不了它们 —— 当时的症状是 `/iterate` 的错误文案说「生成候选」，
实际却走成了「重出 Spec」。

### 15.5 已落地（参考图统计层 = §5.1 的①）

| 项 | 内容 |
|---|---|
| 模块 | `doodle_lib/http_method/kitsu/ai_light_stats.{h,cpp}`，**不含 HTTP 依赖**（与 `ai_light_orchestrator` 同性质，单测直接调） |
| 入口 | `analyze_display_image(const cv::Mat&)` 纯函数（显示空间 8bit BGR → `reference_image_stats_t` + `confidence`）；`analyze_reference_file(const FSys::path&)` 读已落盘 PNG，读不出返回 `nullopt` |
| 接入点 | `http_method/kitsu/ai_light.cpp` 的 `reference_route::post`：在 `FSys::rename` **之后**统计（统计描述的是最终存下来的工件），写 `set_stats(...)` + `stats_confidence_`；读不出直接 400 |
| 度量口径 | 见 §5.2（对比度 = p90 − p10；调色板 k=5 且固定随机种子；`retouch_score_` 是下界）与 §8.2（色温是显示空间规则的唯一例外） |
| 导出 | 两个函数都带 `DOODLELIB_API` —— `doodle_lib` 是 SHARED、`test_main` 是独立 exe，不加导出宏测试会**链接失败** |
| 置信度 | `min(1, sqrt(w·h/(1280·720))) × (1 − 0.5·retouch) × content_factor`，clamp 到 **[0.05, 1]**。地板保证「分析成功」≡`references_have_stats()` 为真（低置信度是数值，不是缺失） |
| 测试 | `src/test/core/ai_light_stats.cpp`（15 用例 / 59 断言），全喂合成图：纯黑/纯白/渐变的分位与削波率、中灰 → **6500K**（整条 sRGB→线性→XYZ→McCamy 链的锚点）、冷暖排序、纯色块回落、调色板簇数与 **RGB 通道序**、确定性、灰度/BGRA 等价、空图与 32F 抛错、对称暗角 vs 单侧压暗、痕迹压低置信度、分辨率单调、读文件路径 |
| 不改 | `convert_to_png` / `image_info_t` / 任何数据库列与 JSON 字段（`reference_image_stats_t` 的七个字段是已锁定契约，本轮只是把它填满）；`user_version` 保持 31 |
| 未做 | §5.3 的多图聚合与冲突检测（②）；`is_outlier_` / `excluded_reason_` 仍恒为 `false` / 空；光晕、局部重绘这类频率域痕迹（所以 `retouch_score_` 是下界）；已上传历史行的统计**不回填** |

**统计层顺手堵掉的一个洞**：`preview::convert_to_png` 对 `.png` 输入**不做解码校验**
（`if (l_cv.empty())` 那个判断在非 PNG 分支里），所以「后缀是 .png、内容不是图片」的文件
会带着 `size_ = 0×0` 一路走到入库，留下一条尺寸 0×0、看起来只是「统计不可用」的坏记录。
新代码在统计这步读不出就 400，把这个洞堵住了 —— 但没有改 `convert_to_png` 本身
（它还被 `pictures.cpp` 用，改动会外溢）。

**一个实测暴露出来的 OpenCV 细节**：第一版拿浮点亮度直接做 `[0,1]` 的 256 bin 直方图，
结果**整幅纯白图的分位全是 0** —— 原因是 `cv::calcHist` 的范围是 `[low, high)` 半开区间。
完整分析与修法见 §15.4 的坑 5。

**接缝（本轮返回 501，不是占位桩）**：

```cpp
// 三个接缝都在 ai_light_orchestrator.cpp，参数被显式 (void) 引用一次，
// 免得「未使用参数」的告警掩盖了「接缝没实现」这件事。
generate_spec(req)                       -> spec_generation_result_t    // 缺 deepseek_client（参考图统计已落地，见 §15.5）
generate_candidates(req)                 -> candidate_generation_result_t // 缺 deepseek_client + 预设检索
structure_feedback(session_id, text)     -> std::vector<axis_step_t>    // 缺 deepseek_client
```

**接缝契约（D3）**：接缝抛异常 ⇒ `advance_session` 把会话状态**回滚到入口状态**并记录
`failure_reason_`，然后原样重抛。`failed` 保留给不可恢复错误，不用来表达「能力还没到」。
理由是：半成品比失败更糟 —— 一个「有 Spec 但没有候选」的会话会让前端以为可以判断了。
接缝实现者不得留下非幂等的部分写入。`/spec/confirm` 的 Spec 确认**不随回滚撤销**：
确认是人工动作，不该因为下游能力缺失而被撤销。

**其它本轮定下的取舍**：

1. **`/judge` 的滑块通道本轮即可用**。`feedback_axes` 直接从请求体取（决策 11：滑块为默认），
   `structure_feedback` 只是补充通道；捕获到 501 就降级为「文本原样留存 + 只用滑块轴」，
   判断本身完全成立。所以「缺 LLM」不会让闸门 2 不可用。
2. **定稿的三处写入必须同事务**。`preview_file` 行、候选的定稿标记与 `preview_file_id_`、
   会话状态分两个事务提交就会出现「预览文件已建但候选没标定稿」的半成品。
   所以调用方（HTTP 层）负责碰文件（OpenCV 读图 + `save_variants` 落变体），
   `apply_judgement` 负责一次事务落库 —— `preview_file` 按**值**传入，
   因为 ORM 的 `values()` 会回填 `uuid_id_` / `created_at_` 这类服务端生成字段。
3. **`is_selected_` 与 `is_accepted_` 是两个语义，不合并**。前者专指「被选为下一轮基准」
   （续轮 warm start 的起点），后者指「定稿」。定稿时**不**置 `is_selected_` ——
   定稿没有下一轮，混用会让续轮查询拿到一张已定稿的图。
4. **中间候选与参考图都不进 `preview_file`**（决策 1）。每轮 6 张全注册会把任务的预览列表
   冲垮，也会把「交付物」这个概念稀释掉。中间图走 `/candidate/{id}/image`，
   EXR 一律不下发浏览器（线性数据、体积大、浏览器渲染不了）。
5. **建会话接口不吞任何接缝错误 —— 因为它不调接缝。** 早先建会话会立刻跑第一步，
   所以需要「吞掉 501 以免前端拿不到 session id 而重试建重复会话」这套说法；
   现在建会话只 insert，`POST /spec` / `/iterate` 的 501 一律原样返回 ——
   那是能力缺口，前端应该看到，而不是被包装成一个「成功但少了点东西」的 2xx。
6. **「过期」不做成会话状态，也不留后台扫描器**（决策 12）。它是 `judgement_due_at_` 的
   纯派生标志，读模型带出 `is_judgement_overdue` 即可。做成状态会**关掉唯一的判断入口**：
   `apply_judgement` 只接受 `awaiting_judgement`，而没有任何路径能从那个状态回到
   `awaiting_judgement`，于是超时后的候选再也判断不了 —— 详见 §10.3。
   代价是前端必须自己看这个标志来催办（没有服务端推送），换来的是判断入口永远可用、
   少一个常驻后台任务、少一个「任务挂了没人发现」的失败面。

**实现期发现、值得记住的六个坑（本轮）**：

1. **`std::optional` 的 JSON 序列化用库自带的，别自己写；而 `nlohmann::json{*opt}` 是列表初始化，会静默变成单元素数组。**
   先说结论：nlohmann 从 C++17 起自带 `std::optional` 的读写（本工程 vcpkg 装的是 3.12.0，
   编译期 `JSON_HAS_CPP_17` 成立，所以一定生效）——
   读在 `nlohmann/detail/conversions/from_json.hpp:52-63`（`null` → `nullopt`，
   否则 `opt.emplace(j.get<T>())`），写在同目录 `to_json.hpp:271-281`
   （有值 → `j = *opt`，无值 → `j = nullptr`）。所以**直接赋值**就是对的形式，
   既不需要三元转换也不需要助手函数：

   ```cpp
   j["judgement_due_at"] = p.judgement_due_at_;        // 对
   if (j.contains("x")) j.at("x").get_to(p.x_);        // 对，null 自动变 nullopt
   ```

   而**花括号是错的**：`nlohmann::json` 有 `basic_json(initializer_list_t)` 构造函数，
   列表初始化会**优先**匹配它，于是 `j["at"] = nlohmann::json{*p.at_}` 写出的是
   `{"at": ["2026-...Z"]}` 而不是 `{"at": "2026-...Z"}`。这个坑极其隐蔽：
   代码合法、编译通过、写库成功，只有读回来时才在 `from_json` 里对着数组抛
   `type_error.303`，而报错信息里**完全看不出是哪个字段写坏的**。同一个坑对
   `std::string` / `double` / 结构体一样成立（`rig.h` 的 `ies_` 就是这么丢的）。
   要用显式转换就用括号 `nlohmann::json(x)`（括号走普通重载决议）。
   说明写在 `common.h`，并加了一条专门钉住 JSON **形状**（而不只是值）的回归用例 ——
   值相等掩盖不了多包一层数组。
2. **「查不到行」的异常语义在 ORM 层统一了，不要在调用点各自翻译。**
   起因是一个很隐蔽的类型关系：`to_single()` 原来抛 `std::runtime_error("No rows returned")`，
   它与 `sqlite_orm_exception` 是**兄弟**（都直接继承 `std::runtime_error`），
   所以 `catch (const sqlite_orm_exception&)` 接不住它 —— 于是「取一个不存在的 id」
   会一路冒到 `http_session_data` 的 `catch (...)` 变成 **500**，而正确答案是 404。
   当时的补救办法是在 `ai_light.cpp` 里包一层 `try_get_by_uuid()`，把**所有**
   `std::exception` 都翻成 `nullopt` → 404。那个办法有两个毛病：每个调用点都要写一遍；
   而且它顺手把「库真的坏了」也伪装成了「资源不存在」，排查方向直接被带偏。

   现在改成在**抛出点**声明语义：

   - `orm::sqlite_orm_exception` 新增 `code_status_()`（默认 `bad_request`，
     沿用 `doodle_error` 的访问器命名），既有抛出点（外键 NO ACTION 撞
     `FOREIGN KEY constraint failed`、`rebind_range` 参数数量不匹配等）行为不变；
   - `to_single()` 结果集为空时抛 `sqlite_orm_exception{404}`，消息里带上**表名**
     （`"ai_light_preset 中查不到该行"`）—— HTTP 层把它直接当 404 的响应体，
     而 `"No rows returned"` 既看不出是哪张表，也看不出是「不存在」还是「查询写错了」；
   - `http_session_data.cpp` 的 `catch (const orm::sqlite_orm_exception&)` 直接取
     `e.code_status_()` 当响应码，不再硬编码 400；
   - `to_optional()` 语义不变（空结果集 → `nullopt`），它是「允许没有」的显式通道。

   于是 `ai_light.cpp` 的 `try_get_by_uuid()` 整个删掉，四处调用点直接 `get_by_uuid`；
   `build_session_detail()` 也**只吞 404**，其它 `sqlite_orm_exception` 照常上抛成 500。

   契约钉在 `src/test/core/sqlite_not_found.cpp`：断言的是**状态码数值**而不是
   「抛了异常」—— 状态码是行为，不是注释。

   **这次改动顺带暴露的两个既有缺陷**（同一个「异常类型对不上」的病根，
   不修的话新状态码会从它们的 catch 缝里漏出去）：

   - `auth.cpp` 的 `catch (const doodle_error&)` 从来没生效过 —— `get_person_for_email`
     走的 `to_single` 抛的是 `std::runtime_error`，与 `doodle_error` 是兄弟，接不住。
     于是「给未注册邮箱发重置」一路变成 500，而作者写的 400 + `Email not listed in
     database.` 是死代码。已改成只翻译 404。
   - `login.cpp` 里 `get_person_for_email` 查不到时，如果放任 ORM 的 404 出到响应，
     等于给**未登录**接口开了一个「这个邮箱注册过没有」的查询口（用户枚举）。
     已自己翻译成 400 + 「邮箱或密码错误」，与「密码错误」回同一句话，不区分两者。

3. **删掉一个「既是枚举值又是数据库列」的状态时，可以不升 `user_version`** ——
   但成立的**前提必须逐条核实**，不能当成通则。

   本轮删掉了 `ai_light_session_state::suspended` 与 `ai_light_session::suspended_at_`。
   `sync_schema()` 跳过已存在的表（不加列也不改列），所以「不迁移」本身不需要做什么；
   真正让「不迁移」成立的是另外两条：

   - 那一列是**可空**的（`add_column` 没带 `not_null()`），老库里留着它不会让任何 `insert` 失败；
   - ORM 的列清单来自 `reg_table` 声明，`select(object<T>())` 与 `insert` 都只碰声明过的列，
     所以老库里那一列对代码是**惰性**的（既不读也不写）。

   于是：老库留着这一列（无害），新建库没有这一列，`user_version` 保持 31。
   代价是**同一个 schema 有两个物理形态**。这对只在测试环境跑的分支可以接受
   （见 memory 里记录的分支约定：非 `master` 分支不进生产），但一旦要进生产，
   要么补一次重建，要么明确接受两种形态并存。

   对照 `preview_file.source_file_id`（`sqlite_database.cpp:689-694`）：那里**保留**了列声明，
   注释写得很清楚 —— 重建时列的拷贝清单来自 ORM 声明，删掉声明会让该列在将来的重建中
   真的消失，而它同时还出现在 `preview_file` 的 JSON 输出里，删它属于接口变更。
   换句话说：**列声明是「重建之后还要不要这一列」的唯一真相来源**，
   删不删它取决于你要不要它在重建后存在，而不是取决于它现在有没有人用。
   本轮 `suspended_at_` 是连 JSON 一起删的（它从未进过生产，改接口无成本），
   所以两种处理都对，但**必须是有意选的**，不能是顺手删的。

4. **「未实现的接缝」会把流程缺陷掩盖成正常行为 —— 所以接缝的 501 要当成一等状态来对待。**

   建会话接口曾经在同一个请求里「insert 会话 + 立刻蒸馏 Spec」。这看起来很贴心
   （点一次按钮就拿到待确认的方案），实际是错的：Spec 的输入是提示词 + 参考图，
   而参考图只能在会话存在**之后**上传（要 `session_id`，而它是这个请求的返回值），
   所以那一刻 `get_ai_light_references()` **必然为空**，蒸出来的是丢掉主输入的结果。

   为什么没被发现：`generate_spec` 还没实现，抛 501 → 回滚把状态退回 `draft` →
   后续 `/spec` **恰好**还能走通。全部测试、全部手工验证都是绿的。
   **缺陷被安排在「接缝实现」那一刻生效** —— 而那正是这个工具第一次真正干活的时候。

   教训有两条，都可复用：

   - **一个请求里不要做「创建 + 依赖创建结果的后续步骤」。** 依赖项（这里是参考图）
     在响应返回前不可能存在，所以那一步只能拿到空输入。这类缺陷不会报错，只会产出
     一份看起来正常、实际缺了主输入的结果。
   - **别把「501 被吞掉」当作流程正确的证据。** 回滚把状态退回入口，会让下游步骤
     意外地仍然可用 —— 于是失败路径比成功路径更「能跑通」。反过来说，
     凡是有接缝抛 501 的流程，都必须再问一次：**接缝实现之后，这条路径还成立吗？**
     本轮就是这样问出来的：把 `generate_spec` 换成「返回值」而不是「抛异常」，
     会话就会停在 `awaiting_spec_confirm`，而那时 `/spec` 已经进不去。

   顺带一条：修好之后蒸馏改由 `/spec` 触发，于是"同一个状态只能有一个后继步骤"
   这条约束需要重新核对 —— `awaiting_spec_confirm` 上同时挂着 `/spec`（重出）与
   `/spec/confirm`（确认后出候选），看起来违反了它。其实不违反，因为
   `/spec/confirm` 会**先把状态改成 `generating`** 再调 `advance_session`，
   所以 `advance_session(awaiting_spec_confirm)` 只有一个意图。**判断这条约束时，
   要看的是 `advance_session` 的分派输入，而不是「哪些路由会碰这个状态」。**

5. **`cv::calcHist` 的范围是 `[low, high)` 半开区间 —— 值恰好等于上界的像素会被静默丢掉。**

   第一版统计层用 256 个 bin 覆盖浮点亮度的 `[0, 1]`，看起来毫无问题。实际后果是：
   **整幅纯白图的直方图是空的**（所有像素的亮度都是 `1.0` = 上界，一个都没计进去），
   于是 `cv::sum(hist) == 0`，三个分位一起返回 0，对比度、`content_factor`、置信度全错。
   而且它只在「有像素恰好取到上界」时发作 —— 纯白、成片过曝白、纯色块都会命中，
   黑图（下界是闭的）、灰图、渐变图都正常，所以只测后几种的组合漏掉它。

   修法不是「把上界加个 epsilon」（那是碰运气），而是**换一个 bin 与取值一一对应的量**：
   先把亮度量化成 8bit 等级，再用 `[0, 256)` 的 256 个 bin 统计。等级 0 与 255 都是闭的，
   纯黑恰好得 0、纯白恰好得 1，顺带还消掉了「全在 bin 0 却报 0.002」这种别扭值。
   一般化的教训：**凡是库函数收「范围 + 桶数」，先确认边界是开还是闭，再找一个
   边界不可能被取到的量纲**（这里是 256 个离散等级，而不是连续的 `[0,1]`）。

6. **把产物从「项目目录下」挪到「资产根 + uuid 命名」，路径计算里的元数据依赖会一起消失 ——
   依赖消失之后，原来被它挡着的两个缺陷才露出来。**

   旧写法是 `session_dir = project.path_ / get_ai_light_session_path(episode, ground_extend, 会话 uuid)`，
   为了拼出这条相对路径得先查三次库（镜头实体、项目、镜头资产扩展），于是长出了三条「算不出目录」的
   失败分支。改成

   ```cpp
   root_ / "ai_light" / FSys::split_uuid_path(fmt::to_string(会话 uuid))   // 见 §10.2
   ```

   之后：`session_dir` 与编排层算候选路径**都不用再查库**（编排层那个只为算基准而存在的
   `resolve_session_paths` 连结构体一起删掉了）；「镜头没有父实体」「镜头缺少资产扩展信息」
   两条 400 随之消失（它们本来只是路径计算的前置条件，不是业务规则）；工作机侧同理只依赖
   `root_`，而任务载荷里已经带了 `session_id` / `iteration` / `candidate_id`，足以重算出**一模一样**的路径。

   顺带露出来两个缺陷 —— 都在当前**不可达**的分支里（接缝还抛 501），所以测试全绿也照不出来：

   - `reference_route::post` 用的是 `session_dir(...).parent_path() / "ref"` —— 那是会话目录的
     **兄弟**目录（与 `entity_path.h` 自己声明的 `<会话 uuid>/ref/` 都不符）。后果：同一镜头的所有会话
     共用一个 `ref/`，两个会话的 `ref_00.png` 互相覆盖；而删会话时 `backup_session_dir` 搬的是
     `<会话 uuid>` 目录（那目录压根不存在），参考图全成孤儿。已改为走 `get_ai_light_reference_dir()`
     （在会话目录**之内**），并由 `ai_light_paths.cpp` 用 `parent_path()` 相等把它钉住。
   - `ai_light_iteration.server_task_info_id_` 与 `ai_light_candidate.iteration_id_` 落库为 **nil**：
     两处都在 ORM 的 `values()` 回填 uuid **之前**读了 `l_task.uuid_id_` / `l_it.uuid_id_`。
     后果是 `mark_iteration_rendered()`（按任务 uuid 反查轮次）永远 404，候选也永远「不属于本轮」。
     修法与「候选渲染图用 uuid 命名」同源：**uuid 必须在被引用之前显式分配** ——
     `orm::insert().values()` 只回填 nil 的 uuid（`orm/sqlite_orm/orm/insert.h:92-94`；
     批量版本在 `:119-128`），预先分配的值会被保留，所以显式 `core_set::get_set().get_uuid()` 是安全且必要的。
     这条「预分配不被改写」由 `ai_light_state.cpp::pre_assigned_uuid_survives_the_deferred_insert` 钉住：
     它按生产次序先定 uuid、再算文件名/被引用，走**延迟执行**（`values()` 之后对象即销毁、语句到
     `run_sql()` 才落库），最后校验库里的 uuid 能重算出磁盘上那个文件名、任务 uuid 能反查回轮次。
     顺带确认了两件事：`uuid_id` 不是主键（主键是自增 `id`，所以它不会被 insert 语句当主键跳过），
     以及 `bind_value_t` 装的是 `std::any`（值拷贝），延迟执行不依赖源对象的生命周期。

   一般化的教训两条：

   - **用 uuid 做文件名时，「谁先生成这个 uuid」就变成了正确性问题。** 一个实体只要它的 uuid 会进入
     路径（或被别的行提前引用），就必须在构造那一刻定下来。等 ORM 回填是最省事、也最容易在
     「恰好不可达」的路径上埋雷的写法。先例早就有：`build_accepted_preview_file` 里那段注释
     （「uuid 必须在这里就定下来，不能等 ORM 在 insert 时回填」）说的就是同一件事。
   - **不可达分支上的缺陷不会自己消失。** 本轮修掉的两处都在 501 之后，改之前测试全绿、改之后也全绿，
     唯一能暴露它们的是「读一遍这段代码，问它一旦可达会怎样」。这与坑 4 是同一个教训的另一面。
   - **测试夹具的一个既有前提**（本轮踩过一次，记清楚免得再踩）：`test_main` 里凡是要碰库/单例的
     用例，都**必须先构造一个 `app_base`**。原因：`doodle_lib::impl::self` 是文件级静态指针，
     只在 `doodle_lib` 构造时被赋值（`doodle_lib.cpp:36/40`），而 `doodle_lib::Get()` 直接解引用它
     （`:52`）—— 没有活着的 `app_base`（它持有 `doodle_lib_ptr`）时就是**空指针解引用**。
     `ai_light_state.cpp` 里每个用例开头那句 `app_base l_app{};` 就是干这个的。
     实证：新加的「预分配 uuid 不被改写」用例最初漏了这行，**单独跑**该用例就在夹具构造阶段
     崩在 `address 0x10`（跑全量却可能不崩 —— 前面的套件已经把 `impl::self` 指到别处了，
     于是变成"悬垂但看起来能用"，比直接崩更难查）。补上 `app_base` 即通过。
     换句话说 `g_ctx()` 在测试里**是可用的**，前提就是那句 `app_base l_app{}`；
     将来给 `generate_candidates` 写端到端用例，用现有 fixture + 这行即可。
     而 `ai_light_paths.cpp` 走的是另一条路：它在**栈上**构造 `kitsu_ctx_t` 直接调那六个方法
     （头文件内联、只读 `root_` 与入参），完全不碰库与单例 —— 所以它既不需要 `app_base`，
     也不会被这套单例语义牵连。

### 15.6 已落地（推理接入：火山方舟 Responses API）

三个接缝（§15.5 末尾）缺的都是「一个能同步拿到结构化文本的 LLM」。本轮把这件事落成
**可用的推理通道**：`seedance2_client` 除了异步生成，还能走方舟的 **Responses API** 做同步推理，
并且把任务、token、素材 URL 全部接到既有的 `sd2::task` 管线上。接缝本身仍抛 501（下一步再接）。

| 项 | 落点 |
|---|---|
| 推理请求体 / 响应解析 / 重试 | `doodle_lib/http_client/seedance2_client.{h,cpp}` 的 `build_inference_body` / `parse_inference_response` / `extract_output_text` / `should_retry` / `is_sensitive_content_error` / `strip_code_fence`（全是**静态纯函数**，可离线单测） |
| 同步执行入口 | `run_inference_task(in_task, in_sql, in_client)`（`http_method/seedance2/task.cpp`，声明在 `seedance2/inference_task.h`）：一次调用拿结果 → 任务行**插入即终态** + 扣费，同一个事务 |
| 客户端分发 | `ai_client_base::run_task(const sd2::task&)`；`seedance2_client` 按 `type_ == task_type::inference` 分流到 `run_inference_task`（客户端内部那个），`transfer_station_client` 直接返回失败（它没有推理接口） |
| 推理入口头文件 | `http_method/seedance2/inference_task.h`（`make_inference_client` / `run_inference_task` 的**唯一声明处**）。单独立一份是因为 `reg.h` 里有「相对 `doodle_lib` 根」的 include 写法（测试目标引用不到），且它是路由注册头、依赖太重；`reg.h` 在命名空间外用一行 include 把它带进来 |
| 数据模型 | `task_type::inference`（**只能追加**）；`task.ai_light_iteration_id_`（FK → `ai_light_iteration.uuid_id_`，可空）、`task.result_text_`；`person_token.inference_count_` |
| 产物 URL 鉴权 | 新增 `http_client/url_sign.{h,cpp}`（HMAC-SHA256 短时效签名）+ `http_jwt_or_signed_fun` 基类（`DOODLE_HTTP_SIGNED_FUN`）：cookie/Bearer **优先**，签名只作兜底且**只对 GET** 生效 |
| 鉴权优先级 | `try_session_auth()` → `try_signed_url()` → 抛出统一 401（顺序不能反，理由见下） |
| 转签的路由 | `seedance2/pictures`、`kitsu/ai_light` 的 `reference_image_route` / `candidate_image_route`、四条约 `doodle_ai_volcano_engine_inference_materials_*`（POST 上传仍要会话） |
| 常量 | `doodle_config::g_ark_inference_model{deepseek-v4-1-flash-260910}` / `_timeout{180s}` / `_max_retry{3}` / `_default_max_output_tokens{4096}` / `_default_consumed_tokens{5万}` |
| 测试 | `src/test/core/url_sign.cpp`（11 用例 / 35 断言）、`seedance2_inference.cpp`（17 用例 / 74 断言）、`seedance2_task_model.cpp`（7 用例 / 33 断言）、`seedance2_task_pipeline.cpp`（6 用例 / 35 断言，见下「给没被调用过的代码补测」）；全量回归 **150 用例 / 1103 断言**通过（本轮之前是 109 / 926） |

**接缝怎么用**（下一步接上 `generate_spec` / `generate_candidates` / `structure_feedback` 就照这个形状）：

```cpp
auto l_sql    = get_sqlite_database();
auto l_studio = l_sql.get_by_uuid<ai_studio>(会话的 ai_studio_id);
auto l_client = make_inference_client(l_studio);          // 每次新建, 不共享

sd2::task l_task{};
l_task.type_                  = sd2::task_type::inference;
l_task.backend_               = sd2::task_backend::seedance2;
l_task.user_id_               = 触发人;                    // 同时是签名 URL 里的 uid
l_task.ai_studio_id_          = l_studio.uuid_id_;
l_task.ai_light_iteration_id_ = 轮次 uuid;                 // spec 蒸馏阶段还没有轮次 -> 留 nil
l_task.text_prompt_           = 提示词;                    // 去重(compare_task_similarity)与前台展示都读它
l_task.data_request_          = nlohmann::json{
    {"input_text", ...}, {"images", 参考图路径数组}, {"json_schema", schema}, ...   // 见 build_inference_body
};

const auto l_result = co_await run_inference_task(l_task, l_sql, l_client);
// l_result.status_ / result_text_ / result_json_ / total_tokens_; 任务行已落终态并已扣费
```

`data_request_` 里不要自己拼方舟的 `input[]`/`model`/`store` —— 归一化信封（`input_text` / `images` /
`json_schema` …）由 `build_inference_body()` 翻译，签名与公网前缀由客户端在发请求前补。

**鉴权顺序为什么必须是「先会话、后签名」**：第一版实现把 `try_signed_url()` 放在了前面（谁先写在前面谁赢），
这**违背了本轮定下的「cookie / Bearer 优先」**。两者都能通过，但语义不同：

- 会话凭据里的身份来自**登录态**；
- 签名里的 `uid` 来自 **URL**。

先看签名，一个已登录的管理员点开别人签发的图片 URL 时，就会以 URL 里的 uid 去做
`check_project_access()` 一类的授权校验 —— 权限被 URL 静默改写。先看会话则永远以「当前登录的人」为准，
签名只在**没有会话凭据时**（方舟取图那条路径）才成为身份来源。

实现是 `try_session_auth()`：它**复用** `http_jwt_fun::parse_header()` 而不是重写一遍取凭据逻辑
（否则 cookie / Bearer / 查询参数的取法迟早漂移），只把 401 类的 `http_request_error` 吞掉转签名兜底；
数据库故障这类异常照常上抛，不会被兜底掩盖。两条都不通过时再抛一次会话路径的 401，
于是错误文案与普通受保护路由完全一致（`请先登录` / `cookie 验证错误`）。

这一条**没有单测**：`session_data` 的 `method_verb_` 只能从真实 socket 解析出来，且默认是 `verb::unknown`，
构造一个「带 cookie 且带签名」的请求要塞进整套 socket + 路由。所以它靠的是上面这段代码审查结论，
`url_sign_t::verify` 本身有 35 条断言覆盖。若要补测，得先给 `session_data` 一个可注入的构造路径。

**为什么必须先做「签名 URL」才能谈推理**：素材在站点内是 `/api/seedance2/pictures/<uuid>.png`
这类路径，第三方（方舟）要去取图，而**它带不了我们的 cookie**；会话 JWT 是长效的（cookie 365 天）
且服务端没有吊销名单，不能放进 URL。签名提供的是「短时效 + 绑完整路径 + 绑人 + 只 GET」：
泄漏出去的签名最多让第三方在那一条路径上、以那个人的身份读一张图，且十分钟后失效。
这四条是本轮的实施取舍：

1. **签名只签路径部分**。规范串是 `<path>\n<exp>\n<uid>`，`sign_path` 取 `?` 之前的路径参与签名，
   再把 `exp/uid/sig` 追加到原查询串之后 —— 顺序反过来（先补公网前缀再签名，或把查询串也签进去）
   都会让签名串与校验时的规范串不一致。
2. **带 scheme 的 URL 一律原样返回**。`data:` 内联 base64、对象存储签名 URL、方舟自己的 URL
   都不能补前缀：`to_public_url` 原先只放过 `http(s)://`，会把 `data:image/png;base64,...`
   拼成 `http://ip:38192data:image/png;base64,...`（旧代码对 `images[]` 是**无条件**加前缀，
   这个洞一直在，只是没有 base64 调用方）。
3. **`uid` 是真的加载人员，不是装饰**。`http_jwt_or_signed_fun` 校验通过后会 `load_person(uid)`，
   于是 handler 里的 `check_project_access` / `check_not_outsourcer` 照常生效；`uid` 为空则
   fail-closed（需要人员校验的 handler 会拒绝），而不是「免检」。
4. **签名只是兜底，不改变原有优先级**。`parse_header` 先走 cookie/Bearer，只有都没有签名参数时
   才落到签名 —— 于是前端行为完全不变，跨站 `<img>` 因为 `SameSite=Lax` 发不出 cookie 才会用到签名。

**推理任务的形态（方案 A）**：接缝**同步**等结果，任务行**插入时就是终态**（`succeeded`/`failed`
+ `ended_at_` + `data_response_` + `result_text_`），插入与扣费在**同一个 `run_sql` 事务**里。
后台的 `seedance2_task_run_manager` 只捞 `preparing/queued/running`（`task.cpp` 的 `get_task()`），
所以它**永远看不到这条任务** —— 不需要给 `submit_task` 加等待原语，也不需要预扣/退补。
（仍然给 `submit_task` 补了一个 `succeeded` 分支：万一将来有人从公共提交路径建推理任务，
它不会被下面那条 `else` 误判成失败并退款。）

**给没被调用过的代码补测（发现「实现了但没跑过」之后做的事）**

起因是一个诊断问题：`task.cpp` 里 `make_inference_client` / `run_inference_task` 是不是没人用？
查下来确实没有调用方 —— 唯一的调用者是三个接缝，而接缝按计划还是 501（同名的
`seedance2_client::run_inference_task` 是**另一个**东西，它在用，容易看错）。
于是这两个函数当时的状态是「编译链接全绿，一次都没运行过」，连带三件事没人验：
①「插入即终态 + 扣费同事务」到底对不对；②失败任务会不会误扣费；③取不到 usage 时写进去的是哪个数。

修法是**用假客户端把它们真跑一遍**，而不是「读一遍觉得没问题」。`ai_client_base` 有 6 个纯虚函数
（`seedance2_client` 是 `final`，不能继承），除 `run_task` 之外都空实现即可 ——
不需要网络、不需要真 key，就把整条结算管线跑起来了：

| 用例 | 钉住的不变式 |
|---|---|
| `succeeded_inference_inserts_a_final_row_and_bills_total_tokens` | 成功行落库为终态（`succeeded` / `task_id_` / `result_text_` / `ended_at_`）、**按 `total_tokens` 而不是调用方预扣值**结算、扣费正好等于该值、行**不在**管理器的 `preparing/queued/running` 集合里（方案 A 的前提） |
| `failed_inference_is_recorded_but_not_billed` | 失败也落库（`data_response_` 保留原始错误便于对账），但 `completion_tokens_` 记 0、余额**一分不动** |
| `missing_usage_falls_back_to_the_precharge_not_the_generation_default` | 响应没带 usage 时退回调用方给的预扣值，**不是** `task.completion_tokens_` 的默认值 50 万（坑 5 的回归） |
| `transfer_station_backend_fails_before_touching_the_network` | 用**真** `transfer_station_client`：它在取 IP / 发请求之前就拒绝推理，于是这条同样没跑过的分支也被覆盖，且落库失败、不计费 |
| `make_inference_client_picks_the_backend_and_carries_credentials` | 后端选择正确（`dynamic_cast` 到具体类）、凭据按后端取、**每次新建**（坑 6 的回归）、签名密钥「有 `kitsu_ctx_t` 才带」 |
| `without_an_ssl_context_the_factory_reports_instead_of_crashing` | 没有 `ctx_ptr` 时抛 `doodle_error`，而不是崩在 `SSL_new` 里（坑 12） |

写这份测试的过程本身又挖出**四个真问题**（下面的坑 9–12）：三个是「只有库外调用方才会暴露」的，
一个是「换个宿主就崩」。这就是「补测」与「补注释」的区别 —— 注释不会让 LNK2019 出现。

**本轮的坑（12 条）**：

1. **Responses 的 `input_image.image_url` 是字符串，生成 API 才是 `{"url": ...}` 对象。**
   两个形状长得像但差一层，共用一个只认对象的遍历器会在 Responses 体上抛 `type_error`，
   而这类错会一路冒成 500。`ai_client_base::for_each_media_url` 同时吃两种形状
   （`content[].{image_url,video_url,audio_url}.url` 与 `input[].content[].image_url`），
   并把「补前缀 + 签名」收敛到唯一的 `to_public_url`。
   顺带修掉 `transfer_station_client` 的 `images[]` / `mask`：它过去对元素**无条件**加前缀。
2. **`thinking` 打开时 `output[0]` 是 `reasoning` 项，不是答案。** 取 `output[0]` 会拿到思维链，
   更常见的表现是「拿不到文本、json 解析失败」。必须遍历 `output[]` 找 `type == "message"`，
   再拼其中 `type == "output_text"` 的 `text`。这一点有专门用例（`output[0]` 是 reasoning）。
3. **`store` 的缺省是 `true`** —— 不显式关掉就会把提示词与素材留在方舟侧，与 §8.5 的
   「离线可复现」冲突。我们固定 `store: false`；代价是**不能用 `previous_response_id` 做多轮**
   （它要求服务端留存历史），所以「改一版」这件事只能靠**我们的**轮次语义传递：
   `spec_generation_request_t.previous_spec_` 本来就是显式输入，比服务端隐式历史更可控。
   另外 `instructions` 与 `caching` 是互斥组合（同时发会 400），所以从不发 `caching`。
4. **`text.format` 的形状在文档里自相矛盾**（平铺 `{type,name,schema,strict}` vs Python SDK 那种
   chat 嵌套变体）。默认发**平铺**形状；若真机返回 400，再降级成 SDK 形状 —— 这条列在 §15.2 的待实测里。
5. **`completion_tokens_` 的默认值是 50 万**（`g_max_task_completion_tokens`，给异步视频预留的上限）。
   推理任务按 **`total_tokens`** 计费（**输入**才是成本大头：多图 + 长提示词），
   必须显式赋值，否则配额与每日统计立刻失真；失败记 0 但**保留原始响应**便于对账。
   `default_consumed_tokens(task_type::inference)` 给的是「按标准提交路径」时的预扣上限（5 万），
   与生成类任务的 50 万区分开。
6. **每次推理都新建客户端，重试时重新取 IP 并重新签名。** `http_client_ssl` 内部只有一个
   `socket_` 且**没有锁**（`read_and_write` 是「写完立刻读整包」的一次性组合），共享给并发请求
   会串包 —— 所以 `make_inference_client` 每次调用都 `make_shared`，不复用 `client_factory` 的缓存。
   重试路径上必须重新签名：默认 TTL 只有 10 分钟，指数退避三次后旧签名可能已经过期，
   而「签名过期」会表现成一个与真实故障无关的 401/403。`operation_aborted`（会话取消）不重试。

**保留的边界**：不做 SSE 流式（仓库的 `http_client_ssl` 是一次性组合，读不到增量）、不做
tools/function calling、不做方舟 Files API 上传（`file_id` 只能作为输入引用）、不做本地 JSON Schema
校验（vcpkg 里只有 nlohmann-json）、不升 `user_version`（非 master 分支，见 §15.4 坑 3）。
`exp/uid/sig` 会进服务端日志与 `data_request_` 的往返，这是接受的风险（10 分钟 TTL + 路径绑定），
38192 也没有 TLS —— 上线前若要收敛，应换成一跳内的对象存储直传或带鉴权的代理。

7. **`NLOHMANN_JSON_SERIALIZE_ENUM` 生成的 `from_json` 认不出字符串时**不抛**，而是静默落到第一个枚举值。**

   这是本轮最值得记住的一条，因为它直接改变了「加一个枚举值」这件事的风险评估。
   `nlohmann/detail/macro_scope.hpp:247`：

   ```cpp
   e = ((it != std::end(m)) ? it : std::begin(m))->first;   // 不匹配 -> 第一个枚举值
   ```

   于是两件事同时成立，而之前只想到了第一件：

   | 路径 | 老服务端遇到新值 / 拼错的值 | 后果 |
   |---|---|---|
   | 数据库（`sqlite_statement.h` 的 magic_enum 反查） | 抛 | 至少不是静默错 |
   | JSON（`SERIALIZE_ENUM`） | **静默变成 `task_type::picture`** | 一个 `"type": "infer"` 的拼写错误会安静地生成一张图片 |

   修法不是「小心别拼错」，而是在**解析入口**把回落堵住：`task.h` 新增
   `checked_enum_from_json<Enum>(json, 字段名)`，用 `magic_enum::enum_cast` 校验后再赋值，
   认不出就抛 `type_error`（与 `get_to` 失败同类）。`task::from_json` 的 `type` / `backend`
   都改走它，测试 `unknown_type_is_rejected_instead_of_becoming_picture` 钉住「不会变成 picture」。

   顺带确认了一个之前只在脑子里推过的事实：`checked_enum_from_json` 用 magic_enum 校验，
   等于**在运行期强制**「JSON 名 == magic_enum 名」这条约定 —— 两条路径的名字从此不可能悄悄分叉。

   **但老二进制里没有这道闸**，所以「先升服务端、再产生 `inference` 行」这条顺序仍然必须遵守，
   而且理由要改成：老服务端收到 `"type": "inference"` 不会报错，它会把它当成一个**图片任务**。
   这类「不报错的错」比抛异常危险得多。

8. **`nlohmann::json{值}` 是列表初始化，会静默包成单元素数组 —— 本轮在测试里又踩了一次。**
   §15.4 坑 1 记的是同一个坑（当时是 `std::optional`）。这次的表现是
   `nlohmann::json(枚举).get<std::string>()` 与 `nlohmann::json{枚举}.get<std::string>()`
   **一个对一个抛 302**；`nlohmann::json{task}` 同理（`at()` 直接报 "cannot use at() with array"）。
   在测试里它尤其阴：断言自己写错了，看起来却像被测代码有问题。
   规矩很简单：**要转换就用括号 `nlohmann::json(x)`，要构造对象/数组就用成对的
   `{{"k", v}}` / `nlohmann::json::array({...})`**，不要用单个值的花括号。

9. **定义写在匿名命名空间里 = 内部链接：编译链接全绿，但动态库里根本不导出。**
   `task.cpp` 的结构是 `namespace doodle::http::seedance2 { namespace { …client_factory… } … }`，
   而 `make_inference_client` / `run_inference_task` 当初被写在了那个匿名命名空间**里面**。
   于是它们是 `doodle::http::seedance2::(anonymous)::xxx`，与头文件里声明的同名函数是**两个不同的实体**：
   库内不受影响（没人调用），也**不会报错**，直到测试（库外目标）引用时才以
   `LNK2019: unresolved external symbol` 暴露，而 MSVC 的提示里还带着 `dllimport` 字样，
   很容易被误读成「忘了 `DOODLELIB_API`」（先加了宏，错误照旧，才找到真因）。
   两个可复用的判断：
   - 「同名的成员函数在别处正常工作」会把人带偏 —— `seedance2_client::run_inference_task` 一直在用，
     与这个自由函数毫无关系；
   - **只要一个函数从未被库外代码引用过，就无法证明它有外部链接**。给它补一个调用方（哪怕只是测试）
     是唯一能自动发现这件事的办法 —— 这正是坑 9–12 的共同来源。

10. **`#include` 写在 `namespace` 里会把被包含头的命名空间整个嵌套进去。**
    搬声明时第一版把 `#include ".../inference_task.h"` 留在了 `reg.h` 的命名空间块内，
    那份头里的 `namespace doodle::http::seedance2` 就变成了
    `doodle::http::seedance2::doodle::http::seedance2`，于是 `task_status` / `task_backend`
    全部「不是 `doodle::http::seedance2` 的成员」，而报错指向的是 `reg.h` 打开的命名空间那一行 ——
    看起来像 `task.cpp` 自己的问题。规矩：**头文件只在文件顶部、命名空间外包含**。
    顺带的设计改进：入口声明从「路由注册头」里挪到 `inference_task.h`，
    测试就不必为了两个函数把整套路由类拉进来。

11. **`set_logger(nullptr)` 是一个必崩的输入。** `g_logger_ctrl().get_http()` 的实现就是
    `spdlog::get("http")`，在任何没建过 http logger 的宿主里返回 `nullptr`（单测宿主、只初始化了一半的
    worker），而 `transfer_station_client::run_task` 的推理拒绝分支第一行就是 `logger_->error(...)`
    —— 崩在地址 `0x8`（`shared_ptr` 内部的空指针解引用）。修法在 `ai_client_base::set_logger`：
    空就回落到 `spdlog::default_logger()`。这条不是「测试环境的问题」：任何 logger 未就绪的部署
    都会以同样的方式炸，而崩一个 `0x8` 完全看不出跟日志有关。

12. **`core_set::get_set().ctx_ptr` 只有主服务器初始化才有。** 它由
    `launch/kitsu_supplement.cpp` 赋值（`tlsv12_client`），worker / 单测宿主里是空的。
    `make_inference_client` 原先直接 `*ctx_ptr`：空指针解引用退化成一个**空引用**交给
    `ssl::context`，最终在 `SSL_new` 读 `SSL_CTX*` 时崩在 `0x8` —— 与坑 11 同一个地址、
    完全相同的误导性。现在用 `DOODLE_CHICK` 明确报错。这条对下一步的 worker 端
    `run_ai_light_distributed` 是**必需**的：worker 迟早要在没有服务器初始化的情况下建客户端，
    那时它应该得到一句「ssl context 未初始化」，而不是一个 `0x8`。


