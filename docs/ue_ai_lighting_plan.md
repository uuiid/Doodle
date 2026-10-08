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
- 浏览器前端是**唯一交互面**，也顺带解决了「浏览器看不到 UE 视口」这个约束——因为不再需要实时视口，美术看的是渲染出来的成片（每候选一段几秒的视频 + 首帧海报）
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
│ ④ UE 无头执行器（工作机）                                                  │
│  （批 9 修订）无 `-run=`：UnrealEditor-Cmd <uproject> <render_map>          │
│                 -DoodleAiLightParams=<json>（进程内由插件模块的            │
│                 UDoodleLightingRoundRunner 驱动 MRQ + PIE 执行器，D45/D46）│
│  · 关卡来自载荷 render_map，编辑器打开它跑 PIE（资产永不落盘）              │
│  · 一次启动，逐候选：把 rig 建进 PIE 世界 → MRQ 出 PNG 帧序列              │
│  · 工作机把帧合成 mp4 + 首帧当海报（EXR 可选，D42）+ 逐张 HTTP 上传（D40）  │
└────────────────────────────┬─────────────────────────────────────────────┘
                             │  回传一份报告（帧率 + 每候选状态/帧数）
                             │  + 产物逐张上传 + put_job_info
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
    "feedback_axes": {                 // 结构化反馈：键 = variant_axis_kind 枚举名，值 = 增量
      "key_direction": -15, "kelvin": -600
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

**单图统计（本节的①）与多图聚合（本节的②）都已落地**。逐图统计在
`doodle_lib/http_method/kitsu/ai_light_stats.cpp` 的 `analyze_display_image()` /
`analyze_reference_file()`，随上传写入 `ai_light_reference.stats_` / `stats_confidence_`；
多图聚合在同一个文件的 `aggregate_reference_stats()`，由 `POST /session/{id}/spec` 在蒸馏
Spec 前调用一次（§15.7）。`is_outlier_` / `excluded_reason_` 仍是**人工排除**字段，聚合器
**不写**它们 —— 自动疑点走 `suspected_outlier_ids_`，最终落在 Spec 的
`conflict_resolution_` 里（见 §5.3 与 §15.7 的决定 D3）。

### 5.3 多图聚合与冲突检测

1. 逐图提取统计量
2. 逐维度取中位数（或按面积/清晰度加权）
3. **冲突检测**：维度内离散度超阈值 → 标记 `conflicts`，**不自动平均掉**
4. 冲突维度进入人工闸门 1 裁决，或降级为「该维度不设目标」

**已落地**（`ai_light_stats.cpp` 的 `aggregate_reference_stats()`，返回 `aggregated_reference_stats_t`）：

| 步 | 落法 |
|---|---|
| 1 逐图提取 | 调用方传进来的 `ai_light_reference::stats_`；**人工排除的图不参与**（`is_outlier_ == true` 直接跳过），也不发给模型 |
| 2 逐维度取中位数 | 取**中位数**而不是均值（§5.3 原文的「或按面积/清晰度加权」没做：参考图是同一场景的不同角度，没有哪张更权威）；同时留下极差区间作为目标区间 |
| 3 冲突检测 | 逐维算 `dimension_spread_`（P90−P10 的相对离散度），超 `g_conflict_spread_threshold` 的维度记进 `stats_.conflicts_`，**不自动平均掉** |
| 附录 | 逐维置信度 `per_dimension_confidence_`（由各图 `stats_confidence_` 与离散度共同压低）；疑似外点 `suspected_outlier_ids_`（维度距离 > `g_outlier_score_threshold = 2.5`） |

两处与本节原文的口径差：外点阈值计划里写的是 `d > 3.0`，**实现定在 2.5**（参考图数量少，
3.0 在 5~8 张样本上几乎抓不到偏得明显的图，而抓错只是「标记为疑点」而非剔除，代价不对等）；
冲突维度**没有**降级为「不设目标」的实现 —— 聚合器照常给出区间，是否采纳由模型与人工闸门 1 决定，
这也是「聚合做事实、模型做取舍」的分工。

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

**实现约束（§15.8）**：`variant_axis_t` 只带**一条**轴（`axis_` + `value_` + `label_`），所以候选展开必须是
**OFAT**（候选 0 = 基线，其余 5 个各沿一条轴变化一档），**不是**全因子组合。上表「每轴 2–3 档」在实现里的
落法是**两种自由度的组合**：模型选 1–2 条轴（`g_axis_max_count`），**档位总数**固定为
`g_axis_step_budget = g_candidate_count - 1 = 5`，由 `parse_axis_plan()` 硬校验；校验不过则退回
`default_axis_plan(spec)` 的固定轴表。

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

> **实现现状（§15.8 决定 D7）**：`embedding` 列已建好，但全仓库**没有可用的查询向量生成器** —— 唯一的
> embedding 生成器是 ONNX 的 `src/doodle_lib/ai/llm2vec.{h,cpp}`（被 `src/doodle_lib/ai/kimodo.cpp` 用），
> 需要 ONNX Runtime + 模型文件，且与预设的 CLIP 图编码不是同一个空间。所以本轮 `pick_presets()` 走
> **词法匹配**（`preset_lexical_score()`：tags/name 与 Spec 的 `intent` 文本做子串打分），取
> top-`g_preset_prompt_top_k`(12) **只送进 `input_text` 当知识注入**，不参与候选展开。向量检索留到有编码器时再接。

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

> **实现现状（§15.8 / §15.9）**：这张表里的 7 个「工具」**全部由我们的代码调用，模型从不调用任何工具**
> （线格式里根本不发 `tools`，§15.6）—— 真正调用 LLM 的是 `build_spec`（§15.7）、`generate_candidates`
> （§15.8）与 `parse_feedback`（即 `structure_feedback`，§15.9），另外四个（`inventory_lights` /
> `query_scene` / `list_presets` / `commit`）是 worker 侧或数据库侧的纯本地操作。因此上表
> `generate_candidates(spec, axes, n)` 里的 `axes` 在**有上一轮人工反馈时是输入**，在**首轮 / Spec 升版后是
> 输出**（接缝自己排轴，见 §15.8 决定 D6）；而 `parse_feedback(text)` 产出的也是 `axes`（单值方向），
> 它经 `/judge` 写进 `feedback_axes_` 后，下一轮就变成 `generate_candidates` 的**输入**（§15.9）。

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
UnrealEditor-Cmd.exe <uproject> <render_map> -windowed -log -Unattended \
    -DoodleAiLightParams=<json>
```

> **（批 9 修订）**上面这条命令行是本节的原始设计：`-run=DoodleLighting` 的 commandlet 形态**已删除**
> （候选产物改视频后，commandlet 里 MRQ 一帧都渲不出 —— 见 §15.16 的两条引擎事实），现在是无 `-run=` 的
> 编辑器启动 + 插件模块 `UDoodleLightingRoundRunner` 驱动 MRQ/PIE 执行器。本节余下的「无状态、不碰源资产」
> 「命令行列参数」等结论仍然成立，详见 §15.17。

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
- `DoodleLightingEditor`（Editor）：**执行器 + 一轮渲染的驱动者**（无面板；批 9 起不再有 commandlet，见 §15.17）

> 命令类模块必须是 Editor 类型（`UCommandlet` 需要），所以 `DoodleLightingEditor` 仍然存在，但内容比 v3.1 少得多。
>
> **落点状态（2026-08 实施）**：两个模块**已落地**，共 20 个文件，见 §15.13（`DoodleLighting` 11 个文件 =
> IR/JSON/报告/纯换算；`DoodleLightingEditor` 9 个文件 = 执行器/一轮驱动者/冻结后处理基线）。UE 侧代码未在本机编译过。
> **批 9（见 §15.17）**：旧的 SceneCapture + `UDoodleLightingCommandlet` 路径已删除（编辑器模块 11 → 9 个文件）。

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

**本轮落地范围（§15.8）**：第一条（沿轴生成差异化候选）已由 `expand_candidates()` 确定性完成；
**第二条「灵敏度标定」本轮不做** —— 它需要渲染回环（渲染 → 测落点 → 估灵敏度），属于 M2 之后。
现在「候选在轴上确实不同」靠 `map_candidate_result()` 的**抛错**而非标定：任何一档轴经
`apply_axis_step()` 后 rig 实质无变化，接缝直接 502。宁可这一轮失败，也不要发 6 张看起来一样的图
去浪费美术的一次判断。

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
  <root>/ai_light/<会话 uuid[0:3]>/<会话 uuid[3:6]>/<会话 uuid>/<轮次 uuid>/<候选 uuid>.png
  <root>/ai_light/<会话 uuid[0:3]>/<会话 uuid[3:6]>/<会话 uuid>/<轮次 uuid>/<候选 uuid>.exr
  <root>/ai_light/backup/<YYYY_MM_DD_HH>/<会话 uuid>/          ← 删会话时整体搬到这儿
  ```

  > 轮次目录名就是**轮次自己的 uuid**（不用 `it_001` 那种序号）：候选行自带 `session_id_` 与
  > `iteration_id_`，于是「候选 → 产物路径」是一次纯计算，不必回查轮次行去拿那个可变的
  > `iteration_` 序号（见 §15.15 的 D44）。

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
  全部靠查询得出。定稿是**可移动的单指针**（再次定稿会先解除上一次，旧 `preview_file` 不删，
  见 §15.10），每会话至多一个定稿由 `accepted_session_id` 上的普通唯一索引在库层强制
  （定稿才填 session_id、否则为 NULL；SQLite 的唯一索引把 NULL 视为互不相等，于是等价于
  `WHERE is_accepted = 1` 的 partial index —— 为什么不能直接用 partial index 见 §15.3）。
- 「等人工判断」是**会话/轮次状态**，不进 `server_task_info_status`

**`preview_file` 的复用边界**：

| 产物 | 进 `preview_file`？ | 取图方式 |
|---|---|---|
| 参考图（多张） | 否 | `/api/doodle/ai/light/reference/{uuid}/image` |
| 中间候选渲染（6 × N 轮） | 否 | 首帧海报 `/api/doodle/ai/light/candidate/{uuid}/image` + 成片 `/api/doodle/ai/light/candidate/{uuid}/movie` |
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

> **M1 进度（2026-08）**：设计链路的**服务端段已完成** —— 提交/参考图、`generate_spec`（§15.7）、
> `generate_candidates`（§15.8）、派发（§15.12）、回传落库（§15.12）、UE 侧两个插件模块（§15.13，
> 未编译）都已落地。**未完成的 M1 剩余项**：浏览器页面本身（§14 + `docs\ue_ai_lighting_frontend.md`，
> 不在本项目实现）与真机一轮回环；另外候选的 `metrics_` 复算仍缺（§15.12 末尾），M4 的区间满足率依赖它。

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
> **已可观测（§15.7 / §15.8）**：`generate_spec` 已落地，它按聚合器的结果判断并把 `degrade_level_` 落库，
> 于是这条现在是真正能查的字段；`generate_candidates` 也已落地。**多图聚合（§5.3 的②）同样已落地**
> （`aggregate_reference_stats()`），所以 `stats_only` 的「聚合可用」语义现在是完整的。

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

   **接缝进度**：三条接缝**全部接上** —— `generate_spec`（§15.7）、`generate_candidates`（§15.8）、
   `structure_feedback`（§15.9）。**仓库里已经没有 `not_implemented` 的端点。**
   接法就是 `make_inference_client()` + `run_inference_task()`，参数与返回值早已定好。

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
   | `POST /api/doodle/ai/light/session/{session_id}/spec` | 生成 Spec：`draft` 出第一版，`awaiting_spec_confirm` 重出（**要求已有参考图**，否则 400）—— **已实现**（§15.7）：聚合统计 + 一次方舟推理；失败时会话状态回滚、Spec 版本不变 |
   | `POST /api/doodle/ai/light/session/{session_id}/spec/confirm` | 人工闸门 1：确认 Spec + 冲突裁决，随后触发首轮 |
   | `POST /api/doodle/ai/light/session/{session_id}/iterate` | 生成一轮候选并派发 —— **已实现**（接缝 `generate_candidates`，见 §15.8）；无需请求体 |
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

   **端点不是占位桩**：权限、状态校验、会话查找全部真实执行，只有最里层的 LLM 接缝曾是
   `not_implemented`（→ HTTP 501，见 §15.4）。这条约束当初的意义是「参数错了明确报错、参数对了告诉你
   能力还没到」，而不是一个假装成功的 200。**三条接缝现已全部落地**（§15.7 / §15.8 / §15.9），
   仓库里不再有 501 端点。`/judge` 的滑块通道（`feedback_axes`）**从一开始就可用**：
   文本结构化是补充通道，接缝失败时降级为「文本原样留存 + 只用滑块轴」，判断本身完全成立。
6. **轮次编排**：`awaiting_judgement` 会话状态、4 小时过期标志、跨会话恢复（§10.3）。
   **已落地**：`doodle_lib/http_method/kitsu/ai_light_orchestrator.{h,cpp}`
   （`advance_session` / `apply_judgement` / `mark_iteration_rendered` / `build_session_detail`；
   三条接缝与校验层已拆成独立文件，见 §15.11）。
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
   算法口径见 §5.2，色温的显示空间例外见 §8.2。**多图聚合与冲突检测（§5.3 的②）已落地** ——
   `aggregate_reference_stats()`（`http_method/kitsu/ai_light_stats.cpp`），随 §15.7 的 `generate_spec` 一起接上。
9. **反馈结构化**：`/judge` 收到的自然语言反馈经 LLM 转成 `feedback_axes`。
   **已落地**（§15.9）：`structure_feedback(sqlite_database&, const feedback_structuring_request_t&)`；
   只在 `decision_ == continue_iteration` 时调用，滑块已给的轴**不覆盖**（决策 5：滑块是默认通道），
   接缝抛任何 `doodle_error` 都降级为纯滑块反馈（文本原样留存）。

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

> **实施偏差（2026-08，见 §15.12 / §15.13）**：
> ① `run_task()` 分支已加（`work.cpp:237`），新任务类在 `exe_warp/ai_light_render.{h,cpp}`（未抽公共基类，
>    直接复用 `base_distributed_task` + `async_run_ue` 设施）；
> ② `allowed_task_types` 默认集合**也加了** `ai_light`（`event.cpp:31-38`）—— 与第 2 条相反，是实施时的选择
>    （工作机与主服务器都要显式开，否则默认不派）；
> ③ **回传的只是一份** `render_report.json` 摘要（D26/D27/D30）：产物落**工作机本地**、逐张 HTTP 上传，
>    所以第 5 条的体积问题不存在（见 §15.14 的 D38/D39/D40）。
> ④ UE 命令行必须带 **`-AllowCommandletRendering`**（否则 `CommandletHelpers::TickEngine` 不 tick 渲染），
>    见 `exe_warp/ai_light_render.cpp:53-58`。

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

> **实施偏差（2026-08，见 §15.13 —— 本节其余条目仍是设计原文）**：
> ① **不复制基线关卡**（D37）：`LoadObject<UWorld>` 后换 `GWorld`，不 `DuplicateAsset`/不 `SaveLoadedAssets`/不落盘；
> ② **不按 op 增删灯，而是按 `rig.lights_` 全量重建**（D31；`diff_rigs()` 永不产生 add/remove），
>    因此也不去改 `ClearAllLight()` —— 我们只销毁自己建的 actor（美术手工灯一概不碰）；
> ③ **不用 MRQ**（D36）：`USceneCaptureComponent2D` 出 LDR PNG + 线性 EXR，8 次丢弃式预热抓帧；
> ④ 预览机位取载荷 `camera_file_path_` 的交付机位（临时 level sequence 导入 FBX），分辨率 1920×1080 常量；
> ⑤ `metrics` 不由 UE 计算（D30），服务端复算〔**尚未落地**，见 §15.12 末尾〕。

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

> **前端不由本方案实现方实现。** 本章是**需求侧**的描述（前端要负责什么、边界在哪）。
> 交给前端实现方的**施工级交接文档**在 `docs/ue_ai_lighting_frontend.md`：
> 逐页操作逻辑、接口契约（19 条路由的请求/响应/错误）、六条轴的滑块规格、指标看板呈现规格、
> 通知与轮询、验收清单。**两份文档冲突时以交接文档为准**（它的字段与状态码全部取自已落地代码）。

### 14.1 现状：它是什么

- 前端是 **Kitsu 的 fork**（cgwire 开源动画协作平台），Vue 3 + Pinia + vue-router + Bulma + Vite。
- 由主服务器作为**静态资源**托管：`http_method/kitsu/kitsu_front_end.cpp` 从 `root_path_`（= `kitsu_ctx_t::kitsu_front_end_path_`，生产 `D:/kitsu/dist`）读文件，非 `/api` 路径回落到 `index.html`（SPA 兜底）。注册在 `create_kitsu_route_2` 的**最后一行**（`kitsu.cpp:425`）。
- 已经有一层很厚的 Doodle 扩展（构建产物里 `/api/doodle/*` 出现 71 处）：`ai_image`、`model_library/*`、`pictures/*`、`task/*`（含 log / inspect / restart）、`computing_time/*`、`attendance/*`、`deepseek/key`、`key/ji_meng`、`tool/version`。
- **已经会提交 UE 装配任务**：`/api/actions/projects/{project_id}/shots/{id}/run-ue-assembly`。
- **已经在浏览器里直接调 LLM**：产物里打包了 OpenAI JS SDK（`OpenAI` 出现 68 处），key 从 `/api/doodle/deepseek/key` 取（对应 `http_method/other/other.cpp:21`）。

### 14.2 决策 2/3/4 带来的变化

取消 UE 实时交互后，前端从「方案与评审」升级为**唯一交互面**，而且**不再受「看不到 UE 视口」的约束**——因为不需要实时视口，美术看的是渲染出来的成片（每候选一段视频 + 首帧海报），这正是 Kitsu 已经擅长的事（preview files、对比、评论、状态流转）。

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

> 这 5 个页面的**施工规格**（每页的字段、操作顺序、状态与按钮可见性对照表、错误码处理、验收清单）
> 见 `docs/ue_ai_lighting_frontend.md`；本章只定「有哪些页面、各干什么」。该文档另记录了
> 三个必须在开工前定下来的问题：落在哪个前端工程、是否复用 Kitsu 的 preview file/评论组件、滑块的最终档位。

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
   - **`thinking: disabled`** 是否被接受（有的模型只允许 `enabled`/`auto`）；
   - **`thinking` 关闭时的蒸馏质量**：结构化任务**不发** `thinking`（§15.6 坑 2 的取答案路径
     要求 `output[0]` 不是 reasoning 项）。代价是少了思维链，`intent` 可能偏浅 —— 若实测明显变差，
     退路是改成 `thinking: auto`；解析端已按「遍历 `output[]` 找 `message`」写好，不用改。
   - **全字段 `required` 的严格 schema 是否被照做**：`build_spec_json_schema()` 每层都要求列全
     所有字段（严格结构化输出的要求），于是「本维度不设目标」**不能靠省略字段表达**，只能写进
     `conflicts`（§15.7）。真机上要看模型是否照做。
     `evidence` 曾经是 `additionalProperties: <条目 schema>` 的自由键字典，**已改成 17 个固定键 +
     全 required**（§15.7 坑 19）—— 严格结构化输出不接受自由字典，这一条不必再实测。
   - **不设 `max_output_tokens` 之后的实际输出长度**（§15.7 坑 21）：模型按自己的上限出。若真的被截断，
     `result_json_` 会是 null → 接缝抛 502，而它与「`text.format` 形状被拒」的表现**无法区分** ——
     真机上若见 502，先去 `data_response_` 看 `status` 是不是 `incomplete`。
   - **`input` 项是否必须显式带 `type:"message"`**（§15.7 坑 20）：文档写 InputItem 的 `type` 取值固定
     `"message"`，但没写「省略会怎样」。我们改为**显式发**，这一条的实测价值只在于「确认加了没被拒」。
5. **FK 前向引用已实测通过（无需再验）**：`seedance2_task_2` 注册在 `ai_light_iteration` **之前**，
   新加的 `ai_light_iteration_id` 外键在 CREATE TABLE 时指向了尚未注册的表 —— SQLite 允许这样做，
   实测建表/重建/外键语义用例（`sqlite_schema` / `sqlite_rebuild` / `sqlite_fk_*` / `ai_light_schema`）
   全绿。这一条列在这里是为了**关掉**它，不必再怀疑。
6. **候选生成的实测项**（§15.8，同样只能拿真 key 打一次才知道）：
   - **模型是否真的会「排轴」**：`build_candidates_json_schema()` 要求它给出 1–2 条轴 + 档位合计 5，这是
     把「探索方向」交给模型的唯一地方。若真机上它总给 1 轴 5 档、或干脆给 0 档，说明自由度太窄 ——
     退路是回到 `default_axis_plan()` 的固定轴表（现在已经是回退路径，只需把「回退」从异常降级为常态）。
   - **`axes[].steps[]` 的档位是否会重复**：`parse_axis_plan()` 只查**档位总数**不查去重，
     `expand_candidates()` 也不去重 → 模型给 `[0,0,0,0,0]` 时会生成 5 个一模一样的候选。
     `map_candidate_result()` 的「轴无实质变化即 502」能兜住（5 个都退化成基线），真机上若频繁触发 502，
     就该在 `parse_axis_plan()` 里加去重校验。
   - **预设词法匹配的质量**（§15.8 决定 D7）：中文没有词边界，`preset_lexical_score()` 是子串包含，
     预设库词条多起来以后误命中会上升。真机上看 `input_text` 里命中的预设名是否合理。
7. **反馈结构化的实测项**（§15.9，同样只能拿真 key 打一次才知道）：
   - **模型是否真的把中文反馈翻成轴方向**：轴名必须是 `variant_axis_kind` 的枚举名（英文），
     而美术写的是「主光再低一点、冷一点」。若真机上它频繁给未知轴名（会被丢弃）或干脆给空 `axes`，
     说明提示词里的「轴 → 语义」对照还不够 —— 退路是让滑块成为前端的**主要**入口、文本只当备注。
   - **`key_fill_ratio` 的绝对/增量语义会不会被搞混**：六条轴里只有它是**绝对比值**，其余五条都是增量
     （提示词里已写「须参考【当前 key:fill 比】」）。真机上看它给的是不是「当前比值附近的一个数」而不是
     「一个 ±0.3 的增量」—— 后者会被 D10 直接丢弃（非正数）或产出荒谬的补光强度。
   - **`axes` 会不会超过 2 条**：这是形状违规 → 接缝抛 502（D14 会降级成纯滑块，但美术的文本就白写了）。
     `maxItems: 2` 在严格结构化输出里应被强制，真机上确认它是「被拒」而不是「照发 3 条」。
   - **`understanding` 的质量**（只进日志、不落库）：它是唯一能看出「模型到底读懂了什么」的窗口。
     真机上值得看一眼，再决定要不要把它落库 —— 现在不给它列，是因为它是不可校验的自由文本。

### 15.3 已落地（数据库层）

| 项 | 落点 |
|---|---|
| 六张表的结构体 | `src/doodle_core/metadata/ai_light/`（`common.h` / `rig.h` / `spec.h` / `session.h` / `reference.h` / `iteration.h` / `candidate.h` / `preset.h` / `ai_light.h`） |
| ORM 注册 | `sqlite_database.cpp` 的 6 个 `reg_table` 块（含定稿唯一索引） |
| 迁移 30 → 31 | `sqlite_upgrade.cpp` 的 `upgrade_3_t` + `upgrade_3()`（门控常量 `g_version_30`）。**生产库停在 30，而这是 30 之后的唯一步骤**：29 → 30（`upgrade_2_t`）已随该步骤进入生产而移除，31 → 32 的终态改写也已撤销（见 §15.10），所以链条终点是 31、升级链只认 30 及以上的库 |
| 查询方法 | `sqlite_database.h/.cpp` 的 19 个 `get_ai_light_*` |
| 产物路径 | `doodle_core/metadata/kitsu_ctx_t.h` 的 `get_ai_light_session_dir` / `_reference_dir` / `_reference_file` / `_iteration_dir` / `_candidate_file` / `_backup_dir`（**唯一**布局真相来源；原先在 `core/entity_path.h/.cpp` 的四个 `get_ai_light_*_path` 已删除，见坑 6） |
| 任务类型 | `server_task_info_type::ai_light` |
| 测试 | `src/test/core/ai_light_schema.cpp`（建表、版本门控、JSON 往返无损、定稿唯一性、过期时限经 TEXT 列往返后仍判得对） |

**实现期发现、值得记住的三个坑**：

1. **ORM 的列类型不覆盖嵌套结构体与 `std::map`** —— 只有 `nlohmann::json` 可以。所以 `rig` / `metrics` / `intent` / `targets` / `judgement` 全部以 JSON 列落库，
   靠 `get_xxx()` / `set_xxx()` 做类型化访问。这不是偷懒：直接把 `lighting_rig_t` 当列成员会编译不过。
2. **`user_version` 与「每一步各写自己的目标版本」必须配套** —— 当时 `upgrade_2_t`（29 → 30）若仍写
   `g_current_version`，29 版库会被直接标成 31，`upgrade_3_t` 的 `> 30` 门控随即跳过建表，而且**版本号上
   看不出任何异常**。`src/test/core/sqlite_upgrade.cpp` 里专门有一组用例盯住这种「静默跳过」。
   规则是「**只有最后一步和 `upgrade_init_t` 写 `g_current_version`**」，判据只有一个：这一步是不是最后一步。
   `upgrade_2_t` 现已随该步骤进入生产而移除（同一取舍的先例：27 → 28、28 → 29），版本化步骤只剩 30 → 31
   一步，所以它写 `g_current_version` 是正确的 —— **将来在它前后再插步骤时必须回头检查这一行**。
   代价是刻意的：**升级链只认 30 及以上**，停在 30 之前的库（例如开发机上的旧备份）会拿到 AI 灯光六张表
   但缺 `entity_asset_extend_2.te_xie` 列，读写那张表会报 `no such column`，需要重建或手工补列。
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
| `/judge` accept（定稿，**可重复**） | `awaiting_judgement` | `generating`（**不是终态**，见下方说明） |
| `/judge` continue_iteration | `awaiting_judgement` | `generating` |
| `/judge` reject_all | `awaiting_judgement` | `generating`（**不是终态**，见下方说明） |

**三个判断都回到 `generating`，因为会话没有终态**（§15.10）：美术对同一个镜头的灯光可能是
**无限轮**的调整 —— 今天定稿，明天导演改意见，回来接着调。定稿因此不再是终点，而是把
「当前定稿」这个指针移到这一张（`accepted_session_id` 上的唯一索引仍然保证每会话至多一个，
再次定稿会先解除上一次；旧 `preview_file` 不删，它是已经交付过的图）。`reject_all` 也不再终止，
只表示「这一轮全部不要」。一轮只能判断一次（`apply_judgement` 的 `iteration.state_ != judged` 闸门），
所以「再来一轮」必须经 `POST /iterate`。

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

**接缝**（三条都原样保留，其中两条已落地）：

```cpp
// 三条接缝各在自己的文件里（ai_light_seam_{spec,candidates,feedback}.{h,cpp}，见 §15.11）；
// ai_light_orchestrator.{h,cpp} 只留状态机（伞形头 + 四个状态机函数）。
generate_spec(in_sql, req)           -> spec_generation_result_t      // 已落地，见 §15.7
generate_candidates(in_sql, req)     -> candidate_generation_result_t // 已落地，见 §15.8
structure_feedback(in_sql, req)      -> std::vector<axis_step_t>      // 已落地，见 §15.9
```

**接缝契约（D3）**：接缝抛异常 ⇒ `advance_session` 把会话状态**回滚到入口状态**并记录
`failure_reason_`，然后原样重抛。失败**没有独立状态**（§15.10 起 `failed` 已取消）：
`failure_reason_` 是失败的唯一记录，会话永远停在「可以重试」的那个状态上。
理由是：半成品比失败更糟 —— 一个「有 Spec 但没有候选」的会话会让前端以为可以判断了。
接缝实现者不得留下非幂等的部分写入。`/spec/confirm` 的 Spec 确认**不随回滚撤销**：
确认是人工动作，不该因为下游能力缺失而被撤销。

**其它本轮定下的取舍**：

1. **`/judge` 的滑块通道独立可用**。`feedback_axes` 直接从请求体取（决策 11：滑块为默认），
   `structure_feedback` 只是补充通道；捕获到任何 `doodle_error` 就降级为「文本原样留存 + 只用滑块轴」，
   判断本身完全成立。所以「缺 LLM」不会让闸门 2 不可用。
2. **定稿的三处写入必须同事务**。`preview_file` 行、候选的定稿标记与 `preview_file_id_`、
   会话状态分两个事务提交就会出现「预览文件已建但候选没标定稿」的半成品。
   所以调用方（HTTP 层）负责碰文件（OpenCV 读图 + `save_variants` 落变体），
   `apply_judgement` 负责一次事务落库 —— `preview_file` 按**值**传入，
   因为 ORM 的 `values()` 会回填 `uuid_id_` / `created_at_` 这类服务端生成字段。
3. **`is_selected_` 与 `is_accepted_` 是两个语义，不合并**。前者专指「被选为下一轮基准」
   （续轮 warm start 的起点），后者指「当前定稿」。会话没有终态之后（§15.10）定稿**也**置
   `is_selected_`：定稿后接着调时，下一轮的 warm start 应该从定稿那套 rig 出发（那是目前最好的一张），
   而不是回到 LLM 的原始基线。两个字段因此可以在同一行上同时为真，但各自的含义没有变。
4. **中间候选与参考图都不进 `preview_file`**（决策 1）。每轮 6 张全注册会把任务的预览列表
   冲垮，也会把「交付物」这个概念稀释掉。中间图走 `/candidate/{id}/image`，
   EXR 一律不下发浏览器（线性数据、体积大、浏览器渲染不了）。
5. **建会话接口不吞任何接缝错误 —— 因为它不调接缝。** 早先建会话会立刻跑第一步，
   所以需要「吞掉 501 以免前端拿不到 session id 而重试建重复会话」这套说法；
   现在建会话只 insert，`POST /spec` / `/iterate` 的接缝错误一律原样返回 ——
   那是能力缺口或真实失败，前端应该看到，而不是被包装成一个「成功但少了点东西」的 2xx。
   （三条接缝现已全部实现，见 §15.7 / §15.8 / §15.9。）
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
| 转签的路由 | `seedance2/pictures`、`kitsu/ai_light` 的 `reference_image_route` / `candidate_image_route`（`/api/doodle/ai/volcano-engine/**` 那四条已按「不再使用」删除，见下） |
| 常量 | `doodle_config::g_ark_inference_model{deepseek-v4-1-flash-260910}` / `_timeout{180s}` / `_max_retry{3}` / `_default_max_output_tokens{4096}` / `_default_consumed_tokens{5万}` |
| 测试 | `src/test/core/url_sign.cpp`（11 用例 / 35 断言）、`seedance2_inference.cpp`（17 用例 / 74 断言）、`seedance2_task_model.cpp`（7 用例 / 33 断言）、`seedance2_task_pipeline.cpp`（6 用例 / 35 断言，见下「给没被调用过的代码补测」）；全量回归 **150 用例 / 1103 断言**通过（本轮之前是 109 / 926） |

**接缝怎么用**（三条接缝都照这个形状接上了，见 §15.7 / §15.8 / §15.9）：

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
// 信封由 build_spec_request() 以**强类型**给出(inference_request_t), 赋给 json 时走它的 to_json;
// 落库键集与手写 json 完全一致
l_task.data_request_          = build_spec_request(请求, 聚合统计);   // 字段见 build_inference_body

const auto l_result = co_await run_inference_task(l_task, l_sql, l_client);
// l_result.status_ / result_text_ / result_json_ / total_tokens_; 任务行已落终态并已扣费
```

`data_request_` 里不要自己拼方舟的 `input[]`/`model`/`store` —— 归一化信封（`input_text` / `images` /
`json_schema` …）由 `build_inference_body()` 翻译，签名与公网前缀由客户端在发请求前补。

归一化信封现在是**强类型结构体** `doodle::http::seedance2::inference_request_t`
（`http_client/seedance2_inference_request.h`）：13 个键名与类型只写这一处，生产端填字段
（`build_spec_request()` 直接返回它），消费端 `build_inference_body()` 按字段读。`data_request_`
列仍然是 `nlohmann::json`（生成类任务共用该列），结构与 json 之间靠 `to_json` / `from_json` 转换 ——
键缺失取默认值，**键存在但类型不符抛 `nlohmann::json::type_error`**（静默丢掉 `images` 等于发出一个
没有参考图的请求）。因此落库形状与改造前一致，旧行不需要迁移；也不要用结构体去重序列化从库里读出来的
`data_request_`（未知键会丢）。

**已删除：`/api/doodle/ai/volcano-engine/**` 下四条素材路由**（本轮确认不再使用后清掉）。
删掉的是「通用素材上传 + 取图」这一套，不是 AI 灯光的参考图通道 —— 后者走
`kitsu/ai_light` 的 `reference_image_route` / `candidate_image_route`，与这些产物路径无关。

| 删除项 | 位置 |
|---|---|
| 四个路由类的声明 | `http_method/kitsu/kitsu_reg_url.h`（`DOODLE_HTTP_SIGNED_FUN` 那四个） |
| 五条注册（video 的 `.mp4` 与 `.png` 两条都指向同一个类） | `http_method/kitsu.cpp` |
| 四个处理器实现 | `http_method/kitsu/pictures.cpp`（video 上传里的「读首帧做缩略图」一并删除） |
| 产物路径 | `kitsu_ctx_t::get_inference_materials_video/_image`（除这四个处理器外无人使用，属同一功能的死代码） |
| OpenAPI 生成物 | `docs/api/openapi.yaml` 的 5 条 path + `docs/api/paths/api_doodle_ai_volcano-engine_*.yaml` 5 个文件 |

删除后 `DOODLE_HTTP_SIGNED_FUN` / `http_jwt_or_signed_fun` 仍有三个使用者
（`seedance2/pictures` 与 `ai_light` 的两条图片路由），所以那套签名鉴权**不动**。
`preview::convert_to_png` 仍被 `ai_light.cpp` 使用，也保留。

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

### 15.7 已落地（接缝 1：`generate_spec` —— 提示词 + 参考图统计 → LightingSpec）

§5 讲的是「提示词 + 多张参考图 → 一份结构化照明目标」该怎么做；§15.5 落地了其中的**感知层**
（①单图统计、②多图聚合），§15.6 落地了**推理通道**。本轮把两者接起来：
`POST /api/doodle/ai/light/session/{id}/spec` 从 501 变成**真的跑一次推理并落库一版 Spec**。

一次调用的数据流：

```
POST /session/{id}/spec
  → ai_light::spec_route::post
      · 读会话与参考图（get_ai_light_references）
      · 操作人 = JWT 里的 person（http_jwt_t::person_.person_.uuid_id_）
      · co_await advance_session(in_sql, l_session, l_operator)
          → 入口校验：会话至少已有一张参考图（决策 1「参考必有多张」）
          → 逐图读**已落库**的 stats_（不重新算图 —— 统计是感知层的活）
          → co_await generate_spec(in_sql, l_req)
              · aggregate_reference_stats(l_req.references_)          // 纯函数，§15.5 的②
              · person / ai_studio 解析（解析不出 → 400，不是 500）
              · sd2::task + make_inference_client + run_inference_task // §15.6：一次同步推理
              · map_spec_result(result_json_, l_agg)                  // 纯函数：统计为底、LLM 为覆盖
          → 同一事务：insert(ai_light_spec) + update(会话 state=generating, current_iteration=0)
          → 失败：整段回滚到入口状态 + 写 failure_reason_，**Spec 版本不变**
```

| 项 | 落点 |
|---|---|
| 接缝本体 | `http_method/kitsu/ai_light_seam_spec.cpp` 的 `generate_spec(in_sql, in_request)` `:565` |
| 离线可测的 5 个纯函数 | 同文件 `build_spec_instructions()` `:120` / `build_spec_json_schema()` `:142` / `build_spec_input_text()` `:227` / `build_spec_request()` `:331`（返回强类型信封 `http::seedance2::inference_request_t`，定义在 `http_client/seedance2_inference_request.h`）/ `map_spec_result()` `:361`（声明在 `ai_light_seam_spec.h`，**必须留在 `doodle::ai_light` 命名空间内**，见坑 9）；服务器自组 `evidence_` 的 `put_evidence()` `:104` |
| 请求 / 结果结构 | `spec_generation_request_t{ session_id_, project_id_, operator_id_, prompt_text_, references_, degrade_level_, previous_spec_ }`；`spec_generation_result_t{ spec_, evidence_, reference_stats_, conflict_resolution_ }` |
| 状态机 | `advance_session(in_sql, in_session, in_operator_id)` `:124`（新签名，见 D1） |
| 参考图 URL | `http_method/kitsu/ai_light.h` 的 `g_reference_image_path_template` + `reference_image_url(uuid)`。放在路由头是因为 `/api/doodle/ai/light/reference/{}/image` 的注册要用**字面量**喂 `_url` 这个 UDL，`constexpr std::string_view` 递不进去 |
| 指令串 | `build_spec_instructions()`：§6.1 职责边界两侧 + §6.3 冲突四条 + **§5.5 六行映射表原文** + 单位口径（对比度 = 显示空间 `p90 − p10`；色温 Kelvin；区间是定长 `[low, high]`）+ 「不许改几何 / 材质」 |
| 输出 schema | `build_spec_json_schema()`：平铺 `{intent, targets, evidence, conflicts}`，逐层 `additionalProperties:false` 且**全字段 required**；`palette` / `ownership` / `space` 刻意**不在** schema 里（见下表）。`evidence` 不是自由字典，而是 `g_evidence_paths` 那 **17 个固定键**（`additionalProperties:false` + 全 required），且只列**模型能写**的字段路径（坑 19） |
| 测试 | **新建** `src/test/core/ai_light_spec_prompt.cpp`（套件 `ai_light_spec_prompt`，10 用例，全离线）；`ai_light_stats.cpp` 补 7 个聚合用例；`ai_light_state.cpp` 改 3 处调用点并新增/改写 3 个用例 |

**五条实现决定（D1–D5）**

- **D1 改签名，不引入隐式全局。** `generate_spec(sqlite_database&, const spec_generation_request_t&)`；
  `advance_session(sqlite_database&, ai_light_session, uuid in_operator_id)`；请求体新增 `project_id_`（写
  `sd2::task.project_uuid_id_`）与 `operator_id_`（**计费主体**，同时也是签名 URL 里的 uid）。
  `in_operator_id` **刻意不给默认值**：有默认值的话，新调用点漏传就会把 `add_remaining_tokens_for_person`
  的账静默记到别人头上。nil 时在 `advance_session` 内回落到 `session.submitter_`，两者都 nil → 400
  （既解析不出工作室、也无法计费）。操作人**不加会话列**：它是「每次调用」的属性，加列会逼出一次迁移且语义错。
- **D2 聚合放在统计层。** `aggregate_reference_stats` 在 `ai_light_stats.{h,cpp}`，不在编排层：§5.1–§5.3
  整章是感知层，显示空间度量学口径（含「色温要先线性化」这唯一的例外）**只能有一处定义**。
- **D3 外点只算不写。** `is_outlier_` / `excluded_reason_` 是**人工排除**字段（`spec_route::post` 写的是
  `excluded_reason_ = "人工排除"`）。自动判定写进去会让两种语义在同一列里不可区分。自动疑点走
  `suspected_outlier_ids_` → `conflict_resolution_.resolved_` 持久化，交人工裁决。
- **D4 落库的降级级别以事实为准。** 调用方预判的 `degrade_level_` 不再直接落库；落的是
  `spec_generation_result_t.spec_.degrade_level_`（接缝按 `aggregated_reference_stats_t` 的
  `per_dimension_confidence_` 是否为空判定）。两者会在「有 stats 行但置信度全 0」「全部图被人工排除」时
  **分叉** —— 那两种情况下调用方的预判是 `stats_only`，而事实是「没有可用统计」。
- **D5 受控的副作用例外。** `generate_spec` **不写任何 `ai_light_*` 表**（Spec 的 insert 由 `advance_session`
  在自己的事务里做），但它经 `run_inference_task` 写一条 `sd2::task` 终态行并**扣费**。这一个副作用
  **有意保留**：推理确实发生了，必须留痕计费；否则「调用方回滚会话状态」会连带抹掉 `person_token` 的扣费。

**口径差（计划 vs 实现，以代码为准）**

| 位置 | 计划里写的 | 实现里落的 | 为什么 |
|---|---|---|---|
| 自动疑点阈值 | `d > 3.0` | `g_outlier_score_threshold = 2.5` | 参考图本来就少；抓错的代价（只是标成疑点 —— 仍进聚合、只是多一条线索给人工）远低于漏掉的代价，所以取得更敏感 |
| 冲突维度 | 「降级为不设目标」 | 照常给区间，只标冲突 + 压低置信度（地板 0.05） | 「清零」的语义是「没有统计」，那是 `prompt_only` 降级。**「有统计但互相矛盾」必须由人工闸门 1 裁决**，实现无权替它决定 |

**服务端钉死 vs LLM 可覆盖（`map_spec_result` 的边界）**

| 字段 | 谁说了算 | 理由 |
|---|---|---|
| `schema_version_` / `session_id_` / `human_confirmed_` / `confirmed_by_` / `confirmed_at_` | **服务端** | 与 LLM 无关的事实 |
| `targets_.space_` | **服务端**（恒 `display_srgb`） | 全部统计都在显示空间算，换空间得重算 |
| `targets_.palette_` | **服务端**（整块取自聚合） | 颜色只能来自统计，不接受 LLM 编造 —— 所以 schema 里根本没有 `palette` |
| `targets_.ownership_` | **服务端** | 责任归属是产品决策（§6.6），不该每轮由模型重新发明 |
| `reference_stats_` | **服务端**（整块） | 它是**证据**，不是结论 |
| `intent_` 全部 | LLM | 这就是「把提示词变成结构化意图」 |
| `targets_` 的标量区间 / `grading_hints_` | LLM | 与聚合值逐元素比（`1e-9` 容差）：相等 → `evidence.source = reference_stats`；不等 → `prompt`（§6.3 提示词优先） |
| `evidence_` | **服务端**（LLM 给的那份**根本不读**） | `map_spec_result` 完全用 `put_evidence()` 自己组装，末尾 `l_spec.evidence_ = l_ret.evidence_`。schema 里的 `evidence` 只是要求模型**自述**每个字段的理由（便于人工审阅原始响应），不参与解析。LLM 没给到的字段路径，按 `default_value` / `scene_facts` + 置信度 0.3 补一条，于是 evidence 永远覆盖全部目标字段（下游评判层可以无条件按字段路径查证据） |
| `conflicts_` | LLM，**并入服务端集合** | LLM 报的冲突加 `"llm: "` 前缀并入 `reference_stats_.conflicts_`；聚合器算出的冲突**不会被覆盖** |

**为什么 LLM 的每个字段都要校验**：`NLOHMANN_JSON_SERIALIZE_ENUM` 的 `from_json` 认不出字符串时
**静默落到第一个枚举值**（§15.6 坑 7）。所以 `map_spec_result` 里没有任何一处用裸 `get<Enum>()` ——
枚举一律走 `magic_enum::enum_cast`，认不出就**保留统计推导的值**并记一条 `default_value` 的 evidence
（例如 `hardness = "super_hard"` 不会变成 `hard`）。同理 `result_json_` 不是 object（含 null）直接
`bad_gateway`，**不二次解析 `result_text_`**（`parse_inference_response` 内部已做过 `strip_code_fence` + parse）。

**方舟合规性修复（拿「创建 Response」文档逐字段核对线格式后改的三处）**

前两处是**形状错**（真机会 400），第三处是**缺省值错**（只在长输出时、以「别处的错」的形式暴露）：

| # | 位置 | 错在哪 | 改法 |
|---|---|---|---|
| A | `build_spec_json_schema()` 的 `evidence` | 用了 `additionalProperties: <条目 schema>` 自由字典，与 `strict` 互斥 | 改成 `g_evidence_paths` 的 **17 个固定键** + 全 required + `additionalProperties:false`（坑 19） |
| B | `seedance2_client.cpp:127` 的 `input[]` 项 | 缺 `type:"message"`（文档写 InputItem 的 `type` 取值固定为 `"message"`） | 显式补上（坑 20） |
| C | `build_inference_body()` 的 `max_output_tokens` | 缺省替调用方塞了 4096；大 schema 被截断后的表现与「形状被拒」重合 | 缺省**不发**该键（坑 21） |

核对中确认**本来就对**、不必再动的四处：`content[].type == "input_text"`、`input_image.image_url` 是
**字符串**、`detail` 位于 `input.content.detail`、结构化输出用 `text.format`（Chat 才是 `response_format`）。

**测试**

| 用例（`ai_light_spec_prompt.cpp`） | 钉住的不变式 |
|---|---|
| `json_schema_matches_cpp_enums`（`:167`） | schema 里出现的枚举串与 C++ 枚举名**逐字一致**（用 `magic_enum::enum_names` 交叉验证）—— 两边一旦分叉，真机上的合法输出会被我们判成非法 |
| `json_schema_is_strict_object`（`:197`） | 逐层 `additionalProperties:false` 且全字段 `required`；并断言 `palette` / `ownership` **不出现**在 schema 里。此外钉住 `evidence` 的键集合**恰好**是 `g_evidence_paths` 那 17 条（排序后逐个比对）、`required` 是同一集合、`additionalProperties == false`，且 `targets.palette` / `reference_stats` **不在**其中（坑 19 的回归） |
| `input_text_carries_prompt_and_stats`（`:236`） | 四段文本齐全；提示词原文被标注为「权威意图」；逐图统计带在段里 |
| `build_request_omits_images_when_empty`（`:275`） | 没有可用参考图时**整个 `images` 键不发**（空数组会被方舟当成「有一张空图」）；有图时只发未被人工排除的那些，且是**站点内相对路径** |
| `map_result_pins_server_owned_fields`（`:317`） | `schema_version` / `space` / `palette` / `ownership` / `reference_stats` 全被服务端钉死，LLM 给什么都改不动 |
| `map_result_without_stats_degrades_to_prompt_only`（`:349`） | 聚合无统计时降级级别是 `prompt_only`（D4） |
| `map_result_prompt_overrides_stats_target`（`:357`） | LLM 给 `[2900,3100]`、统计是 `[4000,4200]` → 取 LLM 且 `evidence.source == prompt` |
| `map_result_echo_marks_reference_stats`（`:379`） | LLM 原样回显统计值 → `source == reference_stats`，且**不**记进 overridden 维度 |
| `map_result_falls_back_on_illegal_enum`（`:392`） | `hardness = "super_hard"` → 不静默落枚举首值，保留统计推导值 + 一条 `default_value` evidence |
| `map_result_rejects_non_object`（`:431`） | 非 object 的 `result_json_` → `bad_gateway`（`:441`、`:450` 两条断言） |

- `ai_light_stats.cpp` 的 7 个聚合用例在 `:391`–`:603`（`aggregate_takes_median_but_keeps_range_as_interval` 32 断言 /
  `aggregate_skips_excluded_and_statless_references` 8 / `aggregate_flags_conflict_without_narrowing_or_zeroing` 8 /
  `aggregate_expands_zero_width_intervals_for_a_single_reference` 15 / `aggregate_without_usable_stats_degrades_without_throwing` 9 /
  `aggregate_reports_suspected_outlier_without_touching_the_record` 7 / `aggregate_palette_shares_become_cross_image_intervals` 46）。
  它们钉的是 §5.3 的三条契约：**取中位数但区间取极差**（绝不用平均值收窄）、**冲突只标记不清零**、
  **外点只报不写**。
- `ai_light_state.cpp`：`unimplemented_seams_report_not_implemented` **整段已删** —— 三条接缝全部落地后
  没有 501 可断言了（`generate_spec` 见 §15.7、`generate_candidates` 见 §15.8、`structure_feedback` 见 §15.9）；新增
  `generate_spec_rejects_unresolvable_operator`（operator 为 nil 且
  submitter 也为 nil、以及 operator 指向不存在的 person，两条都断言 400）；两个回滚用例的期望从
  `not_implemented` 改成 `bad_request`，并额外断言 `failure_reason_` **不是**「没有可推进的步骤」——
  光看状态码分不出「状态机无步骤」与「接缝失败」。

**本轮的坑（承 §15.6 的 12 条，13–18）**

13. **`ai_light_reference::get_stats()` 返回的是值，不是 `optional`。** `reference.h:142` 是
    `json_get_or_default<reference_image_stats_t>(stats_)`，而 `json_get_or_default<T>` 返回 `T`（null → `T{}`）。
    第一版按 optional 用（`has_value()` / `->`）直接编译失败。判「这张图到底有没有统计」必须与匿名命名空间的
    `references_have_stats()` 用**同一谓词**：`!stats_.is_null() && stats_confidence_ > 0.0`。
    **只看 `is_null()` 不够**：`stats_confidence_` 是统计可信度，0 表示不该用；聚合器也是这么筛的 ——
    两处判据一旦分叉，接缝发给模型的图与它算出来的统计就会来自**不同的图集**。
14. **测试不能包含路由头。** `doodle_lib/http_method/kitsu/ai_light.h:27` 写的是
    `#include <core/http/http_function.h>`（相对 `doodle_lib` 根），而 `test_main` 的 include 路径是
    `-IE:\Doodle\src\test -IE:\Doodle\src -IE:\Doodle\src\doodle_core\..`，**没有** `E:\Doodle\src\doodle_lib`
    → `fatal error C1083: Cannot open include file: 'core/http/http_function.h'`。
    这正是 `inference_task.h` 当初被单独拆出来的同一个原因。于是测试里没法调 `reference_image_url()`，
    断言改为**测试自己拼**期望 URL：`fmt::format("/api/doodle/ai/light/reference/{}/image", boost::uuids::to_string(l_id))`。
    这反而更对：测试要钉的是**线格式**，跟着实现走就测不出格式漂移。
15. **`reference_image_url` 不在 `doodle::ai_light`。** 它和 `g_reference_image_path_template` 声明在路由头里，
    命名空间是 `doodle::http::ai_light`。接缝里必须写全名 `http::ai_light::reference_image_url(...)`，
    否则 `error C3861: 'reference_image_url': identifier not found`。
16. **沙箱不允许被启动的子进程写真实 `%TEMP%`。** 症状极具误导性：24 个用例同时报
    `doodle::orm::sqlite_orm_exception: 14: unable to open database file`（每个建临时库的用例）与
    `critical check cv::imwrite(...) has failed`（写临时 PNG 的用例），看起来像 ORM / OpenCV 坏了，
    其实是 `std::filesystem::temp_directory_path()`（`C:\Users\TD\AppData\Local\Temp`）对**子进程**不可写 ——
    同一个 shell 直接 `Set-Content` 到该目录却成功。跑测试前把 `$env:TEMP` 与 `$env:TMP` 都指向工作区内目录
    （`E:\Doodle\build\test_tmp`）即可（`GetTempPathW` 先读 `TMP` 再读 `TEMP`，两个都设最稳）。
17. **`cmake --build … *> log` 之后不要用 `Select-String` / `$LASTEXITCODE` 判成败。** 本机的 `Select-String`
    （含 `-AllMatches`）与 `Where-Object -match` 会**静默返回空**，于是失败的构建被报成 BUILD OK；
    而 `$LASTEXITCODE` 会被 `set_env.ps1` 包装脚本重置，同样不可信。可靠做法只有一个：**构建日志落文件，
    再用 grep 工具搜** `FAILED|error C|fatal error|ninja: build stopped`。另外 `cmake --build … | Select-Object -Last N`
    会让后台作业报 exit 0（管道最后一个命令的退出码），即使 ninja 已经失败。
18. **Boost.Test 打不出普通枚举与 vector 迭代器**（`print_helper.hpp: static_assert failed:
    'Type has to implement operator<< to be printable'`）。前者要套一层
    `template <typename E> std::string en(E in_e) { return std::string{magic_enum::enum_name(in_e)}; }`
    再比 `BOOST_TEST(en(a) == en(b))`；后者（`std::find(...) != end()` 这类）要**先落成 `const bool`** 再断言。
    这不是被测代码的问题，但报错位置会指到断言那一行，很容易误判成实现有 bug。
19. **严格结构化输出（`strict`）不接受 `additionalProperties: <条目 schema>` 这种自由字典。** `evidence`
    原本设计成「键 = 字段路径，值 = `{source, confidence, note}`」的字典，schema 里用 `additionalProperties`
    指向条目 schema —— 这是合法的 JSON Schema，但 OpenAI 系的严格模式要求**每个 object 都
    `additionalProperties:false`**，自由字典与 `strict` 互斥，最可能的真机表现是一句 400。
    修法：把它变成**有限键集合** —— `g_evidence_paths`（17 条字段路径，`:666`）逐键指向同一份条目 schema +
    全 required + `additionalProperties:false`。**这反而是更真的建模**：这张字典的键从来就是有限的，
    而且只该列**模型能写**的字段。`targets.palette` / `reference_stats` 是服务端口径（`map_spec_result`
    整块钉死，模型改不动），让模型为它们编理由没有意义，所以**刻意不放进 schema**。
    > 顺带一个更省事的观察：**LLM 返回的 `evidence` 我们根本不读** —— `l_ret.evidence_` 完全由服务端
    > `put_evidence()` 组装（`:753` 定义，14 个调用点），末尾 `l_spec.evidence_ = l_ret.evidence_`。
    > 所以 schema 里这个字段与其说是给解析用的，不如说是**要求模型自述理由**（便于事后审阅原始响应）。
    > 将来若要精简 schema，这整块可以删掉而不影响任何代码路径。**改 schema 前先 grep 一遍「这个字段
    > 到底有没有被读」**，能省掉一整轮「改了 schema 却什么都没发生」的困惑。
20. **Responses 的 `input[]` 项要显式带 `type:"message"`。** 文档写 InputItem 的 `type` 取值固定为
    `"message"`、`content[].type` 固定为 `"input_text"`。实现原来只发了 `role` + `content` ——
    `content[]` 那层是对的，**`input[]` 这层漏了 `type`**。这与坑 1（`image_url` 是字符串）属于同一类错：
    Responses 与 Chat 的形状只差一层，必须**逐层**照抄文档。补上后回归直接断言 `input[0].type == "message"`
    （`seedance2_inference.cpp` 的 `body_uses_responses_shape_with_string_image_url`）。
21. **`max_output_tokens` 的缺省值是个静默陷阱，干脆不发。** `build_inference_body` 原来写的是
    `in_request.value("max_output_tokens", g_ark_inference_default_max_output_tokens)`（4096，
    `static_value.h:72`）。对 `build_spec_json_schema()` 这种**每层都全字段 required** 的 schema，4096
    偏紧；一旦被截断，方舟返回 `status == incomplete` + 半截文本 → json 解析失败 → `result_json_` 变 null
    → 接缝抛 502。**最坏的是失败面重合**：它和「`text.format` 平铺形状被真机拒绝」的表现一模一样
    （都是 502 + `result_json_` null），会把人引到错误的方向去查。
    改法：**完全不发这个键**（让模型用自己的上限），只在调用方显式给了才发 ——
    `if (in_request.contains("max_output_tokens")) l_body["max_output_tokens"] = in_request.at("max_output_tokens");`
    `build_spec_request()` 相应删掉那一行。`g_ark_inference_default_max_output_tokens` 因此**再无任何引用**，
    作为「想自己设上限的调用方」的配置旋钮保留（注释已改写）。
    > 一般教训：**给一个「未指定」的请求参数塞缺省值，等于替调用方做了一个它没做的决定** ——
    > 而且这个决定只在长输出时、以「别处的错」的形式暴露出来。

**保留的边界**：不做 worker 侧推理（`--local` 没有 `deepseek_keys_`，LLM 只跑主服务器）、不做本地 JSON Schema
校验（vcpkg 里只有 nlohmann-json，schema 由方舟侧强制）、**接缝不重试**（重试全在客户端内：
`g_ark_inference_max_retry` / `should_retry` 覆盖 429/5xx/408）、**三条接缝全部落地**（§15.7 / §15.8 / §15.9）。
**单测宿主既没有 `core_set::ctx_ptr` 也没有 http logger**，所以单测只覆盖纯函数与
`advance_session` 的失败分支；`generate_spec` 的成功路径（真发一次推理）没有单测 ——
它依赖的推理通道自己的测试在 §15.6。

**回归**：全量 `test_main` **168 用例 / 1317 断言全通过、exit 0**（§15.6 时是 150 / 1103，
§15.3–§15.5 之间到 161 / 1182，本轮 +7 用例；A/B/C 三处修复后断言 1307 → 1317）。其中本轮的四个 AI 灯光套件：
`ai_light_schema` 6 用例 · `ai_light_state` 18（171 断言）· `ai_light_stats` 22（184 断言）·
`ai_light_spec_prompt` 10（83 断言）；另含 `sqlite_real_db`（真实生产库副本上的 `rebuild_all_tables` 往返）、
`sqlite_upgrade`、`sqlite_schema`、`sqlite_fk_semantics`、`sqlite_conn_pragma`。

**信封结构体化后的又一轮回归**（`build_spec_request()` 改返回 `http::seedance2::inference_request_t`，并删掉
`build_inference_body(const nlohmann::json&)` 这个兼容重载之后）：全量 `test_main` **171 用例 / 1362 断言全通过、
exit 0**（较基线 168 / 1317 为 +3 用例 / +45 断言，新增 `ai_light_spec_prompt/build_request_reaches_ark_body`
与 `seedance2_inference` 的 `inference_request_round_trips_through_struct` / `inference_request_rejects_wrong_typed_keys`）。
> 信封现在**只有一条入口** `build_inference_body(const inference_request_t&)`：裸 json 必须先显式
> `json.get<inference_request_t>()`（落库路径 `run_inference_task()` 就是这么做的），于是不可能再出现
> 「测试按 json 构造 body、生产按结构体构造」两套并行解析各说各话。

> 跑测试的环境要求（坑 16）：先把 `$env:TEMP` 与 `$env:TMP` 指向工作区内目录，否则所有建临时库 /
> 写临时 PNG 的用例会以「无法打开数据库文件」的形式集体失败。

---

### 15.8 已落地（接缝 2：`generate_candidates` —— 排轴 + 一次推理 + 确定性展开）

`POST /api/doodle/ai/light/session/{session_id}/iterate` 已可用（§13.1 第 ④ 项）。接缝 2 的形态与 §15.7 的接缝 1
刻意**同形**：同一条推理通道、同一套严格取值器、同一套失败映射；区别只在「模型的产出被我们拿去做多少事」。

```
advance_session(generating 分支)
  ├─ 收拢输入
  │   ├─ get_current_ai_light_spec()  → to_spec()（必须 human_confirmed_）
  │   ├─ get_ai_light_presets(true)   → 预设库（只取启用的）
  │   ├─ get_ai_light_selected_candidate(session, current_iteration) → warm start
  │   ├─ get_ai_light_iteration(session, current_iteration).get_judgement().feedback_axes_
  │   │      → 人工滑块反馈（**不走 LLM**），逐项 magic_enum::enum_cast<variant_axis_kind>，认不出的键忽略
  │   ├─ operator = in_operator_id ?: session.submitter_（都 nil → 400，无法解析工作室/计费主体）
  │   └─ iteration_id = core_set::get_set().get_uuid()   ← 提前铸造（推理任务行要用它落库）
  └─ generate_candidates(in_sql, req)
       ├─ operator → person → ai_studio（与 generate_spec 同形；查不到 → 400）
       ├─ build_candidates_instructions() + build_candidates_input_text(req)
       ├─ build_candidates_request(req) → http::seedance2::inference_request_t
       ├─ run_inference_task()          → result_json_（模型给的轴计划 + 基线 rig）
       └─ map_candidate_result(json, req)
            ├─ parse_axis_plan(json)       轴名 ∈ magic_enum、1..2 条轴、档位合计 == 5
            │                              ↑ 任一不满足 → default_axis_plan(spec) 回退 + strategy_ 记「弃用」
            ├─ build_base_rig(json)        严格取值器把模型给的基线物化成 light_t[]
            └─ expand_candidates(base, axes)  ← 纯确定性 OFAT：候选 0 = 基线，其余 5 个各沿一档
```

| 项 | 落点（`src/doodle_lib/http_method/kitsu/`，行号是起点，定位靠函数名；接缝 2 已拆到 `ai_light_seam_candidates.{h,cpp}` 与 `ai_light_rig_ops.{h,cpp}`，见 §15.11） |
|---|---|
| 预算常量：档位 5 / 轴上限 2 | `ai_light_rig_ops.h` 的 `g_axis_step_budget` / `g_axis_max_count` |
| 预算常量：strategy 500 / 预设 top-k 12 | `ai_light_seam_candidates.cpp` 的 `g_strategy_max_len` / `g_preset_prompt_top_k` |
| 轴语义与单位说明（嵌进 instructions 的原文） | `g_axis_value_semantics_text`（`ai_light_seam_common.h`，**接缝 2 与 3 共用**） |
| 通用灯光段落（§6.1 边界 + §5.5 六行表）**与接缝 1 共用** | `ai_light_seam_common.cpp` 的 `build_common_lighting_section()` `:202` |
| 接缝 1 专属指令（§6.3 冲突规则） | `ai_light_seam_spec.cpp` 的 `build_spec_instructions()` `:120` |
| 接缝 2 专属指令（轴语义 + 输出要求） | `build_candidates_instructions()` `:198` |
| 输出 schema（轴计划 + 基线 rig，平铺、严格） | `build_candidates_json_schema()` `:233` |
| 输入文本（Spec 投影 + 命中预设 + 上一轮反馈） | `build_candidates_input_text()` `:286` |
| 信封 | `build_candidates_request()` `:326` |
| 默认轴计划（回退用） | `default_axis_plan()` `:344` |
| 把模型的 JSON 物化成基线 rig | `build_base_rig()` `:368` |
| 单轴增量的纯函数（不碰 IO） | `ai_light_rig_ops.cpp` 的 `apply_axis_step()` `:51` |
| 确定性 OFAT 展开成恰好 6 个 | `ai_light_rig_ops.cpp` 的 `expand_candidates()` `:138` |
| 基线 → 候选的差异（供 UE 侧精确增删） | `ai_light_rig_ops.cpp` 的 `diff_rigs()` `:168` |
| 结果映射 + 护栏校验 | `ai_light_seam_candidates.cpp` 的 `map_candidate_result()` `:454` |
| 接缝本体（推理通道 + 失败映射） | `generate_candidates()` `:536` |
| 轴计划的严格解析（返回失败原因） | `parse_axis_plan()` `:143` |
| 预设的词法检索（无 embedding，见 §6.4） | `pick_presets()` `:113` + `preset_lexical_score()` |

**三条实现决定**

**D6 —— 轴计划是双向的（这是接缝 2 与 §6.5 唯一实质性的偏离）。** §6.5 把 `generate_candidates(spec, axes, n)`
写成「给定轴就机械展开」，`candidate_generation_request_t::feedback_axes_` 有值时确实是这个语义（滑块通道，
不经 LLM）。但**首轮与 Spec 升版后没有任何上一轮可选**，轴必须有出处 —— 于是这两条路径由**模型给轴计划**，
`default_axis_plan(spec)` 只作回退。理由：这正好落在 §6.1 划的界线上 ——「决定灯的数量/角色/类型/相对方位、
选择/组合预设模板」是模型的活，「逐帧精确数值」不是；「排哪两条轴、各多少档」属于前者，「按轴把 rig 的某个
子集改掉」属于后者且是纯算术。把后者交给模型只会引入「5 个候选其实改的是同一条轴的同一个方向」这类
**不可检测**的错误。

**D7 —— 预设检索走词法，不走向量。** 理由见 §6.4 的注（没有查询向量生成器）。`pick_presets()` 取
top-`g_preset_prompt_top_k`(12) 只送进 `input_text` 当知识注入，不参与展开 —— 预设本轮的角色是「让模型知道
我们有哪些成熟打法」，不是候选骨架。

**D8 —— `baseline_level_` 本轮只透传（这是 M2 的阻塞项）。** 它由 `advance_session` 填、`map_candidate_result()`
钉到每个 rig 上，**没有生产端**。真正算出这个值要走 `auto_task::shot_render_light(project_id, task_id)` 内部的
三段私有逻辑：`shot_render_light_builder::fill_base()` 的 episodes/shot 推导、`resolve_scene_ue_path()`、
`shot_pretreatment_probe::run()`；其中 `get_entity_and_entity_asset_extend_by_shot_id()` 还在
`src/doodle_lib/http_method/kitsu/auto_task.cpp` 的**匿名命名空间**里，库外不可用，而复用整个
`shot_render_light()` 又会带上 `check_files` / `scan_shot_output` 等一整串重失败面。
现在 `baseline_level_` 为空时 `advance_session` 打 `WARN`（措辞含「M2 阻塞项，见方案 §15.8 决定 D8」）**不拦**；
`lighting_rig_t::baseline_level_` 默认空串，JSON 往返无碍。**UE 侧执行器拿到空值必须拒绝**，而不是「就渲染
当前关卡」—— 那会污染美术的工程。

**口径差（计划/文档原文 vs 实现，以代码为准）**

| 项 | 原文 | 实现 | 为什么 |
|---|---|---|---|
| 候选展开方式 | §6.2「每轴 2–3 档」× 6 个候选，读起来像全因子组合 | **OFAT**：候选 0 = 基线，其余 5 个各沿一档 | `variant_axis_t` 只有一条轴（`rig.h`），本来也表达不出组合 |
| 档位预算 | §6.2 只说「每轴 2–3 档」 | 档位**总数**必须恰好 `g_axis_step_budget = 5` | 6 = 1 基线 + 5 变异，只能这么分；`parse_axis_plan()` 硬校验 |
| 轴数 | 未明确 | 1..`g_axis_max_count = 2` | 全压在一条轴上会让 6 张图在同一维度单调铺开，美术看不出「换维度」；两条轴可以交叉 |
| 预设检索 | §6.4 写 embedding → top-k | **词法子串打分** | 没有查询向量生成器（§6.4 注） |
| `feedback_axes` 的键 | §4.4 样例 `{"key_elevation_delta": -15, "kelvin_delta": -600}` | 键必须是 `variant_axis_kind` 的**枚举名**（**没有 `key_elevation`**），value 是**增量**不是绝对值 | `advance_session` 用 `magic_enum::enum_cast` 解析键名；§4.4 的样例本轮已按此修正（§15.9） |
| `image_detail` | 计划 §三.E 写发 `image_detail="high"` | **不发** | 接缝 2 没有 images；给「不存在的输入」配参数正是坑 21 的同一类错误 |
| 回退时的 `strategy_` | 测试原断言含「默认轴计划」四字 | 实现是 `"{}模型给的轴计划被弃用：{}"` | 测试改为断言「弃用」+ 退回的轴表**逐条等于** `default_axis_plan(spec)`（坑 24） |

**服务端钉死 vs 模型可覆盖**

| 字段 | 谁定 |
|---|---|
| 6 个候选的 rig（含每条轴各自的一档） | **服务端**：`expand_candidates()` 确定性展开 |
| 每个 rig 的身份字段 `session_id_` / `spec_id_` / `iteration_` / `candidate_index_` / `rig_id_` | **服务端**：`advance_session` 覆盖 —— 接缝只负责「灯怎么打」，不负责「这是第几个」 |
| `baseline_level_` | **服务端**：透传，接缝不产生（D8） |
| `schema_version_`、`exposure_.method_ = "manual"` | **服务端**（曝光必须手动锁定，§7.5） |
| `provenance_`（`model_` / `prompt_hash_` / `seed_`） | **服务端**：`model_` 取自配置，`prompt_hash_` 由 Spec + 轴计划确定性算出 |
| `ops_` | **服务端**：`diff_rigs()` 从「基线 rig → 候选 rig」的差算出，模型**不写 op** |
| 灯的 `id_` / `role_` / `type_` / 锚点 / 偏移 / 强度 / 色彩 / 形状 / 阴影 | **模型**（在基线 rig 里给） |
| 全局：曝光 / 白平衡 / 环境 / 调色 | **模型** |
| 轴计划（哪 1–2 条轴、各多少档） | **模型**（校验不过则回退默认计划） |

**为什么「候选展开」必须在我们这边，而不是交给模型**

- 展开规则是**纯算术**（`apply_axis_step()` 按轴改 rig 的一个已知子集）。由我们展开时，第 i 个候选的
  `variant_axis_` 是**构造出来的**，于是 §6.6「6 个候选的灯集合与角色完全相同」**天然成立** ——
  测试 `expand_candidates_is_ofat_around_the_baseline` 逐个比较 6 个候选的灯 id 与 role 就是在钉这条。
- 代价：模型**无法给组合式候选**（比如同时改色温与对比度）。这是有意的 —— 组合式候选一旦被选中，
  下一轮在该点上收缩时无法归因到单轴，§6.2「把人工判断变成有效的搜索梯度」就失效了。
- 副产品：`diff_rigs()` 把差异同时写成 `ops_`，UE 侧才能做**精确增删**（§7.5：`ClearAllLight()` 会清掉美术的
  手工灯，不能复用）。测试 `map_result_pins_identity_and_emits_ops` 断言 ops 里**不出现 add/remove** ——
  本轮的候选是从基线 rig 整体替换，不是增量编辑既有灯。

**测试**（新建 `src/test/core/ai_light_candidates.cpp`，套件 `ai_light_candidates`，**15 用例 / 214 断言，全离线**）

| 用例 | 钉什么 |
|---|---|
| `axis_enum_excludes_none` | schema 的轴枚举**不含 `none`** —— 模型不该被允许说「这条轴不变」，不变的那个是基线候选 |
| `json_schema_is_strict_and_free_of_server_fields` | 每层 `required == properties` 且 `additionalProperties:false`；dump 里**不得**出现 `variant_axis`/`baseline_level`/`rig_id`/`candidate_index`/`schema_version`/`ops`；`lights` **不设 `maxItems`**（灯数由模型判断，决策 17 不设硬上限） |
| `build_request_omits_absent_inputs` | 信封不含 `images`/`image_detail`/`max_output_tokens`/`thinking`；`text.format.name == "lighting_candidates"`、`strict == true`、`temperature == 0.2`；content 里没有 `input_image` |
| `input_text_carries_spec_presets_and_feedback` | 四段文本都在（Spec / 预设 / warm start / 上一轮反馈）；**停用的预设不进**提示 |
| `default_axis_plan_fills_six_candidates` | 回退计划自身就能填满 6 个候选 |
| `base_rig_materializes_ids_and_locks_exposure` | 灯 id 依次 `L00`/`L01`/`L02`；`exposure_.method_ == "manual"`；`schema_version_ == g_rig_schema_version` |
| `base_rig_rejects_illegal_values` | 非法 role / color_mode、缺 `fog_density`、`rgb` 非 3 元、空 `lights` **全部**抛 `doodle_error` |
| `apply_axis_step_moves_only_the_named_axis` | 每条轴只动它该动的：`key_direction` 改方位角（含 360 环绕）且**不动补光**；`key_fill_ratio` 只缩补光且**主光不动**；`kelvin` 同时动灯与全局白平衡；`grading_strength` 只动调色不碰灯；`ambient_level` 天光与 ambient 灯同抬；无补光灯时 `key_fill_ratio` 返回 false |
| `expand_candidates_is_ofat_around_the_baseline` | 候选 0 的 `axis == none`、其余逐轴逐档；**6 个候选的灯 id 与 role 逐个与基线相同**（§6.6）；展开器**不填** `ops_` |
| `map_result_pins_identity_and_emits_ops` | 身份字段与 `baseline_level_` 钉死、`rig_id_` 为 nil、`provenance_.model_` 取自配置、候选 0 无 op 其余非空、ops 里不出现 add/remove |
| `map_result_prompt_hash_is_deterministic` | 同一 Spec + 同一轴计划 → 同一 `prompt_hash_`（§8.5 第 ① 条「同一 Spec + 同一轴 → 同一组候选」的可测形式） |
| `map_result_falls_back_on_illegal_axis_plan` | 0 档、未知轴名 `key_elevation` 都退回默认计划 |
| `map_result_rejects_axis_without_effect` | 去掉补光灯后 `key_fill_ratio` 无实质变化 → 502 |
| `map_result_rejects_missing_key_light` | 没有主光 → 502 |
| `map_result_rejects_non_object` | 结果非 object → 502 |

**本轮的坑（承 §15.6 的 12 条与 §15.7 的 13–21，22–24）**

22. **`BOOST_CHECK_EQUAL_COLLECTIONS` 里不要写临时量的 `begin()/end()`。** `keys_of()` / `to_strings()` 这类
    **按值返回 vector** 的助手，一次表达式里被调用两次会得到**两个不同临时对象**的迭代器 —— MSVC 调试
    迭代器直接以 `vector(202): Assertion failed: vector iterators incompatible` **abort 整个用例**
    （不是断言失败，是抛异常；`--report_level=detailed` 里表现为 `abort` 而不是 `failed`）。必须先把返回值
    绑到 `const auto` 局部量再传 `begin()/end()`。
23. **协议的必填字段要「形状与上下文无关地」校验。** `build_base_rig()` 一开始只在 `color_mode == rgb` 时
    校验 `rgb` 三元组，于是 `color_mode="temperature"` + `rgb:[1,2]` 这种**明确的协议违规**被静默忽略，
    产出一盏「看起来完全正常的灯」。这与 §15.7「裸 `get<Enum>()` 静默落值」是**同一类错误**：
    **「模型没按 schema 回答」这件事必须暴露，不能因为它这次「用不到」就放过。**
24. **口径断言的措辞是脆的，行为断言是稳的。** 测试原本断言 `strategy_` 含「默认轴计划」四个字，实现写的
    是「模型给的轴计划被弃用」—— 两边都没错，但这类断言改一次文案就红一次。改成断言「含『弃用』」+
    「退回的轴表**逐条等于** `default_axis_plan(spec)`」。一般建议：对回退/降级路径，断言**退回后的状态**，
    而不是**说明退回的那句话**。

**保留的边界**：接缝 2 同样**不落任何 `ai_light_*` 表**（§15.7 决定 D5 的受控例外继续适用：唯一副作用是
`run_inference_task()` 自己那个事务里的推理任务行 + 扣费）；**不做灵敏度标定**（§9.1 第二条，需要渲染回环，
M2 之后）；**不校验灯集合一致性** —— §6.6 的另一半留在 worker 侧，本轮没有 worker，能锁的只有
「数量恰好 6」（`advance_session` 校验，失败 500）与「展开时灯集合相同」（`expand_candidates` 结构性保证）
这两条（`structure_feedback` 见 §15.9）。

**回归**：全量 `test_main` **186 用例 / 1574 断言全通过、exit 0**（§15.7 末尾的基线是 171 / 1362，本轮
+15 用例 / +212 断言，全部来自新增的 `ai_light_candidates` 套件）。本轮五个 AI 灯光套件：
`ai_light_schema` 6（126 断言）· `ai_light_state` 18（169）· `ai_light_stats` 22（184）·
`ai_light_spec_prompt` 11（117）· `ai_light_candidates` **15（214）**；另含 `sqlite_real_db`
（真实生产库副本上的 `rebuild_all_tables` 往返）、`sqlite_upgrade`、`sqlite_schema`、`sqlite_fk_semantics`、
`sqlite_conn_pragma`。

### 15.9 已落地（接缝 3：`structure_feedback` —— 自由文本 → 轴方向）

`POST /api/doodle/ai/light/session/{session_id}/judge` 的**文本通道**已可用（§13.1 第 ⑨ 项）。至此**三条接缝
全部落地，仓库里不再有 `not_implemented` 的端点**（§15.7 / §15.8 / 本节）。

接缝 3 与另两条的**方向相反**：接缝 1/2 是「Spec / 轴 → 灯」，接缝 3 是「人话 → 轴」，所以它是三条里唯一
**既不产生 rig、也不改任何表**的一条 —— 它只把一段自由文本翻译成「下一轮往哪个方向试」。

```
judge_route::post
  ├─ l_json.get_to(l_judgement)                 → 滑块通道 feedback_axes 先落地（决策 5 的默认通道）
  ├─ decision_ == accept → 校验 selected_candidate_id_ 非 nil 且属于本轮 → build_accepted_preview_file()
  ├─ if (!feedback_text_.empty() && decision_ == continue_iteration)        ← D16：只在这一个决定下调 LLM
  │    ├─ 本轮看的 Spec：扫 get_ai_light_specs(session) 找 uuid_id_ == iteration.spec_id_
  │    │      ↑ 不用 get_by_uuid<T>（查不到会抛 sqlite 异常 = 500），也不用「最新一版」
  │    │        （Spec 升版后会把美术当时看的图对错目标）→ 找不到就 WARN 并跳过接缝
  │    ├─ feedback_structuring_request_t{…, candidates_ = get_ai_light_candidates(iteration_id),
  │    │                                  slider_axes_ = l_judgement.feedback_axes_}
  │    └─ structure_feedback(in_sql, req)
  │         ├─ operator → person → ai_studio（make_seam_client，与另两条同形）
  │         ├─ build_feedback_instructions() + build_feedback_input_text(req)
  │         ├─ build_feedback_request(req) → inference_request_t（temp 0.2 / strict / "lighting_feedback"）
  │         ├─ run_inference_task() → result_json_
  │         └─ map_feedback_result(json) → std::vector<axis_step_t>
  │              ├─ 形状违规 → 抛 bad_gateway（协议被无视）
  │              └─ 语义无效 → 丢弃（none / 0 / 未知轴名 / 非正 key_fill_ratio / 同轴重复）
  ├─ 合并：滑块已给的轴**不覆盖**（D9），其余按 magic_enum::enum_name(axis_) 写回 feedback_axes_
  ├─ catch (const doodle_error&) → WARN + 降级为「纯滑块 + 文本原样留存」（D14）
  └─ apply_judgement() 落库 → 下一轮 advance_session 的 generating 分支读它当**约束**用（§15.8 数据流）
```

| 项 | 落点（`src/doodle_lib/http_method/kitsu/`，行号是起点，定位靠函数名；接缝 3 已拆到 `ai_light_seam_feedback.{h,cpp}`，见 §15.11） |
|---|---|
| 接缝 3 请求结构体（11 字段，含 `spec_` / `candidates_` / `slider_axes_`） | `ai_light_seam_feedback.h:262` 前（`: public seam_request_base_t`，三字段已上提到 `ai_light_seam_common.h` 的 `seam_request_base_t`） |
| 双参签名 + 6 个纯函数声明 | `ai_light_seam_feedback.h` 的 `structure_feedback` / `build_feedback_*` / `map_feedback_result` |
| 调用方（组装请求 + 合并 + 降级） | `ai_light.cpp:713`（`judge_route::post` 内） |
| 轴值语义（与 `apply_axis_step` **共用同一份口径**） | `g_axis_value_semantics_text`（`ai_light_seam_common.h`，**接缝 2 与 3 共用**） |
| 当前 key:fill 比（`apply_axis_step` 的该分支也改用它） | `ai_light_rig_ops.cpp` 的 `key_fill_ratio_of()` `:30` |
| 共用样板：解析工作室 / 统一失败映射（**三条接缝同用**） | `ai_light_seam_common.cpp` 的 `make_seam_client()` `:51` · `chick_inference_ok()` `:88` · `run_seam_inference()` `:116` |
| 反馈轴上限 2 / label 上限 200 / 数值裁剪 | `ai_light_seam_feedback.cpp` 的 `g_feedback_axis_max_count` / `g_feedback_label_max_len` · `trim_number()` |
| 接缝 3 专属指令（角色 + 轴语义 + 六条约束） | `build_feedback_instructions()` `:90` |
| 输出 schema（`understanding` + `axes[≤2]`） | `build_feedback_json_schema()` `:117` |
| 输入文本（七段） | `build_feedback_input_text()` `:138` |
| 信封 | `build_feedback_request()` `:227` |
| 结果映射（形状抛错 / 语义丢弃） | `map_feedback_result()` `:244` |
| 接缝本体（推理通道 + 失败映射） | `structure_feedback()` `:310` |
| 测试 | `src/test/core/ai_light_feedback.cpp`（11 用例 / 84 断言，全离线） |

**八条实现决定**

**D9 —— 滑块优先，文本只填空。** 同一轴上请求体里的 `feedback_axes_`（滑块）与文本推导的轴冲突时，**滑块赢**
（调用方 `if (l_judgement.feedback_axes_.contains(l_name)) continue;`）。理由：决策 5 把滑块定为默认通道 ——
美术拖出来的数值是精确的，自然语言不是。**这修掉了一处偶然**：原代码先 `get_to` 滑块、再写文本推导的轴，
于是「文本覆盖滑块」只是执行顺序的副产品，不是任何人决定过的优先级。

**D10 —— 语义无效的轴在源头丢弃（WARN），只有形状违规才抛错。** 丢弃 = 未知轴名 / `none` / `value == 0` /
非有限数 / `key_fill_ratio <= 0` / 同轴重复（首个胜）。抛错（502）= 结果非 object / 缺 `understanding` /
缺 `axes` / `axes` 超过 2 条 / 元素缺 `axis` / `value` 非数字。**为什么无效轴必须在这里死掉**：它一旦传下去，
下一轮 `advance_session` 会把它变成 `axis_step_t`，`apply_axis_step()` 对 `none` 与 `0` 恒返回 `false`，
最终 `map_candidate_result()` 以「轴无实质变化」抛 502 —— **病根（模型给了一条没有语义的轴）与表现
（下一轮 `/iterate` 502）之间隔了一次人工判断**，美术得先白判一轮。未知轴名丢弃与 `advance_session`
读滑块时既有的容错一致。

**D11 —— 每条轴恰好一个值（`steps_ = {value}`）。** 调用方只取 `front()` 当下一轮的中心值（`ai_light.cpp`），
完整的档位表由下一轮 `generate_candidates` 的 `axes_` 决定。给多个值不是「更详细」，而是给调用方一个
会被静默丢掉的东西。

**D12 —— 最多 2 条轴（schema `maxItems: 2`）。** `parse_axis_plan()` 只接受 1..`g_axis_max_count`(2) 条轴，
而 `feedback_axes_` 在下一轮是**约束**（`build_candidates_input_text()` 要求「这些轴必须出现在你的 axes 里」）
—— 文本给 3 条轴会让轴计划**必然被弃用**，文本通道静默失效。

**D13 —— 允许空 `axes`（schema 不设 `minItems`）。** 「再调调看」这类没有方向的反馈是**合法输入**，
返回空 vector 即退回纯滑块。把它当协议违规会逼模型在没听出方向时硬编一个方向出来。

**D14 —— 调用方降级范围放宽到所有 `doodle_error`**（WARN 带上 `code_status_()`），`std::exception` 不吞。
原代码只吞 `not_implemented`、其余 rethrow；接线之后「模型说这条反馈不合法」会直接变成 `/judge` 的 5xx。
理由：文本是**补充**通道，一次人工判断的成立不依赖于它。加 `WARN` 是为了不把它变成静默失败。

**D15 —— 抽出共用样板 `make_seam_client()` + `chick_inference_ok()`。** 三次复制约 60 行；
「什么算可解释的失败」（内容违规 400 / 其它 502 / 输出非 JSON 对象 502）是一条**判定**，
三份拷贝漂移会让同一个失败在不同端点给出不同状态码。

**D16 —— 只在 `decision_ == continue_iteration` 时调 LLM。** `accept` / `reject_all` 都会结束会话，
结构化的轴没有下一轮可用 —— 那时调一次是纯粹的计费浪费。

**口径差（计划/文档原文 vs 实现，以代码为准）**

| 项 | 原文 | 实现 | 为什么 |
|---|---|---|---|
| `image_detail` | 计划 §三.E 写发 `image_detail="high"` | **不发** | 接缝 3 同样没有 images；与 §15.8 同一条原则（坑 21：不给「不存在的输入」配参数） |
| `axes` 的 `maxItems` 校验时机 | 计划未写 | **先查形状，后滤语义** | 「模型给了 6 条轴」是协议被无视，不能因为其中 5 条会被丢弃就当没发生（坑 25） |
| 轴语义段文本 | §15.8 里是 `g_axis_semantics_text`，同时含轴语义与排轴硬约束 | 拆成 `g_axis_value_semantics_text`（只留轴语义），硬约束段移进 `build_candidates_instructions()` | 接缝 3 复用语义但**不继承排轴规则**；同一份口径只有一处定义 |
| 同轴上文本 vs 滑块的优先级 | 计划未写（代码里是执行顺序的偶然结果） | **滑块优先**（D9） | 决策 5：滑块是默认通道 |
| 调用方吞什么 | 原代码只吞 `not_implemented`，其余 rethrow | 吞**所有** `doodle_error`，`std::exception` 不吞（D14） | 文本是补充通道，人工判断不因它失败 |
| 接缝签名 | §13.1 早期写 `(session_id, text)` | `(sqlite_database&, const feedback_structuring_request_t&)` | 定方向要读当前 Spec、本轮候选与滑块 → 必须能查库；与另两条接缝同形 |

**服务端钉死 vs 模型可覆盖**

| 字段 | 谁定 |
|---|---|
| 轴的**语义**（角度增量 / K 增量 / 绝对比值 / 相对增减） | **服务端**：`apply_axis_step()` 是唯一定义，`g_axis_value_semantics_text` 只是把同一份口径念给模型听 |
| 每条轴的值个数 | **服务端**：恒 1 个（D11） |
| 轴数上限 / 无 `none` / 无 `0` / 无未知轴名 / 无重复轴 | **服务端**：`map_feedback_result()` 丢弃（D10） |
| 写进 `feedback_axes_` 的键 | **服务端**：`magic_enum::enum_name(axis_)`，**不用**模型给的字符串 |
| 与滑块的冲突 | **服务端**：滑块优先（D9） |
| `understanding` | **模型** —— 只进日志，**不进库、不影响控制流**（没有「模型对反馈的理解」这一列：一个不可校验的自由文本不该污染可查询的结构） |
| 轴名 + 值 + `reason`（`reason` 变成 `label_` 回显给美术） | **模型** |

**测试**（新建 `src/test/core/ai_light_feedback.cpp`，套件 `ai_light_feedback`，**11 用例 / 84 断言，全离线**）

| 用例 | 钉什么 |
|---|---|
| `axis_enum_excludes_none_and_schema_is_strict` | schema 的轴枚举**不含 `none`**；每层 `required == properties` + `additionalProperties:false`；`axes` 有 `maxItems: 2` 但**没有 `minItems`**（D12/D13） |
| `instructions_split_the_shared_semantics_from_seam2_constraints` | 轴语义段与接缝 2 **逐字同源**（`g_axis_value_semantics_text` 是唯一出处）；接缝 2 的「恰好 5 个档位 / 档位不要取 0」**不出现在**接缝 3 的指令里 |
| `input_text_carries_verbatim_feedback_and_candidates` | 美术原话**逐字**出现；Spec / 决定 / 6 个候选各自的轴与值 / 选中候选的完整 rig / 当前 key:fill 比都在 |
| `input_text_marks_slider_axes` | 滑块已给的轴在提示里被明确标成「不要再给」 |
| `input_text_survives_empty_candidates_and_no_selection` | 候选还没落库、美术也没指定候选时提示词照样成型（不拼出空段、不崩） |
| `key_fill_ratio_of_is_unit_aware` | 主光与补光**单位不同**（candela vs lumen）时返回 `nullopt`；两盏补光求和后再比 |
| `map_result_returns_one_step_per_axis` | 每条轴恰好一个值（D11）；`reason` 缺失时 `label_` 回落成轴名（`label_` 会回显给美术，不能是空串） |
| `map_result_drops_none_zero_unknown_and_bad_ratio` | `none` / `value == 0` / 未知轴名 `key_elevation` / 非正 `key_fill_ratio` 全被丢弃；**同一批里合法的那条留下**（丢的是那一条，不是整份结果；分批喂见坑 25） |
| `map_result_keeps_first_of_duplicate_axis` | 同轴重复时**第一个胜** —— 确定性，不取决于模型输出的顺序 |
| `map_result_accepts_empty_axes` | 空 `axes` 是合法输出（D13），返回空 vector → 退回纯滑块 |
| `map_result_rejects_shape_violations` | 非 object / 缺 `understanding` / 缺 `axes` / 3 条轴 / 元素缺 `axis` / `value` 非数字 —— **全部 502** |

**本轮的坑（承 §15.6 的 12 条、§15.7 的 13–21、§15.8 的 22–24，续 25–27）**

25. **形状校验先于语义过滤 —— 「丢弃」的测试必须分批喂。** `map_feedback_result()` 先查 `axes.size() <= 2`，
    再逐条滤语义。测试一开始把 6 条「该被丢掉的轴」堆进同一个数组，撞上的却是「超过 2 条」这条**形状**违规，
    用例当场 abort，看上去像实现在乱抛错。一般教训：**任何「先查整体形状、再逐项过滤」的函数，
    针对「某项被过滤」的用例每批都不能超过形状上限。** 顺带记下：Boost.Test 会把 abort 的用例**同时**
    计进 `failed` 与 `aborted`（11 个用例报 10 passed / 1 failed / 1 aborted），而 `grep 'Test case .*has failed'`
    零命中 —— 别把它当成两个问题去查。
26. **`BOOST_TEST(std::find(...) != vec.end())` 编译不过。** Boost.Test 要打印操作数，而 vector 迭代器没有
    `operator<<` → `print_helper.hpp(53): error C2338: static_assert failed: 'Type has to implement operator<<
    to be printable'`。与坑 22 同族（一个表达式里出现临时量 / 不可打印类型），触发方式不同：
    先把结果收敛成 `const bool l_has_none = ...;` 再断言。
27. **一个缺失的类型限定会炸出十几条错。** 共用样板里写了未限定的 `ai_client_base`，实际是
    `http::seedance2::ai_client_base`；报出来的是十几条 `C2039`（`result_json_ is not a member of
    boost::asio::awaitable<...>`）、`C7595`、`C2664`，全在别处。**只修第一条 `error C2065: ... undeclared
    identifier`，其余多半自己消失。**

**保留的边界**：接缝 3 同样**不落任何 `ai_light_*` 表**（§15.7 决定 D5 的受控例外继续适用：唯一副作用是
`run_inference_task()` 自己那个事务里的推理任务行 + 扣费）；**输入提示词里不放候选指标** —— M2 之前
`ai_light_candidate::metrics_` 必为空，放了就是「给不存在的输入配参数」（坑 21 的同一原则），
**记为 M2 之后的增强项**（有了指标，「太暗」才能被定到 `ambient_level` 而不是靠猜）；**不做灵敏度标定**
（§9.1 第二条，需要渲染回环，M2 之后）；**`understanding` 不落库**。

**回归**：全量 `test_main` **197 用例 / 1660 断言全通过、exit 0**（§15.8 末尾的基线是 186 / 1574，本轮
+11 用例 / +86 断言，全部来自新增的 `ai_light_feedback` 套件；`ai_light_state` 的断言数由 169 变为 171 ——
501 断言删除、两条「解析不出触发人」的断言增加）。本轮六个 AI 灯光套件：`ai_light_schema` 6（126 断言）·
`ai_light_state` 18（171）· `ai_light_stats` 22（184）· `ai_light_spec_prompt` 11（117）·
`ai_light_candidates` 15（214）· `ai_light_feedback` **11（84）**。

### 15.10 已落地（取消会话的三个终态：`accepted` / `rejected` / `failed`）

**需求（用户原话，m02347）**：「将 `src/doodle_core/metadata/ai_light/session.h:64~69` 选中的几个状态全部取消,
我们不再使用 这几个状态, 基本情况是美术可能无限的调整某个镜头灯光, 使用可行」。

一句话：**会话不再有终态**。定稿不再是终点，全否也不再是终止，失败没有独立状态。会话是一个长期工作台，
美术对同一个镜头的灯光可以无限轮调整。

**数据流**

```
/judge
  └─ apply_judgement(sqlite_database&, ai_light_session&, ai_light_iteration&,
                     const ai_light_judgement_t&, std::optional<preview_file>)
       ├─ 闸门：轮次属于会话 · session.state_ == awaiting_judgement · iteration.state_ != judged
       ├─ accept ─┬─ 解除上一次定稿（clear_accepted() + 清 preview_file_id_，**同事务先跑**）
       │          ├─ mark_accepted() + preview_file_id_ = 新的 preview_file
       │          └─ is_selected_ = true   ← 下一轮 warm start 从**定稿那套 rig** 出发
       ├─ continue_iteration ── is_selected_ = true
       ├─ reject_all ────────── 不选候选（下一轮从 LLM 基线重新开始）
       └─ 三者**都** mark_judged(generating)   ← 没有终态，`POST /iterate` 立刻可用
```

**项 / 落点**

| 项 | 落点 |
|---|---|
| 状态枚举（删三个量 + 三对 JSON 映射 + `is_terminal`） | `src/doodle_core/metadata/ai_light/session.h`（枚举上方注释重写为「会话状态机 —— **没有终态**」） |
| 三个决定的会话状态 | `src/doodle_lib/http_method/kitsu/ai_light_orchestrator.cpp` 的 `apply_judgement()`：原 `switch` 换成 `const ai_light_session_state l_next_state = generating;` |
| 定稿指针（可移动） | 同上，accept 分支**之前**的解除块（`clear_accepted()` + `preview_file_id_ = uuid{}` + 同事务 UPDATE） |
| 定稿也置 `is_selected_` | 同上，候选 UPDATE 之前的统一赋值（D20） |
| 会话可变性（删 `check_session_mutable` + 三个调用点） | `src/doodle_lib/http_method/kitsu/ai_light.cpp` |
| 删会话不再需要定稿闸门 | 同上，`session_instance_route::delete_`（改为注释说明交付物为什么不受影响） |
| 数据迁移 | **没有** —— 见下方「**没有数据迁移**」一节（30 版库没有 `ai_light_*` 表，表由 30 → 31 新建，出生即空）。`sqlite_upgrade.cpp` 里**不留任何改写逻辑** |
| 版本常量 | `sqlite_upgrade.cpp`：`g_version_30` / `g_current_version = 31`（`g_version_29` 已随 29 → 30 那一步移除）；`upgrade_3_t` 是 30 之后的**唯一**一步，所以它写 `g_current_version` |
| 测试 | `src/test/core/ai_light_state.cpp`（改 5 处 + 新增 1 用例）· `ai_light_schema.cpp`（`by_state(accepted)`→`generating`；版本断言**仍是 31**，去掉了 31→32 那一段叙述）· `sqlite_upgrade.cpp`（版本断言**仍是 31**；无新增用例） |
| 文档 | 本节 · §13.1（状态机表 + 接缝契约 + 取舍第 3 条）· §10.2（冗余指针）· `docs/ue_ai_lighting_frontend.md`（9 处） |

**实现决定（承 §15.7 的 D1–D5、§15.8 的 D6–D8、§15.9 的 D9–D16，续 D17–D23）**

| # | 决定 | 理由 |
|---|---|---|
| D17 | 三个决定**都**落到 `generating` | 它是唯一能再出一轮的状态（`POST /iterate` 只接受它）。判完之后唯一的前进方向就是再出一轮 |
| D18 | `mark_judged()` 继续清空 `judgement_due_at_` | 语义不变；过期仍是 `awaiting_judgement` 上的派生标志（§15.4） |
| D19 | 定稿**可重复**：accept 前先解除上一次定稿（同事务），旧 `preview_file` **不删** | 库层 `accepted_session_id` 唯一索引是已记录的设计保证（§10.2 / §15.3），不先解除旧行必然撞唯一约束；而旧的 `preview_file` 已挂在 `task_id_` 上，是交付物，删掉等于抹掉美术交付过的一张图 |
| D20 | 定稿**也**置 `is_selected_ = true` | 原代码刻意不置，理由是「定稿没有下一轮」；无限轮之后理由消失 —— 定稿后接着调时 warm start 应从定稿那套 rig 出发，而不是回到 LLM 原始基线 |
| D21 | 删掉「定稿后不能删会话」的闸门 | 定稿图由 `preview::save_variants()` 落在**图片预览文件目录**（`get_pictures_preview_file(uuid)`），不在 `root_/ai_light/<会话>` 下；`ai_light_candidate.preview_file_id_` → `preview_file` 的 FK 是 **set_null**（`sqlite_database.cpp:516-518`）不是 cascade，所以删会话不会动交付物 |
| D22 | **不需要任何数据迁移**；`upgrade_4_t` 与其改写语句**一并删除** | 30 版库里根本没有 `ai_light_*` 表（唯一的建表者就是 30 → 31 这一步），六张表出生即空 → 生产库里**不可能**存在 `accepted`/`rejected`/`failed` 三个字符串；删枚举量也不是 schema 变更（`state` 是 TEXT 列）。留一步改不到任何行的迁移就是死代码（判断依据见下方「没有数据迁移」） |
| D23 | `get_ai_light_sessions_by_state()` 保留，只改测试里的引用 | 它是通用查询，与终态是否存在无关 |

**没有数据迁移（30 → 31 是 30 之后的唯一步）**

本轮最初写过一个 `upgrade_4_t`（31 → 32：`UPDATE ai_light_session SET state = 'generating' WHERE state IN ('accepted', 'rejected', 'failed');`），**现已整体删除**。撤销的理由是它一行都改不到：

| 项 | 内容 |
|---|---|
| 生产目标库的版本 | **30**，而 30 之后的升级**从未推送过** —— 生产库里根本没有 `ai_light_*` 表 |
| 那六张表从哪来 | 唯一的建表者是 `upgrade_3_t`（30 → 31）里的 `sync_schema()`，所以表**出生即空** |
| 因此 | 任何受支持的升级路径上都不存在 `accepted` / `rejected` / `failed` 的行 → 那条 UPDATE 是**死代码** |
| 删枚举量本身 | 不是 schema 变更：`state` 是以 magic_enum 枚举名存的 **TEXT 列**，列的定义、类型、约束都没动 |
| 版本号 | `g_current_version = 31`（`g_version_30` 保留作 `upgrade_3_t` 的门控；`g_version_29` 已随 29 → 30 那一步移除）；`upgrade_3_t` 是最后一步，所以它写 `g_current_version` |
| 升级列表 | `sqlite_database.cpp` 为 `upgrade_init, upgrade_3, upgrade_clear_orphan_task`（29 → 30 的 `upgrade_2()` 已随该步骤进入生产而移除） |
| 先例 | §15.4 坑 3：删掉「既是枚举值又是数据库列」的状态可以不动 `user_version`，但前提必须逐条核实（本例的前提更强：表都还不存在） |
| 代价 | 只影响**我们自己跑出来的开发机库**（见下方「保留的边界」） |

判断依据（**以后遇到同类问题照这个顺序问**）：① 这个枚举列在**生产库实际会出现的版本**上有没有行？② 那些行有没有可能存着被删掉的字符串？③ 删除动作本身改不改列定义？三条都答「没有 / 不改」才敢不迁移。坑 28 记的是「答错就要付 500」的代价，不是「必须迁移」。

**口径差（文档原文 vs 实现，以代码为准）**

| 项 | 原文 | 实现 | 为什么 |
|---|---|---|---|
| 会话状态数 | §15.4 的状态机表写 7 个状态、`accepted`/`rejected` 是终态 | **4 个状态，无终态**；三个决定都 → `generating` | 本轮需求；§13.1 的状态机表已同步 |
| `is_selected_` 与 `is_accepted_` | §13.1 取舍第 3 条：「是两个语义，**不合并**」 | 仍然不合并，但**定稿也置 `is_selected_`** | 两个字段含义没变，只是现在可以在同一行上同时为真（D20） |
| `failed` 的用途 | §15.4 / 接缝契约：「`failed` 保留给不可恢复错误」 | `failed` **已取消**；`failure_reason_` 是失败的唯一记录 | 没有任何代码路径写过 `failed`，保留一个没人写的终态只会让人以为失败会被标记 |
| 「定稿后不能删会话」 | `ai_light.cpp:313-318` 的 400 + 前端文档 §5.4 | 闸门删除 | D21 |
| 前端「终态判定」段 | 前端文档 §4.2：「任何写动作都会 400 `会话 {} 已处于终态 {}`」 | 整段删除；不存在「会话已结束所以只读」的分支 | 同上 |
| 迁移 31 → 32 | 本节初稿：`upgrade_4_t` 改写存量终态 | **整步撤销，没有数据迁移** | 30 之后的升级从未推送 → 生产库里没有 `ai_light_*` 表，表由 30 → 31 新建、出生即空（见上节） |

**服务端钉死 vs 模型可覆盖**

| 字段 | 谁定 |
|---|---|
| 判完之后会话进入哪个状态 | **服务端**：只有 `generating` 一个出口（模型/前端都没有发言权） |
| 「当前定稿」是哪一个候选 | **服务端**：库层 `accepted_session_id` 上的唯一索引，写入只能走 `mark_accepted()` |
| 旧状态字符串的命运 | **不存在**：生产库里不可能出现它们（表由 30 → 31 新建且出生即空），所以没有迁移、也没有客户端参与 |
| 三个决定的**区别** | **服务端**：定稿必须带 `preview_file`、续轮必须有 `selected_candidate_id`、全否必须没有 —— 三条都是 `apply_judgement` 入口的硬校验 |

**测试**

| 文件 / 用例 | 钉什么 |
|---|---|
| `ai_light_state/judge_accept_marks_candidate_and_session`（改） | 定稿后会话是 `generating`（**不是终态**）；定稿候选 `is_selected_` **也为真**（D20）；`get_ai_light_selected_candidate(session, 1)` 返回的就是定稿候选（下一轮 warm start 的起点） |
| `ai_light_state/judge_continue_keeps_session_iterable`（改） | 删掉 `!is_terminal(...)`（函数已不存在），其余不变：回到 `generating`、候选被标 `is_selected_`、不产生 `preview_file` |
| `ai_light_state/judge_reject_all_keeps_session_iterable`（原 `..._terminates_session`，改名 + 改断言） | 全否后是 `generating` 且**不是**「等人工」；仍然不选任何候选；轮次仍是 `judged` |
| **新增** `ai_light_state/judge_accept_moves_the_accepted_pointer` | **D19 的守门人**：先造一个已经是「当前定稿」的旧候选（带自己的 `preview_file`），再对另一张候选走一次 accept → 旧候选的 `is_accepted_`/`accepted_session_id_`/`preview_file_id_` 全被清、`is_accepted_candidate()` 变 false，但**它的 `preview_file` 行仍在**；新候选成为唯一「当前定稿」；不抛唯一约束异常 |
| `ai_light_state/judge_on_terminal_session_is_rejected` | **整段删除** —— 它断言的前提（会话可以是终态）已不存在 |
| `ai_light_state/overdue_session_stays_awaiting_judgement_and_stays_judgeable`（改） | 超时判断后状态由 `accepted` 变 `generating`；其余（超时不剥夺判断能力）不变 |
| `ai_light_schema/*`（改） | `get_ai_light_sessions_by_state(accepted)` → `generating`；版本断言**保持 31**（`upgrade_30_to_31_creates_ai_light_tables` 从 30 出发、终点 31；`upgrade_gating_skips_ai_light_when_already_latest` 从 31 出发仍停 31 —— 后者就是门控本身） |
| `sqlite_upgrade/upgrade_31_to_32_rewrites_legacy_terminal_states` | **整段删除** —— 它钉的那一步（31 → 32 改写）已撤销；该文件里四处 `== 32` 的版本断言一并改回 `31`，随它引入的 `#include <doodle_core/metadata/ai_light/session.h>` 也一并去掉 |

**本轮的坑（承 §15.6 的 12 条、§15.7 的 13–21、§15.8 的 22–24、§15.9 的 25–27，续 28–30）**

28. **ORM 的枚举读出器对不认识的字符串是「抛」，不是「回落」。** `sqlite_statement.h:279-287`：
    空串 → `static_cast<T>(0)`，**非空但不认识 → `throw std::runtime_error("Failed to parse enum value from
    string: ...")`**。所以库里只要留着被删掉的枚举名，那些行此后每次读都是 500，而且自己不会好。
    一般教训：**判断「删枚举量要不要配迁移」的依据是「生产库实际会不会出现那些行」，而不是「删了枚举量」
    这个动作本身** —— 本轮前者答「不会」（表由 30 → 31 新建且出生即空），所以整个迁移被撤销。
29. **同一个「删枚举量」动作，DB 侧炸、JSON 侧静默。** DB 侧见坑 28；而 JSON 侧
    `NLOHMANN_JSON_SERIALIZE_ENUM::from_json` 遇到不认识的字符串会**回落到第一个枚举量**
    （`nlohmann/detail/macro_scope.hpp:247`）—— 把 `accepted` 静默读成 `draft`。两侧行为相反，
    必须分别判断：DB 侧靠迁移，JSON 侧只能靠「这些字段不来自请求体」（`ai_light_session_state` 是
    服务端所有，前端不回传），并把它记在这里当已知边界。
30. **版本常量陷阱会复现，而且是无声的；正确写法取决于「它是不是最后一步」。** 规则：**非最后一步写自己的
    目标版本常量，只有最后一步和 `upgrade_init_t` 写 `g_current_version`。**
    本轮先踩到、后拆掉：曾把 `g_current_version` 提到 32 并新增 `upgrade_4_t`，于是 `upgrade_3_t` 里那句
    `user_version(g_current_version)` 会让**经 30→31 走过的库在同一次启动里被 stamp 成 32**，
    `upgrade_4_t` 的 `> 31` 判定成立、**静默跳过**数据改写 —— 版本号看起来完全正确，数据却还是坏的。
    随后整个 31 → 32 被撤销（30 之后的升级从未推送，D22），`g_current_version` 回到 31、`upgrade_3_t`
    重新成为最后一步，于是它写 `g_current_version` 又变回正确写法。
    **所以增减升级步骤时必须回头检查这一行** —— 它与 §15.3 顶部记的 29→30 那次是同一个错误模式。

**保留的边界**：M2 之前 worker 侧还没有 `ai_light` 派发注册（`http_client/work.cpp` 的派发表、
`allowed_task_types_`），所以「定稿 → 继续 → 再定稿」的完整链只在**单测**里跑通，
真机上还没有一轮渲染回环；前端页面同样尚未实现（本轮只同步了交接文档）。
`POST /iterate` 仍只接受 `generating`，一轮只能判断一次 —— 「同一张候选反复判断」不在本轮范围内
（要重复调整就先出一轮，这也让每一轮的 6 张图有明确的出处）。

**另一条边界：开发机上的库要自己处理。** 我们自己跑开发/测试时产生过的库可能停在 31（或更高）并且含
`accepted` / `rejected` / `failed` 行。它们不是生产目标，所以代码侧不做兼容（`upgrade_3_t` 的 `> 30`
门控会把它们整个跳过），读那些行会抛枚举解析异常（坑 28）。一次性处理办法：手工执行
`UPDATE ai_light_session SET state = 'generating' WHERE state IN ('accepted', 'rejected', 'failed');`，
或者直接删掉那个本地库。

**回归**：全量 `test_main` **196 用例 / 1657 断言全通过、exit 0**。本轮先后落过三次：§15.10 的三终态改动
（当时 198 / 1671）→ 撤销 31 → 32 那一步并删掉它的迁移用例（197 / 1663）→ 移除已进生产的 29 → 30
与它的用例（**196 / 1657**；§15.9 末尾的基线是 197 / 1660）。本轮相关套件：
`ai_light_schema` 6（126 断言）· `ai_light_state` **18（174）** · `ai_light_stats` 22（184）·
`ai_light_spec_prompt` 11（117）· `ai_light_candidates` 15（214）· `ai_light_feedback` 11（84）·
`sqlite_upgrade` **3（21）**。
`ai_light_state` 的用例数没变（删掉 `judge_on_terminal_session_is_rejected`、补上
`judge_accept_moves_the_accepted_pointer`），断言数由 171 增到 174；`sqlite_upgrade` 由 5 例 35 断言降到
3 例 21 断言 —— `upgrade_31_to_32_rewrites_legacy_terminal_states` 整段删除（8 条断言），
`upgrade_29_to_30_adds_te_xie_column` 整段删除、`upgrade_brings_older_db_up_to_current` 改写成只钉
「从 v26 出发也会被带到 31 且建出 `ai_light_session`」（删掉 te_xie 与探针行的断言，顺带删掉
`drop_te_xie` / `insert_probe_extend` / `probe_te_xie` 三个只服务于它的助手）。

### 15.11 已落地（编排层重构 + 统一 JSON 架构验证层）

**需求（用户原话，m02972）**：「整理并重构 `src/doodle_lib/http_method/kitsu/ai_light_orchestrator.cpp`
`src/doodle_lib/http_method/kitsu/ai_light_orchestrator.h` 中的类和方法, 尤其是对模型推理后的 json 架构验证」。

一句话：**行为一字不改**，把当时的单文件（`ai_light_orchestrator.cpp` 约 2700 行 / `.h` 420 行）拆成
「状态机 + 三条接缝 + 校验层 + rig 运算」，并把散落在三处的 JSON 读取器合并成一层；在此之上给模型输出
接上 `json-schema-validator` 做**诊断**（决策 D-1，不做准入）。完整契约（诊断表 / 逐符号搬迁表 / 批次 0–7）
在 `build/refactor_plan_v3.md`。

**目标与动机（为什么要拆）**

| 动机 | 具体症状 |
|---|---|
| 单文件过大 | 2734 行里同时有状态机、三条接缝的提示词/schema/解析、rig 运算、推理管道；改接缝 3 要滚过接缝 1 的 600 行 |
| 读取器同形不同义 | `require_*`（抛）与 `opt_*`（返回 `optional` / 空容器）写法相似，且严格读取器的错误文本**硬编码**写成「候选基线 rig 的字段 …」—— 接缝 1/3 复用它时报错会指向错误的字段（本轮的 F1） |
| 契约无处安放 | 三条接缝的宽容度完全不同（见下表），却只能靠读实现来判断 |
| 无本地校验 | 模型输出是否合规只有 Ark 侧 `json_schema_strict=true` 的承诺；接缝自己读到第一个坏字段就抛，**看不到全部违规** |

**新文件结构**（`src/doodle_lib/http_method/kitsu/`，行数 = 本轮落地时）

| 文件 | 行数 | 职责 | 关键符号 |
|---|---|---|---|
| `ai_light_orchestrator.h` | 135 | **伞形头**：include 下面五个头 + 只声明状态机 4 个函数 | `advance_session` / `apply_judgement` / `mark_iteration_rendered` / `build_session_detail` |
| `ai_light_orchestrator.cpp` | 752 | **只剩状态机**：入口/回滚/两个分支协程 + 上面 4 个函数 | `advance_session_spec_branch` `:139` · `advance_session_generating_branch` `:202` · `advance_session` `:395` |
| `ai_light_json_validation.{h,cpp}` | 213 / 416 | **统一 JSON 层**：架构建造器 + 严格读取器 + 宽松读取器 + 诊断（②自检 / ③模型输出） | `schema_*` / `require_*` / `opt_*` / `validate_schema_is_wellformed` `:360` / `validate_model_output` `:390` / `join_violations` `:404` |
| `ai_light_rig_ops.{h,cpp}` | 63 / 228 | rig 的纯运算（不碰 IO） | `key_fill_ratio_of` `:30` · `apply_axis_step` `:51` · `expand_candidates` `:138` · `diff_rigs` `:168`；`g_axis_step_budget` / `g_axis_max_count`（在头里） |
| `ai_light_seam_common.{h,cpp}` | 152 / 310 | **三条接缝共用**：文本投影 + 布局助手 + **推理管道** | `first_light_with_role` `:156` · `join_strings` `:162` · `join_numbers` `:231` · `rgb_to_hex` `:171` · `describe_targets` `:179` · `describe_rig_brief` `:240` · `describe_spec_brief` `:264` · `describe_preset_brief` `:301` · `build_common_lighting_section` `:202` · `g_axis_value_semantics_text`（在头里）· `seam_request_base_t` `:111` · `seam_inference_result_t` `:130` · `run_seam_inference` `:146` |
| `ai_light_seam_spec.{h,cpp}` | 107 / 605 | 接缝 1（宽松） | `build_spec_*` · `put_evidence` `:104` · `map_spec_result` `:361` · `generate_spec` `:565` |
| `ai_light_seam_candidates.{h,cpp}` | 134 / 572 | 接缝 2（严格） | `pick_presets` `:113` · `parse_axis_plan` `:143` · `build_candidates_*` · `default_axis_plan` `:344` · `build_base_rig` `:368` · `map_candidate_result` `:454` · `generate_candidates` `:536` |
| `ai_light_seam_feedback.{h,cpp}` | 105 / 352 | 接缝 3（混合） | `build_feedback_*` · `map_feedback_result` `:244` · `structure_feedback` `:310` |

**依赖方向（单向，无环）**：`ai_light_json_validation` 只依赖 `nlohmann` 与 `doodle_core` 异常（**公共头不含
`<nlohmann/json-schema.hpp>`** —— 校验库只出现在 `.cpp`）→ `ai_light_seam_common`（公共头**不含 HTTP**，推理
客户端只在 `.cpp` 里拉）→ `ai_light_rig_ops`（借 `first_light_with_role`）→ 三条接缝 → 状态机。伞形头是唯一
对外入口：调用方（`ai_light.cpp` + 4 个测试）只需 `#include "ai_light_orchestrator.h"`。

**搬迁映射（原落点 → 现落点）**

| 内容 | 原落点（拆分前） | 现落点 |
|---|---|---|
| 状态机 4 函数 + 回滚 lambda | `ai_light_orchestrator.cpp` 随处 | 同文件 `:118-752`（拆出两个分支协程，见 D-7） |
| 接缝 1 全部（4 个 `build_spec_*` + `map_spec_result` + `generate_spec` + 白名单 + `g_evidence_paths` + `put_evidence`） | `:717-1221` + `:1703-1728` | `ai_light_seam_spec.{h,cpp}` |
| 接缝 2 全部（`build_candidates_*` + `default_axis_plan` + `build_base_rig` + `parse_axis_plan` + `pick_presets` + `rig_substance` + `map_candidate_result` + `generate_candidates`） | `:1223-1698` + `:1730-1752` | `ai_light_seam_candidates.{h,cpp}` |
| 接缝 3 全部（4 个 `build_feedback_*` + `map_feedback_result` + `structure_feedback` + `trim_number` + 两个上限常量） | `:1754-2032` | `ai_light_seam_feedback.{h,cpp}` |
| `key_fill_ratio_of` / `apply_axis_step` / `expand_candidates` / `diff_rigs` + 两个轴常量 | `:1787-1988` + `:1302-1307` | `ai_light_rig_ops.{h,cpp}` |
| `join_strings` / `join_numbers` / `rgb_to_hex` / `describe_targets` / `describe_rig_brief` / `describe_spec_brief` / `describe_preset_brief` / `build_common_lighting_section` / `g_axis_value_semantics_text` | 分散在 `:761-841` / `:1310-1459` | `ai_light_seam_common.{h,cpp}` |
| `resolved_inference_t` / `make_seam_client` / `chick_inference_ok` + 三段 `generate_*` 的样板 | 匿名 ns `:1706-1782` + 三处各自复制 | `ai_light_seam_common.{h,cpp}` 的 `run_seam_inference()`（一处实现） |
| `require_*` / `opt_*` / `is_one_of` / `make_*_schema` + 17 键 `g_evidence_paths` 之外的校验原语 | 三处各写一份 | `ai_light_json_validation.{h,cpp}` |
| 三条请求结构体的公共三字段 | 三份重复（各自 6/4/3 行文档） | `seam_request_base_t`（`ai_light_seam_common.h:111`） |

**三条策略必须保持不同的不变式（这是拆分的验收标准之一：拆完仍必须不同）**

| 接缝 | 策略 | 缺字段 / 类型不符 | 语义无效 |
| --- | --- | --- | --- |
| spec（`map_spec_result`） | 宽松 | 保留统计默认值 + `put_evidence(default_value)` 留痕 | 自由文本枚举走白名单，不在白名单则弃用该字段 |
| candidates（`build_base_rig` / `map_candidate_result`） | 严格 | 抛 502 | 轴计划非法 → **整体回退** `default_axis_plan()`，原因写进 `strategy_` |
| feedback（`map_feedback_result`） | 混合 | 抛 502（形状） | 未知轴 / `none` / 0 / 非法比值 / 重复 → 静默丢弃，首个胜 |

**实现决定（承 §15.7 的 D1–D5、§15.8 的 D6–D8、§15.9 的 D9–D16、§15.10 的 D17–D23，续 D-1 ~ D-7）**

| # | 决定 | 理由 |
|---|---|---|
| D-1 | 引入 `json-schema-validator`（vcpkg port 2.4.0，MIT）**只做诊断**：②我们自己的 schema 是不是合法 draft-7、③模型输出对 schema 的全部违规；**不把违规升级为拒绝** | ①Ark 侧 `json_schema_strict=true` 已经保证合规输出；②spec 接缝**有意**容忍截断/不完整输出（靠默认值 + `evidence.source = default_value` 留痕），一道 schema 闸门会直接破坏这个设计；③`additionalProperties:false` 若当闸门，会把今天被静默忽略的「模型多回一个无关键」变成整轮 502。**升级路径**：真机日志证明模型从不违规后，某个接缝提升为 502 是**一行改动**（`validate_model_output` 非空时抛） |
| D-2 | 每次调用**本地构造** validator（不共享、不加锁），schema 直接复用 `build_*_request()` 已建好的 `json_schema_` | 库 README 明确「校验是 const 且线程安全，但 validator 构造不是」；本地构造最简单，成本相对一次 LLM 调用可忽略；复用字段 = 诊断的 schema 与真正发出去的那份**必然同源** |
| D-3 | 不给 schema 加 `default`，且**不使用 `validate()` 的返回值**（只调带 `error_handler` 的重载） | 库的 default-value 处理会返回 json patch，`$ref` 内出现 `default` 时会破坏 draft-7 语义；不取返回值 = 不可能被 patch 影响 |
| D-4 | 三条接缝在拿到 `result_json_`、进 `map_*` **之前**调 `validate_model_output(...)`，用 `SPDLOG_LOGGER_WARN` 记**前 3 条**（`join_violations` 截断并标注省略数），带接缝名与轮次 | 控制流零变化；收益是「严格接缝今天读到第一个坏字段就抛，现在日志里能看到全部违规」 |
| D-5 | 三条接缝的请求结构体上提 `seam_request_base_t{session_id_, project_id_, operator_id_}`，`run_seam_inference` 只收一个信封 | 管道真正要的就是这三样。**加基类是安全的**：全仓库对三个结构体只有「值初始化 + 逐字段赋值」，没有任何聚合初始化 |
| D-6 | 推理样板收口成 `run_seam_inference(sqlite_database&, const seam_request_base_t&, seam, text_prompt, const http::seedance2::inference_request_t&, const uuid& iteration_id = {})`，返回**新的** `seam_inference_result_t{json_, input_tokens_, output_tokens_, total_tokens_}`，**不是** `run_task_result_t` | 信念封装的刻意偏离（plan §4 原签名返回 `run_task_result_t`）：`chick_inference_ok` 已经把 `status_`/`error_code_`/`is_timeout_` 消费成 `doodle_error`，调用方拿不到也不该关心它们。字段名与 `run_task_result_t` 的同名成员保持一致，好让三条接缝的日志语句**逐字不变**（只把 `result_json_` 换成 `json_`）。`iteration_id` 为 nil 表示这次推理不属于任何轮次（蒸馏阶段） |
| D-7 | `advance_session` 拆出 `advance_session_spec_branch` / `advance_session_generating_branch` 两个协程，**两个分支都不捕获异常** | 两条分支各自是「一次接缝调用 + 一个事务」的完整闭环，彼此无共享状态（只有 `io_session` 是共享的）。回滚/重抛仍由 `advance_session` 统一负责（接缝契约 1），失败路径只有一份实现，也不可能出现「分支里吞掉异常」的岔路。**判定顺序与状态转移一字未改** |

**服务端钉死 vs 模型可覆盖（本轮新增的行）**

| 项 | 谁定 |
|---|---|
| 诊断结果是否影响本轮 | **服务端**：只看日志，控制流不变（D-1/D-4）；升级为 502 是将来的一次独立决策 |
| schema 形状 | **服务端**：三个 schema 由 `ai_light_seam_*` 唯一构造，并有 shape digest golden 钉住（见下表） |
| 模型输出违规时读哪几条 | **服务端**：前 3 条 + 省略数 |

**测试**

| 文件 / 用例 | 钉什么 |
|---|---|
| **新建** `src/test/core/ai_light_json_validation.cpp`（套件 `ai_light_json_validation`，9 用例 / 97 断言，全离线） | 建造器形状（`required` 自动生成 / 显式重载 / `schema_enum<E>` 剔除 `none`）；严格读取器的**接缝名 + 字段路径**措辞与 `bad_gateway`；`require_enum` 对未知字符串**必须抛**（钉住「绝不用裸 `get<>`」）；六条宽松读取器的缺失/错类型/正确三种输入；`check_schema_consistency` 对三个真实 schema 通过、对故意造的三类违规报错；三个真实 schema 经库判定为合法 draft-7；`validate_model_output` 的违规串是 `root.lights[0].softness` 形状的 json pointer |
| **`schema_shape_digest_matches_frozen_golden`**（同上文件） | 把每个节点的 `path/type/required/additionalProperties/enum/min/max/minItems/maxItems` 摊平成排序后的多行字符串做 FNV-1a 摘要，与三份内嵌 golden 逐字比较 —— **spec `c4b1d8ee3aa3ef47` / candidates `227cb12da4693db1` / feedback `0e13ecb11cd3f474`**。批次 2（改用建造器重写三个 `build_*_json_schema`）后三份摘要**逐字不变**，这是「schema 结构等价」的最强证据 |
| `ai_light_candidates/build_base_rig_requires_rgb_when_mode_is_rgb`（新增 1 用例 → 16 用例 / 223 断言） | F2/F3：`build_base_rig` 按 `color_mode` 路由，`rgb` 模式缺 `rgb` 必须抛（**不再**只在 `temperature` 分支里软处理） |
| 其余五个 `ai_light_*` 套件 | **一行不改**（用例数/断言数与重构前逐个相同）—— 这正是「行为未变」的证明 |

**新坑（承 §15.6 的 12 条、§15.7 的 13–21、§15.8 的 22–24、§15.9 的 25–27、§15.10 的 28–30，续 31–35）**

31. **`using namespace orm;` 不是可选的，而漏掉它的报错会指向网络 API。** 搬走 `make_seam_client` 时漏了
    `ai_light_orchestrator.cpp:71` 那行文件级 `using namespace orm;`，于是 `select(...)` 被解析成
    **winsock2.h:2060 的 `select`** → `error C2660: 'select': function does not take 1 arguments`，
    `object` 直接 `C3861: identifier not found`。`select_t` / `object` 的定义在
    `sqlite_orm/orm/select.h`（`namespace doodle::orm`），**与 `sqlite_database.h` 的 include 无关**。
    教训：搬走一段代码时，**文件级 `using` / 文件级常量也要跟着看一遍**；一个符号名与系统头撞车时，
    编译器不会提示「你少了个 using」。
32. **匿名命名空间 vs 导出。** 模板（`require_enum<E>` / `schema_enum<E>`）**必须 inline 在头里**（模板无法导出）；
    非模板定义必须在**非匿名**命名空间且带 `DOODLELIB_API`，否则库外 LNK2019。公共头也**不得** include
    `<nlohmann/json-schema.hpp>`：校验库只出现在 `.cpp`，公共 API 继续只暴露 `nlohmann::json`。
33. **模板不能只声明。** `schema_enum<E>` 这类只有声明没有定义时，链接期才炸（LNK2019），而且**只在被使用时**
    才出现 —— 看起来像「某个不相关的文件坏了」。
34. **`schema_object` 的 `required` 自动生成假定「满配」。** 三个 schema 的 16 个 object 恰好都是
    「`additionalProperties:false` + `required` 覆盖全部属性」（34 处命中已逐条核对），所以自动生成与现状等价；
    将来要「部分必填」必须改用显式 `required` 的重载，**不能**先加属性再期待它自动变成可选。
35. **诊断校验不得做准入，`validate_model_output` 永不抛。** 它把库异常也转成一条字符串；spec 接缝本就允许
    截断输出（默认值 + `evidence.source = default_value` 留痕），一道闸门会直接破坏该设计。若将来要提升为
    502，必须同时重看 §15.7 的宽容策略。

**施工/工具坑（非产品行为，写给下一次搬文件的人）**

- 搬运用「按**原始行号**单遍删除 + 先取原文再组装」，不要边删边算新行号；区间里若含「还要放进另一个新文件」
  的助手（本轮 `first_light_with_role`），取区间时就要把它排除，否则 LNK2005 重复定义。
- 模板正文用 `write` 写成带 `//__BODY__` 占位符的文件，再用 PowerShell `Get-Content` 取原文 `.Replace()` 回填
  —— 避免在命令行里嵌中文与整段 C++。
- **`edit` 工具在 PowerShell 改写过同一文件后会报 `cannot edit ...: file changed since it was read`**，重新 read 即可。
- 每次链接前先删 `build/Ninja_debug/bin/test_main.exe`（被占用时 `LINK : fatal error LNK1168`）。
- 管道后的 exit code 不可信（§15.10 已记）：必须 grep `error C[0-9]+|error LNK|FAILED:|ninja: build stopped`。

**回归**

- 全量 `test_main`：**218 用例 / 1781 断言全通过、exit 0**。与 plan §8 记的旧快照 196 / 1657 的差额 =
  本轮新增 9 例/97 断言（`ai_light_json_validation`）+ 1 例/9 断言（`ai_light_candidates` 的 F2/F3 用例）
  + 12 例/18 断言（`coro_test_support`，本轮未改动，只是不在那份旧快照里）。
- 重构期间**每个批次**都构建 + 跑受影响套件；接缝拆分前后 `--run_test=ai_light_*` **恒为 101 用例 / 1028 断言**，
  逐套件相同（拆分前 = 拆分后）。
- 重构后逐套件：`ai_light_schema` 6(126) · `ai_light_state` 18(174) · `ai_light_stats` 22(184) ·
  `ai_light_spec_prompt` 11(117) · `ai_light_candidates` **16(223)** · `ai_light_feedback` 11(84) ·
  `ai_light_json_validation` **9(97)** · `ai_light_paths` 8(23)。

**保留的边界**：`build/refactor_plan_v3.md` 是本轮契约（可留档可删）；`ai_light_orchestrator.{h,cpp}.bak2`
是拆分前备份（可留档可删）；本轮**没有** `git commit`（由用户自己提交）。前端文档只引 HTTP 层，
`grep -n 'ai_light_orchestrator' docs/ue_ai_lighting_frontend.md` 命中的两处（`:8` 契约来源、`:549` 接口契约细节）
描述的是「服务端代码是权威」，拆分后依然成立，故未改。

### 15.12 已落地（派发链路：服务端 → 工作机 → UE commandlet → 回传 —— 服务端 + 工作机两段）

**需求（用户原话，m04090）**：「安排 ai_light 工作的派发功能」。

一句话：「一轮 6 个候选」不再只是库里的一张表，而是**真的被派到一台工作机上去渲染、并把结果写回库**，
让会话沿既有的 `generating → awaiting_judgement` 走完闭环（§10.4 的实现）。本轮落地的是**服务端 + 工作机**
两段；UE 侧的 `DoodleLighting` commandlet 是同一批计划的下一段（见本节末尾「未落地」）。

**链路**

```
POST /api/doodle/ai/light/session/{id}/iterate
  └─ advance_session（generating 分支，ai_light_orchestrator.cpp:202）
       ├─ generate_candidates(...)                     ← §15.8：6 个 rig + 候选行的产物路径
       ├─ shot_render_light(project_id, task_id_)      ← 基线关卡 / .uproject / 相机文件（§7.1）
       ├─ ai_light_task_payload_t → server_task_info.command_（type_ = ai_light）
       └─ run_next_task() + broadcast(server-task-info:new)
                    ↓ 工作机 run_next_task（客户端报在线 + 无 running 任务）
http_work::run_task（work.cpp:237）→ ai_light_round_distributed::run()
       ├─ UnrealEditor-Cmd.exe <uproject> … -run=DoodleLighting -Params=<tmp>.json
       ├─ 读 <out_dir>/render_report.json
       ├─ POST /api/doodle/ai/light/render_report/{task_id}   （回传；不带任何路径）
       └─ put_job_info（终态）→ 析构时 task_finished() 归还机器
                    ↓
render_report_route → parse_round_render_report → apply_render_report
       ├─ 至少一张 rendered → mark_iteration_rendered（轮次 awaiting_judgement + 会话 await）
       └─ error_ 非空或无一张 rendered → mark_iteration_failed（只写会话 failure_reason_）
```

**载荷契约**（`src/doodle_core/metadata/ai_light/task_payload.h`，JSON 键**冻结**）

| 结构 | JSON 键 |
|---|---|
| `ai_light_task_payload_t`（`:73`） | `session_id` · `iteration` · `spec_id` · `out_dir` · `baseline_level` · `ue_project_path` · `camera_file_path` · `candidates[]` |
| `ai_light_task_candidate_t`（`:44`） | `candidate_index` · `candidate_id` · `png_path` · `exr_path` · `rig`（完整 `lighting_rig_t`，UE 侧照它 apply_ops） |
| `round_render_report_t`（`:143`） | `candidates[]` · `error`（**没有**成功标志位：一轮的结果就是这 N 条 + 一句错误文本） |
| `candidate_render_report_t`（`:114`） | `candidate_index` · `candidate_id` · `status` · `render_ms` · `light_count` · `metrics`（**不含路径**，D27） |

报告文件名常量：`g_ai_light_render_report_file_name{"render_report.json"}`（`:37`）；工作机在起 UE **之前**先删掉
`<out_dir>/render_report.json` 的残留，否则 commandlet 崩在写报告之前时会把上一轮的报告当成本轮结果回传。

**严格 / 宽松分界**（`parse_round_render_report`，`round_render_report` 接缝名 `ai_light_orchestrator.cpp:756`）

| 字段 | 策略 |
|---|---|
| `candidates`（数组本身） · 每个元素的 `candidate_id` · `status` | **严格**：缺失/类型不符/未知枚举串 → `bad_gateway`（`status` 走 `json_val::require_enum<candidate_render_status>`，钉死「不许 nlohmann 静默回落到 `pending`」；uuid 由 `json_at_path` + `get<uuid>()` 包 try/catch 翻成 502，因为校验层没有 uuid 读取器） |
| `candidate_index` · `render_ms` · `light_count` · `metrics` · 轮级 `error` | **宽松**：缺失取默认（下标 / 0 / 0 / 默认 `candidate_metrics_t` / 空串）；`metrics` 形状不符只丢这一个字段并 WARN，候选本身仍算 rendered |

**服务端落点**（行号是起点，定位靠函数名）

| 项 | 落点 |
|---|---|
| 载荷 / 回传类型 + 报告文件名 | `src/doodle_core/metadata/ai_light/task_payload.h`（`:37` / `:44` / `:73` / `:114` / `:143`）；登记 `doodle_core/metadata/ai_light/CMakeLists.txt` 与伞形头 `ai_light.h` |
| 基线关卡推导（D8 从「只透传」改成「真的去问」） | `ai_light_orchestrator.cpp:282-295`：`http::auto_task::shot_render_light(io_session.project_id_, io_session.task_id_)` → `l_req.baseline_level_ = l_assembly.original_map_.generic_string()` |
| 载荷组装 + 进 `command_` | 同文件 `:339-346`（`out_dir_` = `make_iteration_relative_dir(会话 uuid, 轮次 uuid)`；候选行不写路径列）· `:395 l_task.command_ = l_payload;` —— 载荷里放的是相对路径（`make_iteration_relative_dir` / `make_candidate_file_name`），见 §15.14 |
| 回传解析 | `parse_round_render_report` `:760` |
| 整轮失败落库 | `mark_iteration_failed` `:828`（原因经 `brief_text` 截断到 512 + `"...(已截断)"`，`brief_text` 在 `:85`） |
| 回传落库 | `apply_render_report(sql, ctx, task_id, report)`（两条出口；产物落点按 `candidate_render_path(ctx, …)` 现算，D41/D44） |
| 声明 | `ai_light_orchestrator.h:142` / `:155` / `:161`（都带 `DOODLELIB_API`） |
| 回传路由 | 声明 `ai_light.h:158`（`DOODLE_HTTP_JWT_FUN(render_report_route)`，**不做项目权限检查** —— 调用方是工作机的机器 JWT，与 `/api/data/jobs/{id}` 同一信任级）· 注册 `kitsu.cpp:333` `/api/doodle/ai/light/render_report/{}` · 实现 `ai_light.cpp:790`（先查任务类型必须是 `ai_light`，再解析、落库，回执带 `iteration_state` / `session_state` / `failure_reason`） |
| 客户端方法 | `kitsu_client.h:174` / `kitsu_client.cpp:694` `post_ai_light_render_report(uuid, const nlohmann::json&)`（接受 200/201；回执按非严格 JSON 解析 —— 回执解析失败不该让工作机把整轮判失败） |
| 顺手修 | `ai_light.h` 的 `iterate_route` 头注释删掉过时的「本轮返回 501（LLM 接缝未实现）」 |

**工作机落点**

| 项 | 落点 |
|---|---|
| 任务类 | `src/doodle_lib/exe_warp/ai_light_render.{h,cpp}`：`ai_light_round_distributed : base_distributed_task`，成员只有 `ai_light::ai_light_task_payload_t payload_` + `awaitable<void> run()`（与 `depth_estimation_distributed` 同形，`run()` 不是虚覆盖） |
| 载荷反序列化 | `payload_ = task_info_.command_.get<ai_light::ai_light_task_payload_t>();` + 三条硬前置（`baseline_level_` / `ue_project_path_` / `candidates_` 非空，否则 `throw_exception(doodle_error{...})`） |
| 起 UE | `async_run_ue({ue_project_path_, "-windowed", "-log", "-AllowStdOutLogVerbosity", "-ForceLogFlush", "-Unattended", "-run=DoodleLighting", fmt::format("-Params={}", tmp)}, logger, kitsu_client, /*create_lock=*/true, run_time_info)`（`create_lock=true`：UE 是整机独占资源，`async_run_ue` 内部拿全局 `ue_ctx` 队列闸） |
| 派发分支 | `src/doodle_lib/http_client/work.cpp:237`（`server_task_info_type::ai_light` → `make_shared` + `task_started()` + `co_spawn(..., consign(detached, l_run))` + `return true`） |
| 默认允许类型 | `src/doodle_lib/http_method/local/event.cpp:31-38` 加入 `ai_light`（不加的话 `--local` 的工作机拒绝这类任务，`run_next_task` 永远挑不到它） |
| 终态 | `run()` 返回前 `put_job_info`（§10.4 契约）；`task_finished()` 由 `~base_distributed_task`（`work.cpp:360`）在析构时归还机器 |
| CMake | `src/doodle_lib/exe_warp/CMakeLists.txt`（PUBLIC `ai_light_render.h` / PRIVATE `ai_light_render.cpp`） |

**实现决定（承 §15.7 的 D1–D5、§15.8 的 D6–D8、§15.9 的 D9–D16、§15.10 的 D17–D23、§15.11 的 D-1~D-7、§15.14 的 D38–D43，续 D24–D29 + D44）**

| # | 决定 | 理由 |
|---|---|---|
| D24 | 只走 `server_task_info.command_` **一条**载荷通道：不新增表、不给 `server_task_info` 加 ai_light 专用字段 | `server_task_info` 已经是派发的事实载体（`/api/data/jobs/{id}` 的机器 JWT、`run_next_task` 的在线/占用检查、`broadcast` 全是现成的）；加字段等于把同一份信息存两处，而两边会漂移 |
| D25 | 产物命名**只在服务端产生**（`kitsu_ctx_t` 按会话/轮次/候选 uuid），载荷里的 `png_path_`/`exr_path_` 只是**告知**且是相对路径（根由工作机定，见 D39/D40），回传报文里根本不含路径 | UE 侧不需要知道目录布局；工作机也不该有权决定服务端把图记在哪。候选行也不存路径：「服务端把图记在哪」由命名规则唯一决定（D44） |
| D26 | 整轮失败用**轮级** `error_`，候选级失败用每条 `status=failed` | §7.5 第 9 条：单候选失败不能让整轮失败。两条通道语义不同，不能混用一个标志位 |
| D27 | 回传是**不可信输入**：`candidate_id` 必须属于本轮（只查本轮），路径不接受 | 一个坏回传不能改到别人的候选行上 |
| D28 | 失败**不是终态**：`mark_iteration_failed` 只写会话 `failure_reason_`，不动轮次状态、不动候选行 | 与 §15.10（会话没有终态）一致；美术再点一次 `/iterate` 就该能重试 |
| D29 | UE 侧只认一个 JSON 入口：`-Params=<file>`，**不新增命令行 key** | 与 §13.4 的建议一致；`DoodleAutoAnimationCommandlet` 的 `Main()` 已经只有 `Params` / `ImportRig` 两个 key，命令行的形状越少越好 |

**本轮的坑（承 §15.6 的 12 条 … §15.11 的 31–35，续 36–40）**

36. **`ai_light_candidate::metrics_` 是 JSON 列，不是 `candidate_metrics_t`。** 读库行写 `l_cand.metrics_.contrast_` 会得到
    `error C2039: 'contrast_': is not a member of nlohmann::json`；必须 `l_cand.get_metrics().contrast_`（访问器
    `candidate.h:213-214`）。`candidate_render_report_t::metrics_` 才是 typed —— **同一份数据在「报文里」和「库里」
    是两种类型**，这是本轮最容易写错的一行。
37. **用例体里一个 `co_await` 都没有时，`DOODLE_FIXTURE_TEST_CASE_CORO` 展开出的函数不是协程。**
    `src/test/test_coro.h:253-259` 把它展开成 `static awaitable<void> body(fixture&)`，没有 `co_await` 就没有
    「协程魔法」，于是「返回类型不是 void 却没有 return」→ `error C4716: '...': must return a value`（8 个纯同步
    用例一起报）。纯同步用例改用 `BOOST_FIXTURE_TEST_CASE(name, fixture_t) { auto& l_fx = *this; ... }`。
38. **`task_payload.h` 的四个结构体全在 `doodle::ai_light`，而工作机代码在 `doodle::http`。** 漏限定会得到
    `error C2065: 'round_render_report_t': undeclared identifier` + 一串 `C2672 json::get` / `C7595 fmt::fstring`
    级联（错误都指向使用处，根因在类型名上）。
39. **`run_ue_assembly_arg` 在 `doodle::import_and_render_ue_ns`，不在 `doodle::http::auto_task`。**
    写成 `http::auto_task::run_ue_assembly_arg` 会得到 `error C2039: ... is not a member of 'doodle::http::auto_task'`
    + 一串 `error C2065 'l_assembly': undeclared identifier`。函数 `shot_render_light()` 在 `doodle::http::auto_task`，
    **返回类型在另一个命名空间** —— 用 `auto` 或写全限定都能避开。
40. **`test_main` 里 `g_ctx()` 不可用（悬垂指针），所以「成功路径依赖全局上下文」的代码等于不可测。**
    `apply_render_report` 初版用 `g_ctx().get<http::kitsu_ctx_t>()` 重算产物路径，于是唯一的成功路径只有真机能跑，
    而它恰恰是最容易出错的一段。改成读候选行里已存的路径后，单测能覆盖到转发/落库/状态流转的全部入口
    （语义等价：路径在派发那一轮就写好了，回传也不带路径）。先例见 `src/test/core/ai_light_paths.cpp:7-10`。

**测试**（新建 `src/test/core/ai_light_dispatch.cpp`，套件 `ai_light_dispatch`，**18 用例 / 66 断言**）

| 组 | 用例 | 钉什么 |
|---|---|---|
| 解析 | `parse_full_round_keeps_every_field` · `parse_defaults_missing_loose_fields` · `parse_requires_candidates_array` · `parse_rejects_unknown_status` · `parse_rejects_bad_candidate_uuid` · `parse_rejects_non_object_candidate` · `parse_keeps_round_error_text` · `parse_drops_broken_metrics_only` | 严格/宽松分界逐条（含 `status="succeeded"` 必须 502 —— 钉住「不许静默回落」；`error` + 空 `candidates` 是合法报文） |
| 落库 | `apply_report_advances_iteration_and_session` · `apply_report_keeps_paths_derivable_from_the_row` · `apply_report_partial_failure_still_advances` · `apply_report_error_without_rendered_writes_failure_reason` · `apply_report_all_failed_writes_failure_reason` | 轮次 `awaiting_judgement` + `render_ms_total_` 求和 + 会话 `awaiting_judgement`；**候选级失败不推翻整轮**，部分结果照落（失败那张仍是 `failed`，不伪造成出图）；轮级 `error_` 才是整轮失败（状态原地不动、只写 `failure_reason_`） |
| 归属/边界 | `apply_report_rejects_unknown_task`（404） · `apply_report_rejects_candidate_from_another_round`（400） · `apply_report_rejects_judged_iteration`（400） · `mark_iteration_failed_truncates_long_reason`（512 + `"...(已截断)"`） · `mark_iteration_failed_unknown_task_is_404` | 回传只认本轮、只认未判断的轮次（D27）；失败原因有长度上限 |

**回归**：全量 `test_main` **236 用例 / 1847 断言全通过、exit 0**（§15.11 末尾的基线 218 / 1781 —— 用同一配对口径
统计，差额 = 新增 `ai_light_dispatch` 18 用例 / 66 断言）。本轮 `--run_test=ai_light_*` **119 用例 / 1094 断言**
（上一轮 101 / 1028）。本轮 RFC 相关四个改动面都构建过：`doodle_lib.dll` + `test_main.exe` 链接通过。

**保留的边界 / 未落地**

- ~~**UE 侧 `DoodleLighting`（Runtime）/ `DoodleLightingEditor`（Editor）两个模块尚未创建**（§7.6 的落点）。~~
  **已由 §15.13 补上**（两个模块 + `UDoodleLightingCommandlet`，20 个文件；UE 侧未编译，真机前置 = 重新打包插件包）。
  在此之前，把 `ai_light` 派给工作机会在「UE 起来后找不到 `-run=DoodleLighting`」处失败，并按 D26
  回传一份整轮失败报告（会话上留 `failure_reason_`）—— **不会静默**，这是有意选的失败形态。
- **`baseline_level_` 推导失败只 WARN + 空串**（D8 保留）：候选仍然有价值（下次再派），工作机的硬前置会
  拒掉空基线并回传失败原因。M2 的阻断项依然是「谁在真实链路上产出 `baseline_level_`」，本轮把它接到了
  `shot_render_light()` 上，但**没有在真机上验证过这条推导**。
- ~~**预览机位与分辨率未定**~~ **已定（§15.13）**：预览用交付机位（`camera_file_path_` 的 FBX 经临时
  level sequence 导入），分辨率 1920×1080 固定常量，**都不进载荷**。
- **候选的 `metrics_` 目前还是空的**（§15.13 的 D30 §「工作机/UE 不算指标」只写了一半）：服务端还没在
  `apply_render_report` 里对回传的 PNG 调 `ai_light_stats::analyze_display_image` 复算显示空间指标，
  因此落库的 `candidate_metrics_t` 既没有逐维标量也没有 `landings_`/`satisfaction_` —— 下一轮
  `structure_feedback` 要用它做 §8.1 的「区间满足率 + 落点」时会拿到空值。这是紧接着的一步。
- 前端页面、M2 灵敏度标定、真机一轮渲染回环都不在本轮；本轮**没有** `git commit`（由用户自己提交）。

### 15.13 已落地（UE 侧：`DoodleLighting` + `DoodleLightingEditor` 两个插件模块 —— 一次进程渲染一整轮）

**需求**：填掉 §15.12 末尾「未落地」的第一条 —— 让工作机命令行里的 `-run=DoodleLighting` 真的存在：
一次进程内把整轮 6 个候选各渲染一张 PNG + 一张 EXR，写到载荷给定的路径上（工作机在起 UE 之前
已把相对路径拼成本地绝对路径），再写一份 `<out_dir>/render_report.json` 给工作机回传。

> **候选产物改成视频后（§15.16），本节有两处不再成立**：① `-run=DoodleLighting` 不再是派发流程的入口
> （派发改成无 `-run=` 启动 + PIE 执行器 + MRQ）；② 每候选不再是「一张 PNG + 一张 EXR」而是
> 「一段 mp4 + 首帧海报」。本节里**仍然成立**的是建灯/调色/锚点换算那部分算法与引擎坑 41~46。

**开工前由用户拍板的两件事**

| 问题 | 决定 | 影响 |
|---|---|---|
| 预览用哪个机位 | **交付机位**：载荷 `camera_file_path_`（FBX 相机）经临时 level sequence 导入后求值 | 与美术成片同一机位，不用往载荷里加字段 |
| 预览分辨率 | **1920×1080 固定常量**（`DoodleLightingCapture::PreviewWidth/PreviewHeight`）—— 仅手工调试的 SceneCapture 路径；派发流程的分辨率由 MRQ 定（有电影管线配置资产就沿用，自建配置默认 1920×1080），见 §15.16 | 不进 wire 契约：出图尺寸服务端不参与（它只按命名规则现算落点） |

**模块分工**（`.uplugin` 新增两条 Module 记录，现在是 8 个模块）

| 模块 | 类型 / 加载阶段 | 内容 | 为什么分开 |
|---|---|---|---|
| `DoodleLighting` | Runtime / Default / Win64 | `Source\DoodleLighting\`：IR 镜像（枚举 + 结构体 + 双向字符串转换）、JSON 严格读/写、报告文件读写、纯换算（`ops`） | 数据层不依赖编辑器；「服务端读的 json 就是这里写的」只有一份实现 |
| `DoodleLightingEditor` | **Editor / Default** / Win64 | `Source\DoodleLightingEditor\`：捕获装置、执行器（换世界/落灯/全局量）、`UDoodleLightingCommandlet` | 依赖 UnrealEd / Sequencer / MovieSceneTools / FBX / OpenEXR，都是编辑器模块 |

`DoodleLightingEditor` 的 `LoadingPhase` 必须是 `Default`：`-run=X` 的类查找发生在插件加载阶段之后，
`PostEngineInit` 有找不到 `UDoodleLightingCommandlet` 的风险（`doodleEditor` 装 `UDoodleAutoAnimationCommandlet`
用的就是 `Default`）。

**入口契约（工作机 → UE）**

```
UnrealEditor-Cmd.exe <uproject> -windowed -log -AllowStdOutLogVerbosity -ForceLogFlush -Unattended \
    -AllowCommandletRendering -run=DoodleLighting -Params=<载荷 json>
```

- `-AllowCommandletRendering` **不能漏**：`CommandletHelpers::TickEngine` 只在这个开关出现时才 tick 渲染
  （`Engine\Source\Runtime\Engine\Private\Commandlets\Commandlet.cpp:161`），否则我们抓到的是一张没渲染过的画面。
  已加进 `src\doodle_lib\exe_warp\ai_light_render.cpp:53-58` 的参数表。
- `-run=DoodleLighting` → 引擎补 `Commandlet` 后缀后 `FindFirstObject`，所以类名精确是 `UDoodleLightingCommandlet`，
  **不需要任何注册代码**（`LaunchEngineLoop.cpp:2291-2299`）。
- 载荷是**裸 UTF-8**（工作机 `FSys::write_tmp_file` 写出），commandlet 读的时候手工摘 BOM
  （`LoadUtf8File`）——工作机那一侧是按不带 BOM 的 UTF-8 用 nlohmann 读回报告的。
- 报告固定写 `<out_dir>\render_report.json`（`DoodleLightingReport::MakeReportPath` ↔ 服务端
  `g_ai_light_render_report_file_name`），并且用 `ForceUTF8WithoutBOM` 保存；`metrics` 字段**整块不写**（D30）。

**执行器设计决定**

| 编号 | 决定 | 理由 |
|---|---|---|
| D30 | 工作机 / UE **不算指标**：`metrics` 留空 | 显示空间指标由服务端用既有 `ai_light_stats` 对 PNG 复算，避免两处实现同一套度量（也避免 UE 侧再做一遍色调映射口径） |
| D31 | 执行器**幂等落「期望状态」**：以 `rig.lights_`（全量灯集）+ `rig.globals_` 为准逐候选重建；`rig.ops_` 只作审计 | `diff_rigs()` 永不产生 add/remove（§6.6 护栏），只按 `ops` 执行会一盏灯都建不出来 |
| D32 | 锚点换算在 UE 侧（`rig.h:69-71` 明确「执行器负责换算成世界坐标」）：相机空间 → 主体点 = 相机前向射线命中点（打不到就用相机位置 + 前向 ×5m）；方位角以相机前向为 0、向右为正；仰角以过主体点的水平面为 0；距离单位米 | 服务端只存语义坐标，世界坐标依赖关卡内容 |
| D33 | 灯型细节默认值：spot 外 45°/内 25°（载荷给了用载荷）；rect 宽高 = `source_radius_cm`×2；`softness` → point/spot/rect 的 `SourceRadius`、directional 的 `LightSourceAngle`；单位 candela/lumen/lux/unitless → `ELightUnits`；mobility static_light/stationary/movable → Static/Stationary/Movable | 载荷没有的字段只能由执行器补，且必须写下来（否则「同一个 rig 两台机器出两张不同的图」） |
| D34 | 全局量落点：曝光 → PP `AutoExposureMinBrightness=MaxBrightness=1.0` + `AutoExposureBias`；白平衡 → PP `WhiteTemp`/`WhiteTint`；环境 → `ASkyLight` 强度 + `AExponentialHeightFog` 密度；调色 → PP `ColorContrast`/`ColorSaturation`/`ColorGain=(1+lift)`/`VignetteIntensity`/`BloomIntensity` | §7.5 的曝光锁与冻结后处理基线（`PostProcessVolumeConfig` 那组值）是美术验收基准，必须照抄 |
| D35 | 预览分辨率 1920×1080 常量 | 见上表（用户拍板） |
| D36 | **不用 MRQ，用 `USceneCaptureComponent2D`**：LDR 通道 `RTF_RGBA8_SRGB` + `SCS_FinalColorLDR` → PNG；HDR 通道 `RTF_RGBA16f` + `SCS_FinalColorHDR` → EXR（线性工作色彩空间、无色调曲线） | MRQ 要 level sequence + `UMoviePipelinePrimaryConfig` 资产，现网那条链路还要**第二个进程**驱动；队列没有单帧/同步 API，6 个候选之间换灯也没有可靠的 per-job 生效点。SceneCapture 在一次启动内确定性地出一帧。保真代价：Lumen GI 没有成片前的预热帧，靠 8 次丢弃式预热抓帧收敛（`g_warmup_iterations`） |
| D37 | **不复制关卡**：`LoadObject<UWorld>(nullptr, 包路径)` 后换 `GWorld` 用，全程不 `DuplicateAsset`/不 `SaveLoadedAssets`/不 `MarkPackageDirty` | 现有链路复制工作副本是因为它要把改动落盘、再由第二个进程的 MRQ 从磁盘资产渲染；我们单进程内渲染、退出即丢弃 ——「源资产永不修改」成立得更彻底，也不往 `/Game/` 写临时关卡。美术手工灯不删（§7.5 的 `ClearAllLight` 会连美术灯一起清掉，只清我们自己建的 actor） |

**文件清单**（批 4 时 20 个新文件；**批 9 后为 18 个** —— 下面表里 `DoodleLightingCommandlet.{h,cpp}` 与
`DoodleLightingCapture` 的抓帧/写盘一族已删除，见 §15.17；本表保留批 4 的原貌作为历史记录）

| 文件 | 职责 |
|---|---|
| `Source\DoodleLighting\DoodleLighting.Build.cs` | Runtime 模块依赖（Core/CoreUObject/Engine/Json/JsonUtilities；Private 空 —— 数据层不依赖编辑器） |
| `Source\DoodleLighting\Public\DoodleLighting.h` · `Private\DoodleLighting.cpp` | `FDoodleLightingModule` + `IMPLEMENT_MODULE` |
| `Source\DoodleLighting\Public\DoodleLightingTypes.h` · `Private\DoodleLightingTypes.cpp` | 服务端 IR 的镜像：10 个枚举（含 `EDoodleRenderStatus{Pending,Rendered,Failed}` —— **没有 Succeeded**，与服务端 `candidate.h:34-45` 同名）+ 23 个结构体 + `DOODLE_LIGHTING_ENUM_IMPL` 生成的双向转换 |
| `Source\DoodleLighting\Public\DoodleLightingJson.h` · `Private\DoodleLightingJson.cpp` | `ParseTaskPayload` / `ParseLightingRig` / `SerializeTaskPayload` / `SerializeRoundReport` / `ParseRoundReport`；报错一律带字段路径（例 `candidates[3].rig.lights[1].role：缺失或不是字符串`）；严格 = 服务端 `to_json` 无条件写出的键 |
| `Source\DoodleLighting\Public\DoodleLightingReport.h` · `Private\DoodleLightingReport.cpp` | `MakeReportPath` / `WriteReportFile`（`ForceUTF8WithoutBOM`）/ `ReadReportFile` |
| `Source\DoodleLighting\Public\DoodleLightingOps.h` · `Private\DoodleLightingOps.cpp` | 纯换算：灯型/单位/半径/锥角/锚点定位（`ResolveLightPlacement`，可单测的那一层） |
| `Source\DoodleLightingEditor\DoodleLightingEditor.Build.cs` | Editor 模块依赖：去掉 MRQ 四件，加 `Sequencer`/`MovieSceneTools`/`FBX`/`Imath`/`UEOpenExr`/`UEOpenExrRTTI` + `bEnableExceptions = true`（OpenEXR 的异常是 `std::exception` 派生） |
| `Source\DoodleLightingEditor\Public\DoodleLightingEditor.h` · `Private\DoodleLightingEditor.cpp` | `FDoodleLightingEditorModule` |
| `Source\DoodleLightingEditor\Public\DoodleLightingCapture.h` · `Private\DoodleLightingCapture.cpp` | 两个 `ASceneCapture2D` + `FCaptureScope`（Create/Destroy/SyncCamera/SetPostProcessSettings/CaptureFrame）+ `WritePng`（`IImageWrapper`）/`WriteExr`（OpenEXR，`FExrMemStreamOut : Imf::OStream` 照抄 `MoviePipelineEXROutput.cpp:54-108`）+ `ApplyFrozenPostProcessBaseline` |
| `Source\DoodleLightingEditor\Public\DoodleLightingExecutor.h` · `Private\DoodleLightingExecutor.cpp` | `PrepareRound`（换世界 + 临时 level sequence + FBX 交付机位 + 捕获装置）/ `RenderCandidate`（清上一候选的 AI 灯 → 全局量 → 按 rig 全量建灯 → 预热 → 抓帧 → 写 PNG/EXR）/ `FinishRound` / `GetCameraView` |
| `Source\DoodleLightingEditor\Public\DoodleLightingCommandlet.h` · `Private\DoodleLightingCommandlet.cpp` | `UDoodleLightingCommandlet::Main`：`ParseCommandLine(*Params, …, ParamsMap)` 取 `Params` → 读载荷 → 逐候选调执行器并计时 → 写整轮报告；**载荷解析不出 `out_dir` 时也要能写出报告**（先从裸 json 抠 `out_dir`，会话才不会在 `generating` 上干等） |

**一轮的时序（UE 进程内）**

```
Main(-Params=<json>)
  ├─ LoadUtf8File + FJsonSerializer → ParseTaskPayload      （失败：写 {error} 报告后返回 1）
  ├─ PrepareRound：LoadObject<UWorld>(baseline_level)
  │     → DestroyWorldContext/ DestroyWorld / GWorld = 新世界 / CreateNewWorldContext / InitWorld
  │     → FCaptureScope::Create（LDR + HDR 两个 SceneCapture2D）
  │     → 临时 ULevelSequence（/Temp/DoodleLightingCamera_<guid>）+ SpawnActor<ACineCameraActor>
  │       + AddPossessable/BindPossessableObject + UMovieSceneSpawnableActorBinding
  │       + UMovieSceneSpawnTrack/SpawnSection + CreateCameraCutSectionForCamera
  │       + LevelSequencePlayer->Play() + FBX 导入（ReadyFBXForImport → ImportFBXCameraToExisting
  │         → ImportFBXIfReady）+ 求值出 Location/Rotation/HFOV
  └─ 每个候选：
        清掉上一候选的 AI 灯 → 全局量（PP 设置同时写捕获组件与关卡 PP 体积）
        → 以 rig.lights 全量建灯（含手工灯之上叠加）→ 8 次预热抓帧（丢结果）
        → CaptureFrame → WritePng(<candidate.png_path>) + WriteExr(<candidate.exr_path>)
        → 记候选报告（status=rendered/failed、render_ms、light_count）
  └─ FinishRound（销毁 AI 灯/捕获装置/自建 PP 体积/临时序列 actor，回收世界）
  └─ WriteReportFile(<out_dir>/render_report.json)
```

**这一轮踩到的引擎侧坑（下次改 UE 代码直接照着避）**

41. **`USkyLightComponent` 没有 `SetLightColor`**（`SkyLightComponent.h` 只有 `SetIntensity` /
    `SetLowerHemisphereColor`）。天空光的颜色是它自己的 `LightColor` 语义 → 载荷给 RGB/色纸时**只告警、不动**。
42. **相机切换轨不在绑定 GUID 下**：`UMovieScene::FindTrack<T>(const FGuid&)` 拿不到它，要用
    `UMovieScene::GetCameraCutTrack()`（`MovieScene.h:754`）。`CreateCameraCutSectionForCamera` 返回 **void**，
    切轨句柄只能从 `UMovieSceneCameraCutSection::GetCameraBindingID()` 拿（`MovieSceneToolHelpers.h:438`）。
43. **`CommandletHelpers::TickEngine` 声明在 `Commandlets/Commandlet.h:189`**（`namespace CommandletHelpers`），
    不在 `Commandlets/CommandletHelpers.h`（那个头只有 `BuildCommandletProcessArguments`）。
44. **`FMovieSceneBindingReferences` 是 struct 不是 UObject**（`MovieSceneBindingReferences.h:71`，
    `GetReference` 在 `:81`），`UMovieSceneSequence::GetBindingReferences()` 返回指针（`MovieSceneSequence.h:97`）。
45. **执行器里的「每候选清理列表」不能装长期对象**：机位 actor 与自建的 PP 体积如果进了
    `G_AiSpawnedActors`，第二个候选开头就会被连根删掉（自建 PP 体积现在由 `G_PostProcessVolumeOwned`
    在 `FinishRound` 里单独销毁）。
46. **EXR 只能自己调 OpenEXR**：`ImageWrapper` 的 `ExrImageWrapper.cpp` 在 5.7 只有读路径
    （没实现 `SetRaw`/`GetCompressed`）；头文件靠 `Imath`/`UEOpenExr`/`UEOpenExrRTTI` 三个模块名拿到，
    并且模块必须开 `bEnableExceptions`。

**验证状态（重要：本轮的 UE 代码没有在本机编译过）**

- 仓库里 `E:\Doodle\script` 下只有**打包好的插件副本**（`uePlug\5.7\Plugins\Doodle_*` 里的 `HostProject.uproject`），
  源码树 `E:\Doodle\script\uePlug\Doodle` 没有 HostProject、也没有插件打包/构建脚本 —— 所以这 20 个文件
  **只做了 API 级核对（对着 `E:\UnrealEngine\Engine\Source` 的 5.7 源码逐个查签名），没有 compile 验证**。
  真机验证的前置动作：**重新打包插件包**（`ue_exe.cpp:84-128` 的 `installUePath` 每次跑 UE 前会用
  `get_ue_plugin(version)` 下载插件包覆盖 `Engine/Plugins/Doodle`，不打包就还是旧包、没有 `DoodleLighting`）。
- C++ 侧本轮唯一的改动是工作机命令行加 `-AllowCommandletRendering`：`Ninja_debug` 构建通过
  （`[5/5] Linking CXX executable bin\test_main.exe`），`--run_test=ai_light_*` **119 用例 / 1094 断言 exit 0**
  （与 §15.12 基线一致；全量基线 236 / 1847）。本轮**没有** `git commit`（由用户自己提交）。

**仍未落地**

- 真机一轮回环（工作机 → UE → 出图 → 回传 → `awaiting_judgement`）没跑过；`baseline_level_` 的真实推导
  也仍是 M2 阻断项（§15.12 那条）。
- 前端页面（§14）、M2 灵敏度标定、参考图与候选的并排 UI 都不在本轮。

### 15.14 已落地（产物落工作机本地 + HTTP 上传 —— D38~D43）

**用户指令（原文）**

> 修改方案, 这里的输出目录改为工作机本地目录, 在输出后, 使用 http 上传, 并记录 使用 http::kitsu_ctx_t 类中,
> 生成的目录不在 网络盘/UNC 挂载, 使用 http 上传, 而在 src/doodle_lib/core/entity_path.h
> src/doodle_lib/core/entity_path.cpp 文件中, 生成的目录才会使用  网络盘/UNC 挂载

**这条规则是全局的（不只 ai_light）：两族目录**

| 目录族 | 派生者 | 位置保证 | 谁能用 |
|---|---|---|---|
| `http::kitsu_ctx_t` 派生 | `kitsu_ctx_t::root_` 底下（`get_ai_light_session_dir` / `get_ai_light_iteration_dir` / `get_ai_light_candidate_file` …） | **只保证同一台机器同卷**（`:173-174` 的注释就是为这件事写的：备份挂在同一片根下，`FSys::rename` 才不会因为「源在本地盘、项目在 NAS」失败）。**不是**共享盘 | 只有**这台机器自己**（服务端进程、或工作机上它自己那份 `g_ctx().get<kitsu_ctx_t>()`，先例 `depth_estimation_task.cpp:40`） |
| `core::entity_path` 派生 | `get_shots_animation_output_path` / `get_shots_auto_lighting_upload_path` / `get_entity_character_rig_maya_path` … | 项目 / 资产路径，**在网络盘 / UNC 上**（判定工具：`FSys::is_windows_remote_path`，`file_sys.cpp:209-211`，看根是否 `//`） | 全公司机器 |
| （工作机本地缓存） | `core_set::get_set().get_cache_root(sub)`（`core_set.h:31`，会建目录） | 工作机自己的本地盘 | 只有该工作机 |

**所以**：工作机的渲染产物**不能**写 `kitsu_ctx_t` 派生出来的目录 —— 那份 `root_` 是**服务端**的
`"D:/kitsu_data"`（`launch/kitsu_supplement.cpp:150`），对工作机来说既可能不存在、也可能隔着网络；
写进去就退化成「工作机直写服务端盘」。改法 = UE 写**工作机本地**，工作机再 HTTP 上传。

**实现决定（承 §15.12 的 D24–D29、§15.13，续 D38–D43）**

| # | 决定 | 理由 |
|---|---|---|
| D38 | 目录归属按上表：`kitsu_ctx_t` 派生目录只保证同机同卷、不是共享盘；工作机产物写 `core_set::get_set().get_cache_root(g_ai_light_local_cache_flag)`；只有 `entity_path` 派生的项目/资产目录才在网络盘/UNC | 用户指令；`kitsu_ctx_t::root_` 是服务端的数据根，工作机对它的「可见性」纯属偶然 |
| D39 | 载荷里的路径一律**相对**：`out_dir_ = <session_uuid>/<轮次 uuid>`，候选 `png_path_`/`exr_path_ = <candidate_uuid>.png|.exr`。命名服务端定、根工作机定，工作机起 UE 之前拼成绝对路径写进 `-Params` 那份 JSON | 命名只有一处定，才能让「服务端要用时算出来的位置」与「工作机上传的落点」永远是同一个字符串；根由工作机定才能本机写；相对路径让两侧目录布局各自演化而不破契约 |
| D40 | 工作机 POST 到 `/api/doodle/ai/light/candidate/{id}/file/{render｜render_linear}`，服务端把文件落到按候选行现算出来的落点上（`candidate_render_path`，D44） | 落点由候选行三个 uuid 定，所以工作机连「服务端把图放哪儿」都不需要知道，也不需要传路径，更伪造不了落点 |
| D41 | 回传说 `rendered` 的候选，服务端按命名规则算出该候选的 PNG 落点，查文件是否真的在；不在就降级 `failed`，全被降级则走整轮失败出口 | 上传是独立一步（D40）：UE 写了文件、报告说 rendered、上传失败 → 前端拿到 404 图，而美术唯一的判断通道就是这张 PNG。降级是保守选择（会把「其实渲出来了但没传上来」也判失败，可接受） |
| D42 | EXR 是**可选**产物：工作机尽量传，文件不在 / 传失败都只 WARN、不阻断本轮；服务端按需判存在即可（EXR 不落库，所以没有「悬空记录」要清） | 前端永不展示 EXR；它缺失不影响本轮的判断，也就不该让整轮失败 |
| D43 | 工作机本地产物**保留不删**：上传成功也不清，只在日志里报路径 | 出问题时它是唯一的现场；磁盘增长留给人工 / 后续定时清理 |
| D27（不变） | 回传仍然只带状态与数值，**不带路径**；产物位置一律按 `kitsu_ctx_t` 的命名规则现算 | 见 §15.12 / §15.15 的 D44 |

**一轮的形状**

```
服务端 advance_session(generating)
  ├─ 候选行只写状态与数值（**不写路径**，D44）
  └─ 载荷里写**相对路径**：out_dir = make_iteration_relative_dir(session, iteration_uuid)
                          候选 png_path/exr_path = make_candidate_file_name(cand_id, linear)
工作机 ai_light_round_distributed::run
  ├─ l_local_root = core_set::get_set().get_cache_root(g_ai_light_local_cache_flag)
  ├─ make_ue_payload(payload_, l_local_root)：out_dir / 每个候选的 png|exr 拼成工作机绝对路径
  ├─ 跑 UE（-Params=<那份拼好的 JSON>；命令行见 §15.13）
  ├─ 读 <本地 out_dir>/render_report.json
  ├─ **逐候选上传**（必须在回传报告之前！D41 靠它）：
  │     PNG 不在本地 / 上传失败 → 该候选改成 failed（PNG 必传）
  │     EXR 不在本地 / 上传失败 → 只 WARN（D42）
  ├─ POST /api/doodle/ai/light/render_report/{task_id}（只带状态与数值）
  └─ put_job_info（终态）；本地产物目录保留不删（D43），只打日志
服务端 render_report_route → apply_render_report(sql, ctx, task_id, report)
  ├─ 任一条 rendered？否则 mark_iteration_failed（D28：只写会话 failure_reason_）
  ├─ 逐条覆盖 render_status_/render_ms_/light_count_/metrics_（D27：路径不在回传里）
  ├─ D41 降级：rendered 但 candidate_render_path(ctx, 行, false) 算出的 PNG 不在 → failed + WARN，计数 l_missing_png
  ├─ D42 诊断：同法算出的 EXR 不在 → 只 WARN
  └─ 还有 rendered → mark_iteration_rendered（轮次 awaiting_judgement）
      全被降级 → failure_reason_ = "轮次 {} 的 {} 个候选渲染成功但产物文件缺失（海报或视频），本轮无可判断结果"
```

**落点**

| 项 | 落点 |
|---|---|
| D38–D43 的文字定义 + 三个契约件 | `src/doodle_core/metadata/ai_light/task_payload.h`（相对路径构造在 `make_iteration_relative_dir` / `make_candidate_file_name`；本地缓存分片名 `g_ai_light_local_cache_flag{"ai_light"}`） |
| 服务端上传路由 | 声明 `ai_light.h:166`（`enum class ai_light_candidate_file_role{render, render_linear}`，role 段是 scoped enum → 路由宏自动展开白名单正则）· `:187`（`candidate_file_route`）· 路径模板常量 `g_candidate_file_path_template` · 注册 `kitsu.cpp:338` `/api/doodle/ai/light/candidate/{}/file/{}` · 实现 `ai_light.cpp:828` |
| 服务端 D41/D42 | `ai_light_orchestrator.cpp::apply_render_report`（按现算的落点判存在：PNG 降级 + EXR 只 WARN），include `<doodle_lib/core/file_sys.h>` |
| 工作机上传递归 | `src/doodle_lib/http_client/kitsu_client.cpp::upload_ai_light_candidate_render(uuid, path, bool linear)` → 复用私有 helper `upload_asset_file`（octet-stream + Content-Disposition + Last-Modified，**只认 204**）；URL 用相对路径 `/api/doodle/ai/light/candidate/{}/file/{role}` |
| 工作机改造 | `src/doodle_lib/exe_warp/ai_light_render.cpp`（`make_ue_payload` + 本地根拼接 + 上传循环 + 保留不删）· `ai_light_render.h` 头注释 |
| UE 侧 | **一行未改**（`DoodleLighting*` 那 20 个文件完全不动）：UE 只按载荷给的 `PngPath`/`ExrPath` 写文件、按 `OutDir` 写报告，工作机把绝对路径塞进载荷即可 |

**服务端上传路由的逐条约束（写前端 / 写工作机的人会踩）**

- content-type 必须是 `application/octet-stream`（框架把任何 `application/*` 都归到
  `detail::content_type::application_nuknown`，与 `up_file.cpp:33` 判法一致），否则 **400**。
- 请求体由框架先流到临时文件（`http_session_data.cpp:220-231`，落
  `core_set::get_set().get_cache_root("http")/<uuid><ext>`），路由用 `in_handle->get_file()` 拿它。
- 落点由**候选行**现算（`candidate_render_path(ctx, 候选行, linear)`，D44）；请求里根本没有路径信息，
  所以工作机伪造不了落点，最多只能往「服务端算出来的那个文件名」里灌字节。
- 轮次已 `judged` → 400 拒绝上传；`awaiting_judgement` 期间**允许重传**（美术发现图坏了、工作机重跑）。
- 落盘先写 `<目标>.uploading` 再 `FSys::rename` 到目标：避免前端正好读到半张图。
  **故意不复用 `up_file_base`**（`up_file.h:22-40`）：那套是「按项目+任务类型派生目录」，
  本路由的落点不是派生的，而且它的 `move_file` 在目标已存在时的先备份再 `rename` 在跨卷时会抛。
- 成功回 **204**（`kitsu_client::upload_asset_file` 只接受 204；非 204 → 抛 `doodle_error` → 工作机把该候选判 failed）。
- 授权：`person_.check_not_outsourcer()`，**不做** `check_project_access`（调用方是工作机的机器 JWT，
  只认候选 uuid —— 与 `render_report_route` 同一信任级）。

**测试与回归**

- `src/test/core/ai_light_dispatch.cpp`「第四段：产物存在性（D41）」的 4 个用例：
  `..._downgrades_missing_png_only_for_that_candidate`（13 断言）· `..._all_png_missing_writes_failure_reason`（10）·
  `..._missing_exr_does_not_block`（12）· `..._reported_failure_plus_downgrade_leaves_nothing`（11）。
  注意第 4 个用例：`failure_reason_` 里的计数只数**被降级的**那些，报告里本来就 `failed` 的不算。
- 夹具：`media_dir_`（`temp_directory_path()/doodle_test_ai_light_dispatch_media_<case>`，析构 `remove_all`）
  + `write_probe_file()` 造**真文件**，并用 `candidate_png()` / `candidate_exr()` 两个助手
  （= `candidate_render_path(server_ctx_, 候选行, linear)`）拿到落点 —— D41 判存在，夹具就必须有真文件，
  否则所有「推进到 `awaiting_judgement`」的用例都会因降级而红。
- `Ninja_debug` 构建通过（零 error）；`--run_test=ai_light_*` **124 用例 / 1216 断言** exit 0；
  全量 **241 用例 / 1969 断言 exit 0**。本轮**没有** `git commit`（由用户自己提交）。

**仍未落地**

- **重新打包 UE 插件包**：`ue_exe.cpp:84-128` 的 `installUePath` 每次跑 UE 前都会用
  `get_ue_plugin(version)` 覆盖 `Engine/Plugins/Doodle`，不打包就还是旧包（没有 `DoodleLighting`）——
  §15.13 的 20 个文件因此至今没有 compile 验证（UE 侧零改动，所以这个缺口没有被加重）。
- **真机一轮回环**（工作机 → UE → 出图 → 上传 → 回传 → `awaiting_judgement`）仍未跑过。
- **服务端候选指标复算**（D30 只写了一半）：UE / 工作机不算指标，`metrics_` 目前来自回传里的空值，
  落库后下一轮 `structure_feedback` 的 §8.1「区间满足率 + 落点」会拿到空值。做法 =
  用 `ai_light_stats::analyze_display_image`（`ai_light_stats.h:59`）算候选 PNG，
  再对 spec 的 `targets_` 填 `landings_` / `satisfaction_`。
- `baseline_level_` 的真实推导仍是 M2 阻断项（§15.12 那条不变）；前端页面（§14）不在本轮。


---

### 15.15 已落地（产物路径改「现算」 —— D44）

**用户指令（原文）**

> 检查 src/doodle_core/metadata/ai_light/candidate.h:164~168 中 , 渲染文件路径属性, 他们在
> src/doodle_lib/http_method/kitsu/ai_light_orchestrator.cpp:367~370 这被赋值, 然后, 存库使用,
> 不这样做, 直接将 属性改为方法, 每次重新计算, 是否可行? 会比现在的方法更好吗

> 这里不就是 src/doodle_core/metadata/ai_light/candidate.h:146 in_session_id 参数吗, 同时 ,
> 去除前端契约, 前端不可获取这两个路径, 是否可行

用户的拍板：**做，并把轮次目录名一并换成轮次 uuid**。

**一句话方案**：候选行不记录「产物在哪」——路径 = `candidate_render_path(ctx, 候选行, linear)`，
吃候选行自带的 `session_id_` / `iteration_id_` / `uuid_id_` 三个 uuid。

**为什么可行（三条缺一不可）**

1. 候选行自带三个 uuid（`session_id_` 本来就是有意冗余的列，`candidate.h:146`）——不需要回查轮次行；
2. 轮次目录名就用**轮次自己的 uuid**，于是连「目录名」都是候选行自带的（若用 `it_<序号>`，
   必须查一次轮次行才拼得出来）；
3. `kitsu_ctx_t` 的六个 AI 灯光 getter 补上 `const` 后是**纯拼路径**函数 —— 于是派生函数谁都能调，
   不需要 `g_ctx()`（这一点是 test_main 里必须的，见 D27 的由来）。

**决定（承 §15.14 的 D38–D43，续 D44）**

| # | 决定 | 理由 |
|---|---|---|
| D44 | 候选**不存渲染路径**：`ai_light_candidate` 只有 `render_status_` 与实测数值，没有 `render_png_path_` / `render_exr_path_` 两列；候选 JSON 里也没有路径键。路径由 `candidate_render_path(const http::kitsu_ctx_t&, const ai_light_candidate&, bool linear = false)` 现算，轮次目录名就是轮次 uuid | ① 候选行已经有三个 uuid，再存一份路径 = 同一事实记两遍，且两份可能漂移（`root_` 变了、目录被搬了）；② 存下来就把**服务端磁盘路径**带进了对外契约（候选 JSON），而前端只该拿到 uuid 与图片字节；③ 目录名用 uuid 之后连序号都不用查，「候选 → 路径」是一次纯计算。**代价**：库里没有「文件实际在哪」的记录（只能按规则找）；EXR 缺失退化成「读的时候判存在」；将来若出现**非派生**产物（人工补图、换对象存储）得重新引入一个显式路径字段 |

**一轮的形状（与 §15.14 那张图的差别只有「落点」一列）**

```
派发：out_dir_ = <session uuid>/<轮次 uuid>（相对，D39）
      候选行只写 render_status_ / 实测数值（D44）
工作机：拼本地根 → UE → 逐候选上传（PNG 必传 / EXR 尽力，D42）→ 回传报告
服务端：apply_render_report(sql, ctx, task_id, report)
        ├─ D41：PNG 的落点 = candidate_render_path(ctx, 行, false)，文件不在 → 降级 failed
        ├─ D42：EXR 的落点同法算出，文件不在 → 只 WARN
        └─ 还有 rendered → mark_iteration_rendered（UPDATE 只写状态与数字）
            全被降级 → failure_reason_ = "轮次 {} 的 {} 个候选渲染成功但产物文件缺失（海报或视频），本轮无可判断结果"
```

**改动落点**

| 项 | 落点 |
|---|---|
| 命名规则（唯一真相） | `kitsu_ctx_t.h`：`get_ai_light_iteration_dir(session, iteration_id)` = 会话目录 / `fmt::to_string(iteration_id)`；`get_ai_light_candidate_file(session, iteration_id, candidate, linear)` = 轮次目录 / `<候选 uuid>.png|.exr`；六个 `get_ai_light_*` 全部 `const` |
| 派生函数 | `ai_light_orchestrator.h`：`inline FSys::path candidate_render_path(const http::kitsu_ctx_t&, const ai_light_candidate&, bool in_linear = false)` —— **只**吃 ctx 的 `root_`，所以不做成成员、也不在函数里取 `g_ctx()` |
| 候选行 | `candidate.h`：没有 `FSys::path` 字段，也没有那两个 JSON 键（`render_status_` 与数值保留） |
| 建表 | `src/doodle_lib/sqlite_orm/sqlite_database.cpp` 的 `reg_table<ai_light::ai_light_candidate>` 里没有路径列 |
| 派发 | `ai_light_orchestrator.cpp`：`l_payload.out_dir_ = make_iteration_relative_dir(io_session.uuid_id_, l_it.uuid_id_)`；候选循环不写路径，**本函数也不需要 `kitsu_ctx_t`** |
| 落库 | `mark_iteration_rendered` 的候选 UPDATE 只写 `render_status_` / `render_ms_` / `light_count_` / `metrics_` |
| 回传 | `apply_render_report` 签名带 `const http::kitsu_ctx_t& in_ctx`（第 2 参）：D41 按现算查存在，D42 只 WARN |
| 上传路由 | `ai_light.cpp::candidate_file_route::post`：`l_dst = candidate_render_path(ctx, l_cand, l_linear)` |
| 取图 / 定稿 | `candidate_image_route::get`、`build_accepted_preview_file` 同样现算 |
| 工作机 | `task_payload.h` 的 `make_iteration_relative_dir(session, iteration_id)` 末级是轮次 uuid；`exe_warp/ai_light_render.cpp` **零改动**（它只用 `make_candidate_file_name` 与 `out_dir_`） |
| 前端契约 | `docs/ue_ai_lighting_frontend.md:39-42`：候选对象里没有路径字段，取图只走 `/candidate/{uuid}/image` |

**迁移：不用加版本号**

`upgrade_3_t` 的 `sync_schema()` 对已存在的表只 `continue`（不加列、不改列、不删列），所以「没有路径列」
这件事只需 `reg_table` 里不写那两行 —— **不要**新增 `g_version_31 → 32`。已经在 v31 上的开发库会留下两个
没人引用的列（ORM 只 SELECT / INSERT 映射过的列），无害；生产库从 30 直接升上来就根本没有这两列。

**坑（续 §15.14 的 41–46）**

47. **测试夹具的成员不能叫 `ctx_`。** `doodle_test::coro_fixture_t` 的 `ctx_` 是 asio `io_context`
    （夹具自己就在 `make_strand(ctx_)`），同名会静默遮蔽 —— 夹具里的 `kitsu_ctx_t` 成员叫 `server_ctx_`。
48. **`sync_schema()` 不删列**，所以「候选没有路径列」这件事在磁盘上对老库没有痕迹 ——
    别把「老库列还在」当成改漏了。
49. **JSON 键是对外契约**：去掉候选对象里的两个路径键必须同步改 `docs/ue_ai_lighting_frontend.md`，
    否则前端会以为它们还在。
50. **纯同步用例不能用 `DOODLE_FIXTURE_TEST_CASE_CORO`**（§15.14 坑 37 同一条）：新增的
    `candidate_render_path_is_a_pure_function_of_the_row` 体里一个 `co_await` 都没有 → `error C4716:
    ...: must return a value`；改用 `BOOST_FIXTURE_TEST_CASE(name, fixture_t) { auto& l_fx = *this; ... }`。

**测试与回归**

- 改：`ai_light_paths.cpp`（黄金路径用固定轮次 uuid，用例 `iteration_dir_is_named_by_the_iteration_uuid`）·
  `ai_light_state.cpp`（夹具无路径列；`pre_assigned_uuid_survives_the_deferred_insert` 改为「路径仍可
  由三个 uuid 复现」）· `ai_light_schema.cpp`（夹具无路径列）·
  `ai_light_dispatch.cpp`（夹具新增 `server_ctx_`（root 挂 `media_dir_`）+ `candidate_png()`/`candidate_exr()`
  两个助手，真文件按**同一个函数**造；12 处 `apply_render_report` 调用带 ctx；
  `apply_report_keeps_paths_derivable_from_the_row`；`apply_report_missing_exr_does_not_block`）。
- 新增：`candidate_render_path_is_a_pure_function_of_the_row`（同一行两次结果相同、PNG/EXR 同名不同
  扩展名、文件名是候选 uuid、父目录是轮次 uuid、祖父是会话目录、同轮不同候选不撞、只换 `root_` 时
  相对路径不变）。
- 构建：`Ninja_debug` 零 error（`[4/4] Linking CXX executable bin\test_main.exe`）。
- 回归：`--run_test=ai_light_*` → `124 test cases out of 241 passed`、1216 断言、exit 0；
  全量 **241 用例 / 1969 断言全部通过、exit 0**。本轮**没有** `git commit`（由用户自己提交）。

**仍未落地**（与 §15.14 那份一致）

- **重新打包 UE 插件包**仍是 §15.13 那 20 个文件 compile 验证的前置（UE 侧零改动）。
- **真机一轮回环**（工作机 → UE → 出图 → 上传 → 回传 → `awaiting_judgement`）仍未跑过。
- **服务端候选指标复算**（D30 只写了一半）：`metrics_` 仍来自回传空值，`ai_light_stats::analyze_display_image`
  那条路没有接。
- `baseline_level_` 的真实推导仍是 M2 阻断项；前端页面（§14）不在本轮。

### 15.16 已落地（候选产物改视频：无 `-run=` 启动 + PIE 执行器 + MRQ —— D45~D50）

**需求**（用户原话）

> 「检查结构体 src/doodle_lib/exe_warp/import_and_render_ue.h:114~138, 我们在使用
> src/doodle_core/metadata/ai_light/task_payload.h:113~149 ai_light_task_payload_t 运行渲染时,
> 使用自动灯光已经准备好的相机和定序器, 并且输出渲染视频作为美术选择的参考, 而不是单纯的图片」

**开工前由用户拍板的两件事**

| 问题 | 决定 | 影响 |
|---|---|---|
| 每候选产物 | **mp4 + 首帧海报 PNG**，本轮**不出** EXR | 前端网格仍用海报，点开才播视频；EXR 的角色留在契约里但一轮不产 |
| 渲染走哪条路 | **照 `auto_light` 渲染进程的形状**：无 `-run=` 启动 + PIE 执行器 + 每候选把 rig 建进 PIE 世界（MRQ 画质） | 工作机的命令行、UE 侧入口、命令集全部改；服务端契约加字段 |

**一句话**：一轮 6 个候选各自变成「一段 mp4 + 一张首帧海报」；渲染交给 `auto_light` 已经生成好的那套资产
（`render_map_` / `level_sequence_import_` / `movie_pipeline_config_`）+ 电影渲染管线（MRQ），
rig 由我们在**每个 job 开始前**建进 PIE 世界。

**为什么必须换掉原来的形状（两条引擎事实，都是这轮才查清的）**

1. **`-run=` commandlet 里 MRQ 一帧都不渲**：MRQ 靠 `FCoreDelegates::OnBeginFrame` 推进帧
   （`MoviePipeline.cpp:260`），而全引擎唯一的广播点是 `LaunchEngineLoop.cpp:5663`（`FEngineLoop::Tick` 内）；
   `CommandletHelpers::TickEngine`（`Commandlet.cpp:127` 起）只 tick 引擎、泵 Slate、处理任务图，
   **不广播它** → 管线会被建起来、参数量好，但永远等不到推进。
2. **`MoviePipeline::FindCurrentWorld()` 只认 `Game` / `PIE` 世界**（`MoviePipelineUtils.h:26-48`），
   而原来的 `SwapToWorld` 把换来的世界建成 `EWorldType::Editor`（`DoodleLightingExecutor.cpp`）→
   in-process 执行器拿不到世界。

→ 于是照 `auto_light` 的渲染步走：工作机**无 `-run=`** 启动编辑器，由插件模块挂在 `OnPreTick` 上的
`UDoodleLightingRoundRunner` 驱动，用 **PIE 执行器**（自己会造 PIE 世界）逐候选渲染。
PIE 世界是编辑器世界的复制，但**运行时 spawn 进 `GEditor->PlayWorld` 的 actor 会被渲进去** ——
这正是我们可以逐候选换灯的原因。

**实现决定表**

| # | 决定 | 理由与代价 |
|---|---|---|
| D45 | 候选产物＝视频：渲染交 MRQ；有 `movie_pipeline_config_` 就 `SetConfiguration()`（**深拷贝**）后逐 job 改配置，缺失/加载失败就自建配置（1920×1080） | §15.13 当初拒绝 MRQ 的 D36 三条理由（要序列+配置资产、要第二个进程驱动、队列没有单 job 钩子）**都已不成立**：资产由 `auto_light` 生成好了、引擎子系统能在同一进程里跑、`OnIndividualJobStarted()` 就是逐 job 钩子。**D36 被取代** |
| D46 | rig 注入点＝`UMoviePipelinePIEExecutor::OnIndividualJobStarted()`（在 `UMoviePipeline::Initialize` 之前、PIE 已起）；序列里的灯光 spawnable 在**内存里**剥掉；序列**永不落盘** | 引擎源码：`MoviePipelinePIEExecutor.cpp:296-316`（无延迟分支）先 `OnIndividualJobStartedImpl(...)` 再 `Initialize(...)`。序列里的灯是 `auto_light` 建的，会和候选 rig 抢光照；剥掉用公开 API 即可（`RemoveSpawnable`），不落盘所以不动资产 |
| D47 | 每候选＝MRQ 的 **PNG 帧序列**（UE 只写帧）+ 工作机用 OpenCV 合成 `<候选 uuid>.mp4` + 首帧复制成海报；帧率取回传的 `frame_rate_`（= 序列 display rate），拿不到退 25 | UE 侧不装编码器插件（MP4 直出要额外插件），合成放工作机是既有的 `detail::create_move` 路径。`create_move` 因此新增 `std::int32_t in_fps = 25` 形参 —— 帧数是 UE 定的，帧率不对时长就会被拉长/压缩 |
| D48 | 帧区间＝序列自己的 playback range（`auto_light` 的 `SetPlaybackRange`）；载荷另带 `frame_begin_` / `frame_end_` **只作诊断** | 少一个真相来源；真需要覆盖时用 `bUseCustomPlaybackRange` + `CustomStartFrame/CustomEndFrame` |
| D49 | UE 侧 MRQ 设置对齐 `OnSaveReanderConfig`（`DoodleAutoAnimationCommandlet.cpp:1261-1362`）：PNG 输出 + `UMoviePipelineDeferredPassBase`；`OutputDirectory.Path` = 该候选的**绝对**帧目录、`FileNameFormat = "frame_{frame_number}"`、`ZeroPadFrameNumbers = 4`、`bUseCustomFrameRate=true` + `OutputFrameRate`；**只有没有配置资产时**才补 ConsoleVar（关运动模糊、固定流送池）、AA（TSR、1/1、预热 128）、GameOverride（cinematic、TextureStreaming=None）、Camera（FrameClose、Overscan 0） | 有美术/管线给的配置资产就听它的，没有才用我们自己的一套默认值；两套混着设会让「配置资产」变成假的 |
| D50 | 上传角色扩为三元（`render` / `render_linear` / `render_movie`），枚举从 `ai_light.h` 搬到 `task_payload.h`（服务端与工作机共用一个定义）；**派发时 `level_sequence_` 为空 = 本轮不可渲染 → 抛错**（走既有回滚写 `failure_reason_`） | 枚举名会进 URL（路由正则由 `magic_enum::enum_names` 拼），所以定义必须只有一个；原来「基线关卡拿不到就 WARN 后照跑」在视频化之后必然产不出东西，晚失败不如早失败 |
| D51 | 工作机收产物时**从载荷取位置**：按候选 **uuid** 反查那份「已拼成本地绝对路径」的副本（`l_ue_payload`），用它的 `frames_dir_` / `png_path_` / `movie_path_`；**不按命名规则重算**，也不按 `candidate_index_` 查 | UE 写的就是载荷给的位置（`DoodleLightingRoundRunner.cpp:169` 把 MRQ 的 `OutputDirectory.Path` 设成 `InCandidate.FramesDir`），所以载荷那份是「UE 实际用的值」；重算等于工作机再实现一遍服务端命名，服务端/工作机**版本偏斜**（键名没变、命名值变了 —— 这种漂移解码时不报错）会算到不存在的目录上，白丢一整轮。**不按下标**：`candidate_index_` 是弱键，错配会把 A 的成片挂到 B 的 uuid 上，美术对着别人的灯评判。**代价**：多一个「报告里的候选不在载荷里」分支（该候选判 failed，不猜） |

**一轮的新形状**

```
iterate → 派发（装配硬前置：无 task_id / 装配抛错 / level_sequence 空 → 直接失败）
  载荷：out_dir=<session>/<轮次 uuid>；候选 frames_dir=<候选 uuid>/、png_path=<候选 uuid>.png、
        movie_path=<候选 uuid>.mp4、exr_path=<候选 uuid>.exr（全相对）+ render_map/level_sequence/movie_pipeline_config
  工作机：相对 → 本地根 get_cache_root("ai_light")，拼成绝对路径（这一份是后面**唯一的路径权威**，D51）；删残留 render_report.json
  UE（无 -run=）：UDoodleLightingRoundRunner
        ├─ 读 -DoodleAiLightParams → 载入 render_map + level_sequence → 剥灯光 spawnable
        ├─ 交付机位求一次姿态（PrepareAnchorCamera，随后拆掉导入物）
        ├─ 逐候选一个 MRQ job（帧目录 = 该候选）+ PIE 执行器
        │    每个 job 起渲前：ApplyCandidateToWorld(GEditor->PlayWorld, 候选)（灯 + 全局量）
        └─ 写 <out_dir>/render_report.json（candidates[] + frame_rate）→ RequestExit
  工作机：逐候选按 **uuid 从载荷**取帧目录/海报/成片位置（D51，不重算、不按下标）→ collect frames → 首帧→海报 → create_move(mp4, fps=frame_rate) → 上传海报 + mp4（都必传）
  服务端：apply_render_report(sql, ctx, task_id, report)
        ├─ D41：算出的海报或成片缺一 → 该候选降级 failed；全灭 → failure_reason_，轮次留 rendering
        └─ 有 rendered → mark_iteration_rendered（轮次 awaiting_judgement）
```

**改动落点**

| 层 | 文件 | 改了什么 |
|---|---|---|
| 契约 | `src\doodle_core\metadata\ai_light\task_payload.h` | 载荷加 `render_map_` / `level_sequence_` / `movie_pipeline_config_` / `frame_begin_` / `frame_end_`；候选加 `frames_dir_` / `movie_path_`；回传加 `frame_rate_`（轮级）与 `frame_count_`（候选，诊断）；新增 `ai_light_candidate_file_role{render, render_linear, render_movie}` + `make_candidate_frames_dir()` + `make_candidate_movie_file()`；头注释扩到 D38~D50 |
| 命名 | `src\doodle_core\metadata\kitsu_ctx_t.h` | 新增 `get_ai_light_candidate_frames_dir()` / `get_ai_light_candidate_movie_file()`（都 `const`），目录树注释补五行 |
| 服务端 | `ai_light_orchestrator.{h,cpp}` | 新增 `candidate_movie_path()` / `candidate_frames_dir()` 内联纯函数；派发段改用完整装配 + 两条硬前置；候选聚合 7 成员；`apply_render_report` 的 D41 同时查海报与成片（计数器 `l_missing_artifact`） |
| 服务端 | `ai_light.{h,cpp}` + `kitsu.cpp` | 上传路由的 role 用全限定名（本文件就在 `doodle::http::ai_light` 里，写 `ai_light::x` 会命中内层命名空间自己）+ `render_movie` 落点；新增取成片路由 `candidate_movie_route`；`kitsu.cpp` 注册两条 |
| 客户端 | `kitsu_client.{h,cpp}` | `upload_ai_light_candidate_file(uuid, FSys::path, ai_light::ai_light_candidate_file_role)`，URL 段用 `magic_enum::enum_name(role)`；新增 `#include <magic_enum/magic_enum.hpp>`（原文件没有，靠传递包含不可靠） |
| 合成 | `long_task\image_to_move.{h,cpp}` | `create_move` 加 `std::int32_t in_fps = 25` |
| 工作机 | `exe_warp\ai_light_render.{h,cpp}` | 全文重写：四样硬前置（`ue_project_path_` / `render_map_` / `level_sequence_` / `candidates_`）、`make_ue_payload()` 拼本地根、`collect_render_frames()` 按文件名排序、**无 `-run=`** 启动、逐候选合成 + 两个上传 + 回传 + 终态；收产物时按候选 **uuid** 从 `make_ue_payload()` 的副本取 `frames_dir_` / `png_path_` / `movie_path_`（**D51**，不重算、不按下标） |
| UE 侧 | `DoodleLightingEditor\Public\DoodleLightingRoundRunner.h` + `Private\DoodleLightingRoundRunner.cpp`（**新增**） | 一轮的驱动者：读参数 → 载序列剥灯 → 交付机位 → 建 6 个 job → PIE 执行器 → 逐 job 建 rig → 写报告 → 退出 |
| UE 侧 | `DoodleLightingEditor.{h,cpp}` | 模块启动时看见 `-DoodleAiLightParams=` 就造 runner 并挂 `OnPreTick` |
| UE 侧 | `DoodleLightingExecutor.{h,cpp}` | 新增 `PrepareAnchorCamera()` / `ApplyCandidateToWorld()`；抽 `ReleaseDeliveryCamera()` / `BuildRigLights()`；旧 SceneCapture 路径保留但只剩手工调试用（**批 9 已全部删除，见 §15.17**） |
| UE 侧 | `DoodleLightingEditor.Build.cs` | 加 `MovieRenderPipelineCore/Editor/RenderPasses/Settings`（批 9 撤掉了为写盘器开的 `ImageCore`/`ImageWrapper`/`Imath`/`UEOpenExr`/`UEOpenExrRTTI` 与 `bEnableExceptions`） |
| UE 侧 | `DoodleLightingTypes.h` / `DoodleLightingJson.cpp` | 镜像新字段（`FramesDir` / `MoviePath` / `RenderMap` / `LevelSequence` / `MoviePipelineConfig` / `FrameBegin` / `FrameEnd` / `FrameCount` / `FrameRate`） |
| 测试 | `src\test\core\ai_light_dispatch.cpp` · `ai_light_paths.cpp` | 夹具每候选三个探针（海报/成片/EXR）+ `candidate_movie()`；D41 用例改成「两个产物都要在」；新增「海报在、成片缺」用例；新增 wire 键冻结用例 `task_payload_json_keys_are_frozen`；路径用例补 mp4 与帧目录 |

**wire 契约（外部契约，改键 = 改协议）**

- 派发：`session_id / iteration / spec_id / out_dir / baseline_level / render_map / level_sequence /
  movie_pipeline_config / frame_begin / frame_end / ue_project_path / camera_file_path / candidates[]`；
  候选：`candidate_index / candidate_id / frames_dir / png_path / exr_path / movie_path / rig`。
- 回传：`candidates[] / frame_rate / error`；候选：`candidate_index / candidate_id / status / render_ms /
  light_count / frame_count / metrics` —— **回传里永远没有路径**（D27）。
- 上传：`POST /api/doodle/ai/light/candidate/{候选 uuid}/file/{render|render_linear|render_movie}`（`application/octet-stream`，成功 204）。
- 取用：海报 `GET …/candidate/{uuid}/image`；成片 `GET …/candidate/{uuid}/movie`（`video/mp4`，`make_msg` 自带 Range 支持）。

**测试与回归**

- `ai_light_dispatch`：24 用例（+1 wire 冻结，+1「海报在、成片缺」并合并原 D41 用例）；
  `ai_light_paths`：补 mp4 与帧目录的黄金路径；
  `ai_light_candidates`：17 用例（+1 `diff_rigs_emits_one_op_per_changed_global_block`，钉死全局量四块的 op
  —— 这个 diff 在坑 57 里曾经**静默地一个 op 都不发**，此前没有任何用例覆盖 `diff_rigs`）。
- 回归（本轮实测）：`--run_test=ai_light_*` → **128 用例 / 1357 断言全通过**；
  全量 → **245 用例 / 2110 断言全通过、exit 0**（上一版基线 241 / 1969）。
- 教训沿用：管道后的 exit code 不可信，要抓 `N test cases out of N passed`；Boost.Test detailed 汇总行在报告**开头**。

**坑（续 §15.15 的 47~50）**

51. **MRQ 不能在 `-run=` commandlet 里跑**（`OnBeginFrame` 不广播 + `FindCurrentWorld()` 不认 Editor 世界），
    必须走「无 `-run=` 启动 + PIE 执行器」；这也是 `DoodleLightingCapture.h` 里 D36 那段注释被改写的原因。
52. **`GetSpawnable(i)` 不是插入序**（`AddSpawnable` 内部 `InsertSorted` 按 guid 排），
    剥灯光必须**倒着**遍历，别缓存下标。
53. **`UMoviePipelineExecutorJob::SetConfiguration` 是深拷贝**（`Configuration->CopyFrom(InPreset)`），
    所以逐 job 改输出目录安全；但**配置资产本身**若被改就会污染所有 job。
54. **帧率取整**：`FFrameRate::AsDecimal()` 再 `RoundToInt`（整数除 `30000/1001` 会截断成 29）。
55. **`<目标>.uploading` 再 rename** 对视频尤其重要：半截 mp4 播放器直接报错，还可能被浏览器缓存。
56. **工作机不能再假设「报告里说 rendered 就算数」**：上传是独立一步（HTTP），UE 写盘 ≠ 服务端拿得到；
    D41 因此同时查海报与成片两个文件。
57. **`nlohmann::json{x}` 永远得到数组 `[x]`** —— 连 `std::string`、枚举、`std::optional` 都一样；
    要对象得写拷贝初始化 `nlohmann::json j = x;`（或 `json(x)`）。这个陷阱在本模块里踩过三处：
    派发载荷、回传报告（UE 侧按对象解析会直接失败），以及 `ai_light_rig_ops.cpp` 里对 `globals_` 的
    四个子块 diff —— 数组上 `contains(...)` 恒为 false，于是**全局量的 op 一个都不发**且不报错。
    **同一个 diff 还有第二个独立错因**：`l_blocks` 里的键写成了带尾下划线的 `"exposure_"` 等，
    而 `globals_t` 的 `to_json` 键是 `exposure`/`white_balance`/`environment`/`grading`（`rig.h:508-511`）
    —— 两处都改对之后才真的发 op；`ai_light_candidates.cpp::diff_rigs_emits_one_op_per_changed_global_block`
    现在钉死这一点。教训：`contains()` 类的软查询失败**默认是静默的**，只要可能落空就必须有测试。
    相关的既有说明在 `src\doodle_core\metadata\ai_light\common.h:86`。
58. **`cv::VideoWriter{path, fourcc, fps, size}` 的 fps 形参是 `double`**：传 `std::int32_t` 进花括号初始化
    是收窄转换，MSVC 直接 C2398（`requires a narrowing conversion`）→ 显式 `static_cast<double>(in_fps)`。
59. **`create_move` 要写全限定 `::doodle::detail::create_move`**：工作机文件在 `namespace doodle` 里，
    裸 `detail::` 会先命中 `doodle::http::detail`（`http_client/work.h:31`），报的是 C2039/C3861。

**仍未落地**

- **重新打包 UE 插件包**：`ue_exe.cpp` 的 `installUePath` 每次跑 UE 前都会用服务端的插件包覆盖
  `Engine/Plugins/Doodle` —— 不打包，工作机上还是旧插件，`-DoodleAiLightParams` 没人认、
  `UDoodleLightingRoundRunner` 不存在。（本机不能编译 UE 插件，D 组只做了 API 级核对。）
- **真机一轮回环**：MRQ 在编辑器/PIE 形状下的实际行为、帧数、时长、上传体积都要真机确认。
- **服务端候选指标复算**（D30 只写了一半）：`metrics_` 仍来自回传空值。
- `baseline_level_` 的真实推导仍是 M2 阻断项；前端页面（§14）不在本轮。




---

## 15.17 已落地（批 9：删掉旧的 SceneCapture / commandlet 路径）

> 用户指令原话：**「UDoodleLightingCommandlet 类和相关方法是否已经不使用了?」** → 助手取证答复「是，
> 产品链路零调用者，只剩手工调试的名义身份」并提议删除 → 用户：**「删除,」**。

### 15.17.1 为什么可以删

1. **工作机不再用 commandlet**：`src\doodle_lib\exe_warp\ai_light_render.cpp` 起 UE 时**不带 `-run=`**，
   只传 `-DoodleAiLightParams=<abs json>`（D45/D46）。全 `src\` 里剩下的 `-run=` 只有
   `import_and_render_ue.cpp:223` 与 `export_rig_sk.cpp:109` 的 `-run=DoodleAutoAnimation`。
2. **派发入口是模块钩子**：`script\uePlug\Doodle\Source\DoodleLightingEditor\Private\DoodleLightingEditor.cpp:11-18`
   用 `UDoodleLightingRoundRunner::HasAiLightParams()` → `NewObject` + 挂 `OnPreTick`；模块里没有任何 commandlet
   注册代码（UE 用 `-run=X` 反射 `UXCommandlet`，本来也不需要注册）。
3. **没有任何 include 引用它**：`DoodleLightingCommandlet.h` 只被它自己的 `.cpp` 与自己的 `.generated.h` 包含。
4. 更根本的：候选产物改视频后（§15.16），commandlet 里 MRQ **一帧都渲不出**（`OnBeginFrame` 不广播 +
   `FindCurrentWorld()` 不认 Editor 世界）—— 这条路径即使留着也做不了产品链路要做的事。

### 15.17.2 删了什么（边界）

| 位置 | 处置 |
|---|---|
| `...\DoodleLightingEditor\Public\DoodleLightingCommandlet.h` · `Private\DoodleLightingCommandlet.cpp` | **整文件删除**（165 行；`Main` 在 `:80`） |
| `...\Private\DoodleLightingExecutor.cpp` | 删 `g_warmup_iterations`（原 `:53-55`）、`G_WorldSwapped`、`G_Capture`、`NormalizeLevelPath`、`SwapToWorld`（整段：`LoadObject<UWorld>` + 换 `GWorld` + `FlushLevelStreaming`）、`PrepareRound` / `FinishRound` / `GetCameraView` / `RenderCandidate`（含 `WritePng` / `WriteExr` / 8 次丢弃式预热抓帧）、两处 `G_WorldSwapped = false;`、include `Kismet/GameplayStatics.h` 与 `Misc/PackageName.h`；章节注释「世界与相机」→「相机」 |
| `...\Public\DoodleLightingExecutor.h` | `FRoundContext` **只剩 `FString CameraFilePath`**（删 `BaselineLevel`/`UeProjectPath`/`OutDir`）；删四个声明与 `Math/Rotator.h`·`Math/Vector.h` |
| `...\Public\DoodleLightingCapture.h` · `Private\DoodleLightingCapture.cpp` | 97 → 22 行 / 357 → 35 行：只留 `ApplyFrozenPostProcessBaseline`（逐字保留原赋值体）；删 `FFramePixels`、`FCaptureScope`（Create/Destroy/IsValid/SyncCamera/SetPostProcessSettings/CaptureFrame/GetLastError）、`WritePng`、`WriteExr`、匿名 ns 的 `MakeRenderTarget`/`FExrMemStreamOut`/`EnsureParentDirectory`、`PreviewWidth/PreviewHeight` |
| `...\DoodleLightingEditor.Build.cs` | 删 `ImageCore`/`ImageWrapper`/`Imath`/`UEOpenExr`/`UEOpenExrRTTI` 与 `bEnableExceptions = true`（都是为那个写盘器开的）；`RenderCore`/`RHI` **保留**（本机不能编译 UE，不为纯清洁动作冒 MRQ 传递依赖的风险） |
| `...\Private\DoodleLightingRoundRunner.cpp` | `:347-351` 的 `FRoundContext` 赋值只留 `CameraFilePath`（`Payload.BaselineLevel`/`UeProjectPath`/`OutDir` 在 wire 上仍然存在、仍然要用，只是不再进 `FRoundContext`） |
| `...\Public\DoodleLightingEditor.h` | 头注释改写 + 一句「`UDoodleLightingCommandlet` 已删除」 |
| 模块规模 | 编辑器模块 **11 → 9 个文件**；两模块合计 **20 个文件**（`DoodleLighting` 11 + `DoodleLightingEditor` 9） |

**刻意保留（不能跟着删）**：`DoodleLightingCapture::ApplyFrozenPostProcessBaseline`（执行器匿名辅助
`BuildPostProcessSettings` 在 `:609` 调它，live 路径 `ApplyCandidateToWorld` 在 `:822` 用它 —— 这是
`DoodleLightingCapture.cpp` 还活着的唯一原因）、`PrepareAnchorCamera` / `ApplyCandidateToWorld`、
`ImportDeliveryCamera` / `ReleaseDeliveryCamera` / `BuildRigLights` / `EnsurePostProcessVolume` / `G_World`、
以及 `CollectActor` / `DestroyAiSpawnedActors`（内含 `CommandletHelpers::TickEngine(G_World)`，所以
`CommandletHelpers.h` 的 include 仍要留）。

### 15.17.3 D37 的改述

批 4 的 D37 写的是「`LoadObject<UWorld>` 后换 `GWorld`」—— 换世界那步随 `SwapToWorld` 一起删了。
现在的真实形状：**关卡由载荷 `render_map` 经 `LoadObject<UWorld>(nullptr, *NormalizePackagePath(...))`
取来（`DoodleLightingRoundRunner.cpp:324`），交给编辑器打开跑 PIE**；不 `DuplicateAsset` / 不
`SaveLoadedAssets` / 不 `MarkPackageDirty` 这三条不变。（`:79` 的 `NormalizePackagePath` 是 runner 自己的，
与被删的 `NormalizeLevelPath` 无关。）

### 15.17.4 顺带修掉的过时注释

- `src\doodle_lib\exe_warp\ai_light_render.h:5`：「只起一次 UE（DoodleLighting commandlet）」→
  「不带 `-run=`，进程靠插件模块的 `UDoodleLightingRoundRunner` 驱动（D46）」。
- `src\doodle_lib\http_method\local\event.cpp:31-36`：同样从「起一次 UE 的 DoodleLighting commandlet 全渲染」
  改成「不带 `-run=`，进程内由插件模块驱动 MRQ 在 PIE 世界里逐候选出帧」。

### 15.17.5 仍未落到真机

本次是**纯删除 + 注释**（C++ 侧只动了两个注释），`Ninja_debug` 与 `test_main` 回归照跑；
但 UE 插件侧本机**不能编译**，删除的正确性只做到「全仓 grep 无残留引用 + API 级核对」。
真机验证前仍必须**重新打包插件包**（`ue_exe.cpp` 的 `installUePath` 每次跑 UE 前都会覆盖
`Engine/Plugins/Doodle`）。






