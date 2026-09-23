# LLM 驱动的 UE 灯光工具 —— 方案设计与软件架构

> 状态：**方案设计 v3.1**（输入形态已澄清，见 §1.1、§2）
> 环境：UE 源码树 `E:\UnrealEngine`（5.7.4，已完整编译）、UE 工程 `E:\doodle_plug_dev_5`、Doodle 插件源码 `E:\Doodle\script\uePlug\Doodle`
> 输入：**人工提示词**（氛围 / 角色性格 / 故事上下文 / 背景设定）+ **多张图片参考**（同场景不同角度、经 AI 或 PS 调整过的目标效果图）+ 可选参考视频
> 输出：对场景灯光、环境光与后处理的增 / 删 / 移动 / 改参，使渲染氛围接近输入意图

---

## 1. 核心判断

**判断一：纯 LLM 开环做不到，必须闭环。**
LLM 能可靠决定「要有主光、来自左前方 45°、冷色、硬光、夜景霓虹」这类**语义与结构**，但无法预测「强度 2.3 还是 5.7」这类**连续数值**。渲染链路上还有自动曝光、Lumen 间接光、材质反照率等强非线性环节，任何数值猜测都会被打偏。

**判断二：不让 LLM 直接生成 UE Python/C++ 或直接调 API，而是定义 IR。**
否则不可复现、不可校验、不可撤销、不可 diff、不可回归测试，也无法在没有 LLM 的情况下单测执行器。

**判断三：交互优先，但第一天就用同一套 IR 和同一个执行器承载两种模式。**
既然最终要推到农场，交互模式就不能是「先做个编辑器小工具、以后再重写一遍」。做法是：执行器只有一份（`apply_ops` 等原语），交互模式下它跑在编辑器进程内被 Slate 面板直接调用，自动模式下它被包进 commandlet 由农场调用。IR 既是两者之间的传输载荷，也是交互会话的记录格式——**会话结束时导出的 rig 就是农场任务的输入**，不存在转换环节。

**判断四：参考是「美术调过的目标效果图」，不是物理真值；用显示空间的感知统计去匹配它，是正解而不是妥协。**
参考图是原始渲染经 AI 或 PS 调整后的效果，局部加减光、调色、AI 重绘都破坏了 `E ≈ πL/ρ` 的前提，物理逆渲染会解出错误的辐照度；工程里也没有对应相机，位姿要自己解且同样被 AI 编辑破坏。**但美术调整的恰恰就是「调色板、亮度分布、对比度、色温」这些感知量**——匹配这些量，正是匹配美术的意图。

**判断五：后处理与调色必须在可改范围内，否则参考的一部分目标不可达。**
参考的目标里有一部分（暗角、局部对比、色调曲线、光晕）**只能靠调色实现，靠灯永远够不到**。执行器必须能同时写灯光、环境光、雾、后处理与调色参数。

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
| 2 | **优先美术在编辑器内交互调整**，之后转为农场自动任务 | 交互模式是 M1；执行器必须能在编辑器进程内调用；IR = 会话记录 |
| 3 | **LLM 使用云端 deepseek** | 文本与视觉同一把 key（`deepseek-flash` 支持图片输入）；无需本地模型 |
| 4 | **允许修改环境中所有灯光** | 去掉「只动自己管理的灯」护栏；改为全场景快照 + 完整回滚 + 事务；环境光/雾/天空全在可改范围内 |
| 5 | **后处理/调色必须可改**（判断五） | 从"允许"升级为"必需"；执行器需有 grade 写入原语；Spec 要区分"灯光负责"与"调色负责" |
| 6 | **主判据在显示空间**（色调映射后） | 修正 v2 的「线性 EXR 用于指标」规则；EXR 降为工程检查用途 |

---

## 3. 总体架构

```
┌──────────────────────────────────────────────────────────────────────────┐
│  输入：人工提示词 · 多张图片参考（同场景异视角，AI/PS 调过）· 可选视频      │
└────────┬──────────────────────────────────┬──────────────────────────────┘
         │                                  │
┌────────▼────────────────┐   ┌─────────────▼──────────────────────────────┐
│ 提示词 → intent          │   │ 多张参考图 → 感知统计 → 聚合 → targets      │
│  · LLM 归一化受控词表     │   │  · 显示空间调色板 / 亮度分位 / 对比 / 色温  │
│  · 角色性格 → 灯光语言映射│   │  · 后处理痕迹（暗角/提黑/光晕）             │
│  · 故事上下文 / 背景设定  │   │  · 多图逐维度聚合 + 冲突检测                │
└────────┬────────────────┘   └─────────────┬──────────────────────────────┘
         └───────────────┬──────────────────┘
                         │
              ┌──────────▼───────────┐
              │ LightingSpec          │  ← 一等工件：意图 + 目标区间 + 置信度
              │ 人工核对闸门（必过）  │
              └──────────┬───────────┘
                         │
┌────────────────────────▼─────────────────────────────────────────────────┐
│ ② 规划层 Planner（deepseek + 灯光预设库检索）                             │
│  · 提示词优先于参考的冲突规则                                             │
│  · 检索 top-k 灯光预设（RAG over Rig）→ 候选骨架                          │
│  · function calling → LightingPlan = 有序 op 列表 + rationale             │
│  · schema 约束 + 性能护栏                                                 │
└────────────────────────┬─────────────────────────────────────────────────┘
                         │  LightingRig IR (JSON, 版本化)
        ┌────────────────┴────────────────────────┐
        │                                         │
┌───────▼──────────────────────────┐   ┌──────────▼───────────────────────┐
│ ③A 交互模式（M1，优先）           │   │ ③B 自动模式（M6，农场）           │
│  Slate 面板（编辑器进程内）        │   │  UnrealEditor-Cmd                 │
│   · 直接调用执行器（同进程）       │   │  -run=DoodleLighting -Params=<json>│
│   · 视口实时看效果                 │   │   · 同一执行器、同一 IR            │
│   · 逐个 op 接受/拒绝/撤销         │   │   · 无人在环 → 全自动闭环          │
│   · 会话 = IR 版本序列             │   │   · 分布式派发                     │
│  Doodle 服务 ← WebSocket 取方案    │   │                                    │
└───────┬──────────────────────────┘   └──────────┬───────────────────────┘
        │                                         │
        └────────────────┬────────────────────────┘
                         │  渲染预览（PNG 显示空间 = 主判据；EXR = 工程检查）
┌────────────────────────▼─────────────────────────────────────────────────┐
│ ④ 评判层 Critic（双判据）                                                 │
│  · 判据 A：显示空间感知指标 vs Spec.targets 区间 → 满足率 + 方向一致性     │
│  · 判据 B：交付机位 VLM 语义判分（氛围/性格/故事是否成立）→ 结构化 delta   │
└────────────────────────┬─────────────────────────────────────────────────┘
                         │
┌────────────────────────▼─────────────────────────────────────────────────┐
│ ⑤ 优化器 Optimizer（仅自动模式必须；交互模式由美术的眼睛替代）             │
│  · 灵敏度标定（少量探针渲染得局部雅可比）→ 解析调整 → 残余坐标下降         │
│  · 分阶段降维：先全局（曝光/整体强度/色温/调色）→ 再逐灯；主光优先          │
│  · warm start：复用上一版 rig                                              │
└──────────────────────────────────────────────────────────────────────────┘
```

---

## 4. 数据契约

### 4.1 场景事实 SceneFacts（由 UE 侧 `inventory` 原语产出，喂给 LLM）

```jsonc
{
  "schema_version": "1.0",
  "level": "/Game/.../RenderMap",
  "sequence": "/Game/.../LS_xxx",
  "frame_range": [1001, 1200],
  "camera": {
    "name": "Camera_01",
    "transform": { "location": [x,y,z], "rotation": [p,y,r] },
    "fov_deg": 35.0,
    "near_clip": 0.5,
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

> `postprocess.color_grading` 是决策 5 新增的可写区域，必须与灯光一起出现在 SceneFacts 与 LightingRig 里。

### 4.2 灯光目标 LightingSpec（一等工件）

**这是 v3.1 的核心新增。** 参考不再直接流向评判层，而是先被蒸馏成一份显式、可核对、可版本化、可回归测试的目标。

```jsonc
{
  "schema_version": "1.1",
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
    "space": "display_srgb",         // 明确指标空间（决策 6）
    "luma_percentiles": {"p10": [0.02,0.06], "p50": [0.10,0.18], "p90": [0.55,0.75]},
    "palette": [ {"color": [r,g,b], "share": [0.20,0.35]}, ... ],
    "contrast": [0.55, 0.75],
    "white_balance_kelvin": [3400, 4200],
    "clip_rate": {"high": [0,0.02], "low": [0,0.05]},
    "grading_hints": {               // 从参考中读出的后处理痕迹（决策 5）
      "vignette": "strong", "lifted_blacks": true, "bloom": "moderate"
    },
    "ownership": {                   // 每个目标由谁负责达成
      "lighting":  ["luma_percentiles", "white_balance_kelvin", "clip_rate"],
      "grading":   ["palette", "contrast", "grading_hints"],
      "shared":    []
    }
  },

  "reference_stats": {               // 参考的处理过程与质量
    "image_count": 4,
    "aggregation": "median",
    "per_dimension_confidence": {"luma_percentiles": 0.85, "white_balance_kelvin": 0.5},
    "conflicts": ["white_balance_kelvin"],   // 冲突维度 → 交人工裁决
    "retouch_detected": true,                // 检测到 AI/PS 调整
    "usable_for_physical_solve": false       // 显式声明：不做物理反解
  },

  "evidence": { /* 每个字段的来源与置信度 */ },
  "human_confirmed": false,
  "tier": "stats_only",              // stats_only | multiview | prompt_only
  "degrade_level": "full|stats_only|prompt_only"
}
```

**为什么这是关键**：参考是模糊的、被改过的、多张且互相冲突的，同一组图可以被读成很多种意图。Spec 把歧义显式化、可核对、可版本化、可回归测试。没有 Spec，闭环就没有可验证的目标。

### 4.3 灯光方案 LightingRig（LLM 输出 / 执行器输入 / 会话记录 / 数据库存储）

```jsonc
{
  "schema_version": "1.0",
  "rig_id": "uuid",
  "session_id": "uuid",                  // 交互会话
  "shot_task_id": "uuid",
  "spec_id": "uuid",                     // 追溯：本 rig 服务于哪份 Spec
  "iteration": 3,
  "provenance": { "model": "deepseek-flash", "prompt_hash": "...",
                  "reference": ["ref_01.png","ref_02.png"], "seed": 12345,
                  "tier": "stats_only" },
  "baseline_snapshot": "snapshot_000",   // 全场景快照 id（决策 4：完整回滚）
  "globals": {
    "exposure": { "method": "manual", "ev100_bias": -0.3 },
    "white_balance": { "kelvin": 5600, "tint": 0.0 },
    "environment": { "sky_light_intensity": 0.6, "fog_density": 0.02 },
    "grading": { "contrast": 1.08, "saturation": 0.95, "lift": 0.01,
                 "vignette": 0.25, "bloom": 0.3 }      // 决策 5：必需可写
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
  "ops": [
    { "op": "add",    "light": "key_01",  "rationale": "提示要求左前方冷主光" },
    { "op": "modify", "light": "art_fill_01",
      "patch": { "intensity": { "unit": "candela", "value": 3.0 } },
      "rationale": "压缩 key:fill 比到 4:1" },
    { "op": "set_grading", "patch": { "vignette": 0.25 },
      "rationale": "参考有明显暗角，属于调色责任（Spec.targets.ownership.grading）" },
    { "op": "remove", "light": "art_rim_02", "rationale": "与参考图无轮廓光" }
  ]
}
```

**为什么 `anchor.space=camera`**：绝对世界坐标对 LLM 无意义（它不知道场景尺度），而「相机左前方 45°、抬高 30°、距离 3m」是电影布光的通用语言，且对场景尺寸鲁棒。执行器负责换算成世界坐标。

**为什么 `id` 与 UE actor 名解耦**：美术会重命名、复制、删除 actor。语义 id 是稳定主键，映射表存在 UE 侧（`Tags` 标记）或 Doodle 数据库。

---

## 5. ① 感知层 Perception

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
     参考给环境与调色的目标区间；提示词给意图与角色照明
  ↓ ⑤ 人工核对闸门（必过）
     呈现「我读到的意图 + 参考提取的目标」给美术确认
  ↓ 输出 LightingSpec
```

这条路与「工程里没有相机」的现实吻合，也与「参考是目标效果图」的定位吻合。

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

**受光方向为什么只作软提示**：PS 的加减光和渐变会伪造方向线索。在交互模式下由美术直接指定；在自动模式下由提示词与预设库决定。

### 5.3 多图聚合与冲突检测

参考必有多张，聚合是一等步骤：

1. 逐图提取统计量
2. 逐维度取中位数（或按面积/清晰度加权）
3. **冲突检测**：维度内离散度超过阈值 → 标记 `conflicts`，**不自动平均掉**
4. 冲突维度进入人工裁决（Spec 确认闸门），或降级为「该维度不设目标」

### 5.4 视频 → 关键帧序列

```
分镜检测 → 每镜关键帧(首/中/尾 + 内容变化帧) → 并入多图聚合流程
        → 若关键帧间统计量单调变化 → 输出时序曲线（用于灯光动画）
```

视频不再有独立的处理路径，而是**作为多图来源**统一处理。若视频里相机在动，相机运动导致的统计变化会被聚合的鲁棒性吸收；需要区分时用光流做一次粗筛。

### 5.5 人物角色性格 → 灯光语言映射表（新增）

提示词里最有价值、也最容易落不到灯上的字段。必须结构化。

| 性格维度 | key:fill 比 | 硬软 | 光位倾向 | 色温倾向 |
|---|---|---|---|---|
| 阴郁 / 内向 | 高（6:1–10:1） | 硬 | 侧高、少正面 | 偏冷 |
| 开朗 / 外向 | 低（2:1–3:1） | 软 | 正面偏上 | 偏暖 |
| 威严 / 压迫 | 很高（10:1+） | 硬 | 顶光 / 底光 | 中性偏冷 |
| 温柔 / 脆弱 | 很低（1.5:1–2:1） | 很软 | 正面、包覆 | 暖 |
| 危险 / 不稳定 | 高且不稳定 | 硬 | 侧后、明暗交界 | 冷暖冲突 |
| 神圣 / 超然 | 中 | 很软 | 顶光 | 冷白 |

映射结果写入 `intent.character_personality[].lighting_language`，直接驱动规划层的角色灯参数。

### 5.6 Tier B（可选增强，M7）

多张参考是常态，所以多视角联合约束是**可用的最强信号**：一套灯光必须同时解释所有视角。

- 未知量：光照表示 + 每张参考的相机位姿 + **每张图各自的调色变换（nuisance 参数）**
- 把 AI/PS 的**全局调色**吸收进 per-image nuisance，光照成为多图共享的解释
- 局部改图（加减光、重绘）作为外点 → 鲁棒损失（Huber / trimmed）
- 位姿来源三级：render-and-compare 重定位 → **人工对位（美术在编辑器里为每张参考摆一个相机，每张 1–2 分钟，最务实可靠）** → 放弃

**建议**：Tier A 先落地并跑顺；Tier B 只在 Tier A 的目标区间匹配明显不够时才上。不要一开始就做联合优化。

---

## 6. ② 规划层 Planner

### 6.1 LLM 的职责边界

| LLM 做 | LLM 不做 |
|---|---|
| 决定灯的**数量、角色、类型、相对方位** | 决定最终精确强度/色温/距离 |
| 决定**环境光/雾/天空/曝光/调色**的整体倾向 | 直接写 UE API 调用或代码 |
| 决定**增删改**哪些灯（结构决策） | 逐帧关键帧的精确数值 |
| 读 VLM/美术反馈，给出**结构级修正** | 预测渲染结果 |
| 选择/组合**预设模板** | 绕过 schema 自由输出 |

### 6.2 提示词与参考的冲突规则

**提示词优先于参考图。** 参考只提供「环境与场景的结构与数值」，提示词提供「意图与语义」。

例：提示词说「悲伤、冷调」而参考偏暖 → 以冷调为准，只从参考取环境光水平与对比度。

规则落点：

| 情况 | 处理 |
|---|---|
| 提示词与参考一致 | 直接合并，置信度提升 |
| 提示词明确、参考冲突 | 提示词胜出，冲突写入 op 的 `rationale` |
| 提示词未提及、参考明确 | 参考胜出 |
| 提示词未提及、参考冲突 | 不设目标，交人工裁决 |

### 6.3 知识注入：灯光预设库（Rig Library）

第一批预设直接来自项目已有资产：`ADoodleAssetsPreview` 的 `LowContrast / MidContrast / HighContrast` 三档灯光环境（含 SkyLight + DirectionalLight + PostProcess 的成套配置）。

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

流程：**提示词 intent + 参考统计目标 embedding → top-k 预设 → 作为 LLM 候选骨架 → LLM 裁剪/组合/微调**。参考模糊时，这是把意图落地为物理合理骨架的**主路径**，并大幅降低 LLM 的输出维度。

### 6.4 工具面（function calling schema）

| 工具 | 作用 | 副作用 |
|---|---|---|
| `inventory_lights()` | 读当前灯光与环境清单 | 无 |
| `query_scene(anchor)` | 取对象包围盒/位置/朝向 | 无 |
| `list_presets(query)` | 检索预设 | 无 |
| `build_spec(prompt, refs)` | 生成 LightingSpec | 无 |
| `confirm_spec(spec)` | 人工确认闸门 | 有（解锁后续） |
| `add_light(spec)` | 新增一盏灯 | 进 op 队列 |
| `set_light(light_id, patch)` | 改参数 | 进 op 队列 |
| `remove_light(light_id)` | 删灯 | 进 op 队列 |
| `set_globals(patch)` | 曝光/白平衡/雾/天空 | 进 op 队列 |
| `set_grading(patch)` | 后处理与调色（决策 5） | 进 op 队列 |
| `render_preview(camera, frames)` | 出预览图 | 重 |
| `score(spec)` | 按 Spec 打分 | 无 |
| `commit(rig)` | 落库 + 写序列 | 有 |

所有写操作先进 **op 队列**，由执行器统一校验后应用 —— 天然获得 dry-run、diff、逐步审阅、回滚。交互模式下这正是美术逐个接受/拒绝的粒度。

### 6.5 护栏（硬约束，schema 层面强制）

决策 4 已放开「不能改美术灯」，但以下仍然强制：

- 灯数量上限、动态阴影灯数量上限（性能预算）
- 强度/色温/角度/距离的取值边界
- 光照通道约定（沿用现有 `OnCreateDirectionalLight` 的 `SetLightingChannels(false, true, false)` = Channel1 给主光）
- **任何变更前必须生成全场景快照**，支持一键完整回滚（因为现在什么都能改，回滚是唯一的安全网）
- 禁止破坏已有子序列绑定（`AddSequenceWorldToRenderWorld` 建立的层级）
- 曝光必须为手动（见 §7.3）
- **调色不得吞掉灯光差异**：若调色的改动使灯光指标失去区分度，拒绝该 op 并提示（防止用调色"作弊"通过指标）

---

## 7. ③ 执行层 Executor（UE 侧）

### 7.1 一份执行器，两种宿主

**核心决定：执行器不依赖 Remote Control，也不依赖命令行。**

- **③A 交互模式（M1，优先）**：Slate 面板与执行器同在编辑器进程内，**直接函数调用**。没有 IPC、没有序列化开销、没有 Remote Control 的插件依赖与端口管理。面板负责：输入提示 / 上传参考 / 展示 op 列表与 diff / 逐个接受拒绝 / 撤销 / 与参考图并排比对。
  - 面板 ↔ Doodle 服务（取 LLM 方案、落库、取历史）：复用插件已有的 WebSocket 先例 `DoodleNetWork`（`ws://localhost:50024/socket.io/`，见 `Source/BatchRender/Public/DoodleNetWork.h`），扩展一套 lighting 消息类型。
- **③B 自动模式（M6，农场）**：同一个执行器包进 commandlet：

```
UnrealEditor-Cmd.exe <uproject> -windowed -log -Unattended \
    -run=DoodleLighting -Params=<json>
```

由现有 `import_and_render_ue.cpp:221-225` 的命令行组装点扩展一个分支即可。

> `RemoteControl`（`Engine/Plugins/VirtualProduction/RemoteControl`）仍然可用，但仅作为**外部进程驱动编辑器**的可选补充（例如外部 agent 调试），不作为主路径——主路径不需要它。

### 7.2 执行原语

| 原语 | 说明 |
|---|---|
| `snapshot` | 全场景灯光/环境/**后处理与调色**状态快照（决策 4、5 的安全网） |
| `restore` | 从快照完整回滚 |
| `inventory` | 扫描关卡 → 输出 SceneFacts（含 `postprocess.color_grading`） |
| `apply_ops` | 按 op 列表增删改（含 `set_grading`），全程 `FScopedTransaction` + `Modify()` |
| `render_preview` | 低分辨率预览：**PNG（显示空间，主判据）** + EXR（线性，工程检查） |
| `export_rig` | 导出当前 rig 为 IR（支持从美术手工灯反向生成 IR → 作为会话起点与预设来源） |
| `commit` | 写入 Level Sequence 关键帧轨道 + 落库 |

### 7.3 实现要点（血泪项）

1. **幂等**：同一 rig 重复 apply 结果必须一致。
2. **`ClearAllLight()` 不能直接复用**：现有实现（`DoodleAutoAnimationCommandlet.cpp:535`）无条件遍历销毁所有 `ALight` + `APostProcessVolume`。交互模式下这是灾难性的。改为：先 `snapshot`，再按 op 精确增删，`restore` 兜底。
3. **曝光必须手动锁定**：现有 `PostProcessVolumeConfig()` 已把 `AutoExposureMinBrightness/MaxBrightness` 固定为 1.0、`AutoExposureBias=0.0`。这是对的，**闭环期间绝不能放开** —— 否则调强度会被自动曝光吃掉，闭环直接失效。
4. **Lumen 参数必须固定**：`LumenFinalGatherQuality=4` 等已在代码里固定，闭环期间不要变动，否则指标漂移。
5. **单位显式**：`ULightComponent::SetIntensityUnits(ELightUnits::Candela/Lux/EV)`，IR 里也带单位，避免流明/坎德拉歧义。
6. **环境光也算灯**：`SkyLight / SkyAtmosphere / ExponentialHeightFog / VolumetricCloud` 纳入 IR `environment`。夜景氛围一半来自它们，且决策 4 明确允许改。
7. **调色是可写区域**（决策 5）：`globals.grading` 要能落到 PostProcessVolume 的 Color Grading / Bloom / Vignette 等字段；这是参考目标里"调色负责"那部分的唯一实现途径。
8. **时间维度**：Level Sequence 光照轨道写关键帧；可复用项目已有的 `DoodleLightningPost` / `DoodleCluster` 轨道序列化能力。
9. **交互延迟预算**：面板操作要 <100ms 生效（纯属性写入）；`render_preview` 是唯一重操作，异步 + 可取消，美术可随时中断。

### 7.4 模块落点

新增两个模块，**不污染现有 `DoodleEditor`**：

- `DoodleLighting`（Runtime）：IR + Spec 数据结构、JSON 序列化、校验器、op 队列语义、快照/回滚、指标计算（可脱离 UE 单测）
- `DoodleLightingEditor`（Editor）：执行器、Slate 面板、预览渲染、WebSocket 客户端、命令

---

## 8. ④ 评判层 Critic

### 8.1 双判据

| 判据 | 位置 | 内容 | 用途 |
|---|---|---|---|
| **A 目标区间满足** | 服务端（可离线复算） | 显示空间感知指标 vs `Spec.targets` 区间 | 自动模式的收敛判据；回归基线 |
| **B 语义判分** | 服务端（VLM） | 氛围 / 角色性格 / 故事是否成立 | 捕捉区间覆盖不到的语义偏差 |

判据 A 是**客观、可复现、可进 CI** 的；判据 B 是**主观、不可复现**的，只作辅助与人工参考。**绝不用 B 当唯一收敛判据。**

### 8.2 指标空间（修正 v2 的规则）

- **主指标在显示空间（色调映射后）计算**：参考是美术在显示空间里调出来的成片，它的亮度分布、对比、调色板都是显示空间的量。拿线性空间的渲染去比显示空间的参考，必然对不上。
- **EXR（线性）只用于工程检查**：曝光余量、裁剪、灯光强度是否超出物理合理范围。不参与主指标。

> 这条修正了 v2 的「线性 EXR 用于指标、PNG 用于 VLM」。原因见 §1 判断四。

### 8.3 指标清单与"满足率"

| 指标 | 目标形式 | 责任方 |
|---|---|---|
| 调色板分布 | EMD / 占比区间 | 调色为主，灯光为辅 |
| 亮度分位 p10/p50/p90 | 区间 | 灯光（曝光与强度） |
| 对比度 | 区间 | 调色 + 灯光 |
| 白平衡 / 色温 | 区间 | 灯光 |
| 高光/暗部裁剪率 | 区间 | 灯光 |
| 暗角 / 提黑 / 光晕 | 定性痕迹 | 调色 |

**收敛判据 = 区间满足率**（落入目标区间的指标数 / 总指标数）+ **方向一致性**（指标变化方向是否正确）。这比"到某个绝对值的距离"更适合区间型目标，也更鲁棒。

### 8.4 VLM 判官

输入：`intent` 描述 + 参考图 + 当前渲染 → 输出 `score 0–10` + **可执行的自然语言差异**。关键设计：自由文本必须再经一次结构化映射（LLM）转成 `intent` 字段的 delta，才能进入下一轮规划。

**工程优化**：参考图先用 DeepSeek [Files API](https://api-docs.deepseek.com/zh-cn/guides/vision/) 上传一次拿 `file_id`，闭环内每轮引用而不重复上传（同一张图在几十轮迭代里会被反复送进 VLM）。

### 8.5 loss

```
L = w1·palette + w2·luma + w3·contrast + w4·white_balance
  + w5·clip_rate + w6·(1 − VLM_score/10) + w7·constraint_penalty
```

权重需在项目自有 A/B 盲测集上校准（指标与人工评分的相关性）。**`w7` 要包含"调色作弊"惩罚**：若灯光本身几乎没变而调色大改，视为未达成。

---

## 9. ⑤ 优化器 Optimizer

**仅在自动模式（③B）必须**——交互模式下美术的眼睛就是评判器，不需要 loss。

优化目标从 v2 的「物理量」改为「**显示空间的感知统计量**」。

### 9.1 主算法：灵敏度标定 + 解析调整

```
① 少量探针渲染（每个待调参数各扰动一次）→ 局部雅可比 J = ∂指标/∂参数
② 解线性系统：在 J 的线性近似下，求使指标进入目标区间的最小参数改动
③ 1–2 次验证渲染
④ 残余偏差 → 坐标下降微调
```

按责任方分阶段：

1. **先全局**：曝光 EV、白平衡、调色（对比/饱和/提黑/暗角）—— 这些对指标影响最大且近乎解耦
2. **再逐灯**：按 `role` 分组，主光优先；用 key:fill 比的目标解强度比
3. **最后细节**：source radius / cone angle / 软硬

### 9.2 其余要点

- **warm start**：同 shot 多版本是常态，直接用上一版 rig（或美术在交互模式里定稿的 rig）作初值
- **并行候选**：分布式派发同时评估多个候选
- **停止条件**：区间满足率达标 / Δloss < ε / 迭代上限 / 人工接受
- **不要用黑箱搜索当主算法**：区间目标 + 灵敏度标定比 CMA-ES 快一个量级；黑箱只用于残余

---

## 10. Doodle 服务侧集成

### 10.1 现有可复用资产

| 现有资产 | 复用方式 |
|---|---|
| `exe_warp/ue_exe.cpp::async_run_ue` | UE 进程启动、日志回灌、超时、插件版本安装 |
| `exe_warp/import_and_render_ue.cpp:221-225` | 命令行组装点，新增 `-run=DoodleLighting` 分支 |
| `DoodleAutoAnimationCommandlet.cpp` | 灯光执行逻辑起点（`OnCreateDirectionalLight` / `ClearAllLight` / `PostProcessVolumeConfig`） |
| `ADoodleAssetsPreview` | 预设库第一批条目（三档对比度环境） |
| `Source/BatchRender/Public/DoodleNetWork.h` | 编辑器 ↔ 服务 WebSocket 通道先例（socket.io） |
| `http_method/kitsu/auto_task.h::shot_render_light_builder` | 镜头灯光资产路径装配 |
| `kitsu_ctx_t::deepseek_keys_` + `/api/doodle/deepseek/key` | LLM 密钥来源（文本 + 视觉同一把 key） |
| `http_client/ai_client_base.h` | AI 客户端抽象基类 → 新增 `deepseek_client`（chat/vision，OpenAI 兼容格式） |
| `http_client/transfer_station_client` | 参考图/视频的生成或预处理 |
| `server_task_info_type` + 分布式契约 | 新任务类型派发 |

### 10.2 新增任务类型与数据表

- 任务类型：`server_task_info_type::ai_light`（或复用 `auto_light` 加 stage 字段）
- 表：
  - `ai_light_job`：输入（提示、参考文件、相机、场景）、状态、迭代轮次
  - `ai_light_spec`：**新增**，LightingSpec（含 `tier` / `degrade_level` / `conflicts` / `human_confirmed`）+ 多参考图与聚合中间产物
  - `ai_light_session`：交互会话（IR 版本序列、美术的接受/拒绝记录）
  - `ai_light_rig`：版本化 IR + 预览图 + 指标 + 是否被接受
  - `light_preset`：预设库 + embedding
- 资产路径复用 `get_shots_auto_lighting_upload_path` / `get_shots_auto_lighting_upload_movie_path`（`core/entity_path.cpp:425-445`）

### 10.3 分布式派发必须遵守的既有契约

来自既有实现的教训（`computers.cpp` / `work.cpp`）：

- 派发判据 = 客户端上报 `online` **AND** 服务端自有事实「库里没有 `status_=running` 且 `run_computer_id_` 指向该机的任务」
- 客户端用 `running_task_count_` 计数，归零才报 `online`
- **任务结束前必须 `put_job_info` 上报终态**，否则任务永远留在 running 挡住该机器

灯光闭环会产生大量短任务（每次预览渲染都可能是一条），这条契约是硬性要求。

### 10.4 前端

参考文件上传、Spec 确认与裁决、预设库管理、任务与版本浏览、A/B 对比。**提示输入与交互调灯的主战场在 UE 编辑器面板内**（决策 2），网页端负责素材、Spec 裁决、评审与任务管理。

---

## 11. 分阶段里程碑

| 阶段 | 内容 | 完成判据 |
|---|---|---|
| **M0 地基** | IR + Spec schema、双端序列化、执行器原语（**含 `set_grading`**） | 手工 Spec → 应用 → 出图 → 回滚到原始状态；幂等性单测通过 |
| **M1 交互闭环** | Slate 面板 + 执行器同进程 + 提示词→intent + Spec 确认 + 逐个 op 接受/拒绝/撤销 | 美术能在编辑器里用自然语言改灯，能核对系统对提示词的理解，全程可撤销 |
| **M2 参考驱动** | 多图统计聚合 + 冲突检测 + 合并成 Spec + 人工裁决 | 目标区间稳定、冲突能被人正确裁决、`degrade_level` 判定正确 |
| **M3 会话 → 任务** | 交互会话导出为农场任务 IR；`-run=DoodleLighting` 分支打通 | 交互定稿的 rig 原样在农场跑出同结果 |
| **M4 双判据评判** | 显示空间区间满足率 + 交付机位 VLM 语义判分 | 无人在环时判据可量化、可复现 |
| **M5 自动优化** | 灵敏度标定 + 解析调整 + 残余坐标下降 | 少数几轮内区间满足率达标 |
| **M6 生产化** | 分布式派发、性能预算、回归基线、A/B 盲测校准 | 基线纳入 CI |
| **M7 可选增强** | Tier B 多视角联合反解（含人工对位） | 明显优于 Tier A 时才保留 |

M3 之所以这么早：决策 2 要求「交互优先，之后转自动任务」，而**只要 IR 和会话记录从一开始就是农场任务的输入格式，M3 就几乎是免费的**；反过来如果交互阶段自己造了一套临时状态，M3 就会变成重写。

### 11.1 回归基线（M6，必须重建）

v2 的基线是「与参考图的像素差」，在 v3.1 下不成立。改为四条：

1. **确定性单测**：同一 Spec → 同一 Rig（规划层可脱网复现，黄金用例）
2. **区间满足率**：固定场景 + 固定 Spec → 渲染后指标落入目标区间的比例
3. **人工盲测偏好**：固定场景下多版本 A/B，与指标做相关性校准
4. **退化测试**：故意给不相关参考（纯色块、无关照片），系统必须**不崩且能给出低置信度 Spec**，而不是硬凑

第 4 条专防「参考不可用时的错误自信」。

---

## 12. 关键风险与对策

| 风险 | 对策 |
|---|---|
| LLM 数值不可靠 | 闭环 + 优化器；LLM 只给结构与初值；schema 硬边界 |
| **参考被 AI 改到物理不可解** | **不尝试物理反解**，显式记录 `usable_for_physical_solve=false`；用显示空间统计 |
| **多图统计互相冲突** | 逐维度置信度 + 冲突标记 → 人工裁决；**不自动平均掉冲突** |
| **目标里的调色部分靠灯够不到** | 后处理/调色纳入必需原语；Spec 里区分 `ownership.lighting` / `ownership.grading` |
| **用调色"作弊"通过指标** | `w7` 惩罚项：灯光几乎没变而调色大改视为未达成 |
| 自动曝光 / Lumen 使强度不可预测 | 强制手动曝光、固定 Lumen 参数（已有代码基础） |
| 全场景可改 → 误操作代价高 | 每次变更前 `snapshot`；完整 `restore`；`FScopedTransaction` 撤销栈 |
| 提示词与参考冲突 | 明确规则：提示词优先；冲突记入 rationale 供人工核对 |
| 渲染成本高、迭代慢 | 低分辨率预览、灵敏度标定减少搜索次数、warm start、并行候选 |
| 交互延迟破坏手感 | 属性写入 <100ms；预览异步可取消；面板不阻塞编辑器 |
| 语义歧义（"电影感"） | 受控词表归一化 + 预设检索 + 人工在环选择 |
| **VLM 判分主观性成为主要不确定源** | 判据 B 只作辅助；判据 A 必须独立可复现；A/B 盲测校准 |
| 评测主观 | 项目 A/B 盲测集，校准指标权重与人工评分的相关性 |
| VLM 反复上传同一参考图 | DeepSeek Files API 上传一次，闭环内用 `file_id` 引用 |

### 12.1 三级降级（必须实现，防"参考不可用时的错误自信"）

| 等级 | 条件 | 行为 |
|---|---|---|
| `stats_only` | 多图统计聚合可用（**默认主路径**） | 用显示空间目标区间 + 提示词 |
| `multiview` | Tier B 可用（多视角联合约束） | 增强 |
| `prompt_only` | 参考不可用/完全冲突 | 只用提示词 + 预设库检索 |

---

## 13. 改动归属：四类程序分别负责什么

### 13.0 一张表看懂

| 角色 | 入口 | 一句话职责 | 是否持有 deepseek key | 灯光改动的性质 |
|---|---|---|---|---|
| ① 主服务器 | `kitsu_supplement`（无参数） | 真库 + 全量 API + 任务派发 + **LLM/VLM 调用 + 感知与 Spec 生成** | **是** | 新增：任务类型、提交端点、AI 路由、数据表、预设库 |
| ② 客户端 | `kitsu_supplement --local` | 工位本地后端（内存库 + 本地 API + socket.io） | 否 | 基本无改动（仅插件版本分发被动受影响） |
| ③ 工作程序 | `--local` + POST `/api/actions/local/task/run` | 连服务器 WebSocket、领任务、跑任务、报终态 | 否 | 新增：`run_task` 分派分支 + 新任务类 + allowed 类型 |
| ④ UE 插件 | `E:\Doodle\script\uePlug\Doodle` | 场景内实际增删改灯、**改调色**、渲染、面板 | 否（回调服务器） | 新增两个模块 + 执行器 + 面板 + commandlet 入口 |

**核心结论：改动集中在 ①③④，②几乎不动。** 感知层与 Spec 生成全部归 ①（因为只有它持有 key，且农场模式必须服务端调用）。

---

### 13.1 ① 主服务器（`kitsu_supplement_main::init()` 默认分支）

**现在负责什么**（`launch/kitsu_supplement.cpp:191-222`）

- 打开真实 sqlite 库（`l_args.db_path_`，默认 `C:/kitsu_new.database`）
- `get_register_info()` 从注册表 `SOFTWARE\Doodle\MainConfig` 读 **deepseek keys**、ji_meng 授权、jwt secret、domain；从 `SOFTWARE\Doodle\Email` 读邮件配置
- 构造**全参** `kitsu_ctx_t`（含 `deepseek_keys_`）—— 这是全系统唯一持有 LLM 凭据的地方
- 注册全量路由 `create_kitsu_route_2`：`/api/data/computers`（WebSocket）、seedance2、ue-plugins、前端
- 持有 `l_set.computers_assign_task_ptr_`（服务端派发器单例）

**要改什么**

1. **任务类型**：`server_task_info_type` 加 `ai_light`（`doodle_core/metadata/server_task_info_type.h:20` 附近）。
2. **提交端点**：新增 `/api/actions/projects/{}/shots/{}/ai-light`，照 `http_method/kitsu/distributed_task.cpp:49-74` 的 `actions_projects_shots_run_ue_assembly::post` 写：构造 `server_task_info`，`type_ = ai_light`，`command_ = LightingRig IR`（`command_` 是自由 JSON，可直接承载 IR，无需改表结构），insert 后调 `run_next_task()` + `socket_io::broadcast`。
3. **派发逻辑**：**零改动**。`computers.cpp:376-378` 已按 `computer.allowed_task_types_` 通用过滤。
4. **LLM/VLM 客户端**：新增 `deepseek_client`（继承 `http_client/ai_client_base.h`，OpenAI 兼容格式，同时支持 chat 与 vision）。**必须放在主服务器**——`--local` 的 `kitsu_ctx_t` 是两参构造，没有 key。
5. **AI 路由**（交互面板要回调的）：
   - `/api/doodle/ai/light/spec`：提示词 + 多张参考 → 生成 LightingSpec（含聚合与冲突检测）
   - `/api/doodle/ai/light/spec/confirm`：人工确认/裁决 Spec
   - `/api/doodle/ai/light/plan`：Spec → 返回 op 列表
   - `/api/doodle/ai/light/score`：渲染图 vs Spec → 区间满足率 + VLM 判分
   - `/api/doodle/ai/light/preset`：预设库 CRUD
6. **数据表**：`ai_light_job` / `ai_light_spec` / `ai_light_session` / `ai_light_rig` / `light_preset`，进 `sqlite_database` 的 upgrade 与 ORM 注册。
7. **参考文件接收**：复用 `http_method/up_file.h` 的 `doodle_data_shots_file_auto_light` 模式；DeepSeek Files API 的 `file_id` 缓存也放这里。
8. **闭环归属（重要）**：自动模式的迭代循环**整体放主服务器**，工作机只做「执行 + 渲染」。理由：key 不落到工作机、迭代历史集中可查、评分口径统一。工作机只需把渲染结果回传。

---

### 13.2 ② 客户端（`kitsu_supplement --local`）

**现在负责什么**（`launch/kitsu_supplement.cpp:151-169`）

- `root = D:/sy_maigc`，**内存数据库**（`database_->open()` 无参）
- 授权上下文 + `create_kitsu_local_route()`（`http_method/kitsu.cpp:431-484`）：本地 API 子集、`/api/actions/local/task/run`、socket.io `/events`
- 端口默认 0（随机分配）
- **注意：`kitsu_ctx_t` 两参构造，没有 deepseek keys** → 本地无法直接调 LLM

**要改什么**

1. **基本无改动**。它是 UI 与本地工具入口，不参与灯光执行，也不参与 LLM 调用。
2. **被动受影响的唯一一处**：`exe_warp/ue_exe.cpp:84-128` 的 `installUePath()` 会在每次跑 UE 前从主服务器拉插件包、覆盖 `Engine/Plugins/Doodle`。**新增 `DoodleLighting` / `DoodleLightingEditor` 模块后必须发布新的插件版本包**，否则工作机上模块缺失、`-run=DoodleLighting` 找不到 commandlet。这是最容易漏的一环，且症状是运行期才暴露。

---

### 13.3 ③ 工作程序（`--local` + POST `/api/actions/local/task/run`）

**现在负责什么**（`http_method/local/event.cpp` + `http_client/work.cpp`）

- `actions_local_task_run::post` 创建 `http_work`，`allowed_task_types` **默认 `{export_fbx, auto_light}`**（`event.cpp:32-34`）
- `http_work::async_run()`：连主服务器 `ws://{server_ip}/api/data/computers`，上报 `hardware_id`(SMBIOS 主板 UUID) / `name` / `status` / `allowed_task_types`
- 收到任务 → `run_task()` 按 type 分派到 `run_ue_assembly_distributed` / `export_fbx_arg_distributed` / `depth_estimation_distributed`；不认识的类型直接 `report_task_unsupported` 回 failed
- `running_task_count_` 计数，**归零才报 online**；`base_distributed_task` 析构时 `task_finished()`
- 每个任务必须在 `run()` 结束前 `put_job_info` 上报终态

**要改什么**

1. **`run_task()` 加分支（必须）**：`server_task_info_type::ai_light` → `run_ai_light_distributed`。漏了这一步，任务会被判 unsupported 直接失败。
2. **`allowed_task_types` 默认值**：建议**不要**加进 `event.cpp:32-34` 的默认集合，改由调用方显式传 —— 不是每台机器都有 UE 授权和 GPU，默认放开会让灯光任务被派到跑不了的机器上，然后一直失败。
3. **新任务类**：`run_ai_light_distributed`。可以复用 `run_ue_assembly_base` 的**公共设施**（`create_logger()` 带日志回传 sink、`create_kitsu_client()`、`async_run_ue` 的进程启动/日志回灌/超时/插件安装），但**不能复用 `run_ue_assembly_base::run()` 的整个流程**——它的 command 是装配参数（`run_ue_assembly_arg`），而灯光任务的 command 是 IR。建议抽一个公共基类，或在 `run_ue_assembly_base` 上把「起 UE + 收日志 + 报终态」抽成受保护方法。
4. **交互模式不使用这个角色**（决策 2：编辑器内同进程执行）。只有自动模式（M6）走这里。
5. **不要在工作机上放 key**：自动模式的 VLM 判分走「工作机回传渲染图 → 主服务器评分」（见 13.1 第 8 条）。

---

### 13.4 ④ UE doodle plug（`E:\Doodle\script\uePlug\Doodle`）

**现在负责什么**

六个模块：`doodle`(Runtime) / `DoodleCluster`(Runtime) / `DoodleClusterSequencer`(Editor) / `doodleEditor`(Editor) / `doodleUI`(Editor) / `BatchRender`(Editor)。提供资产导入、批量渲染、序列轨道、编辑器 UI。C++ 侧驱动入口是 `DoodleAutoAnimationCommandlet`（`Main()` 目前只认 `Params` / `ImportRig` 两个 key）。

**要改什么**

1. **新增两个模块**（改 `Doodle.uplugin`）：`DoodleLighting`(Runtime，IR/Spec + 校验 + 快照 + 指标) / `DoodleLightingEditor`(Editor，执行器 + 面板 + 预览)。
2. **IR 与 Spec 双端一致**：UE 侧 `FJsonObject` 解析必须与 C++ 侧 nlohmann 序列化严格对齐（单位枚举、`anchor.space`、`origin`、`ownership`）。建议落一份 JSON Schema，两端各写校验器，CI 跑同一组 golden 用例。
3. **交互面板**（M1 核心）：提示输入、参考导入、**Spec 核对与冲突裁决**、op 列表与 diff、逐个接受/拒绝、撤销、与参考图并排比对。
4. **WebSocket 客户端**：复用 `Source/BatchRender/Public/DoodleNetWork.h` 的 `IWebSocket` 模式，新增 lighting 消息类型，**连主服务器**（不是本地客户端）。
5. **执行器原语**：`snapshot / restore / inventory / apply_ops（含 set_grading）/ render_preview / export_rig / commit`。
6. **必须改的现有实现**：
   - `ClearAllLight()`（`DoodleAutoAnimationCommandlet.cpp:535`）无条件销毁所有 `ALight` + `APostProcessVolume`，不能用于交互模式
   - `PostProcessVolumeConfig()` 的手动曝光（AutoExposure 固定 1.0/1.0）必须保持
   - `OnCreateDirectionalLight()` 的 `SetLightingChannels(false, true, false)`（Channel1 主光）要成为 IR 默认约定
7. **调色写入能力**（决策 5）：`globals.grading` 要能落到 PostProcessVolume 的 Color Grading / Bloom / Vignette；这是参考目标里"调色负责"部分的唯一实现途径。
8. **commandlet 入口**：建议在 `Params` 的 JSON 里加字段，而不是新增命令行 key，减少分支。
9. **`render_preview` 双输出**：PNG（显示空间，**主判据**）+ 线性 EXR（工程检查）。MovieRenderPipeline 配置需相应调整。
10. **插件版本发布**：模块变更后必须发新版本包（对应 13.2 第 2 条）。

---

### 13.5 一次交互的完整调用链（M1）

```
美术在 UE 面板输入提示词 + 拖入多张参考图
  → [④ 插件] 面板收集 SceneFacts（inventory）+ 参考图
  → [④→①] WebSocket/HTTP 到主服务器
  → [①] 提示词 → intent；多图 → 统计聚合 + 冲突检测 → LightingSpec
  → [①→④] 返回 Spec
  → [④] 面板展示 Spec 供核对：「我读到的是：黄昏、暖色主光来自右后方、高对比……对吗？」
        冲突维度（如色温）在此裁决
  → 美术确认 → [④→①] confirm_spec
  → [①] 预设库检索 + deepseek 规划 → 返回 op 列表(IR)
  → [④] 面板展示 op 列表与 diff
  → 美术逐个接受 → 同进程直接 apply_ops（FScopedTransaction，可撤销）
  → [④] 视口实时生效；点"对比参考" → 本机渲染预览（PNG）→ 回传
  → [①] 区间满足率 + VLM 判分 → 返回可读差异
  → 美术继续说"再冷一点" → 回到规划步
  → 定稿 → [①] 会话 IR 落库
```

### 13.6 自动模式的完整调用链（M6）

```
[①] 从 ai_light_session 取定稿 IR（或从零生成）→ 建 ai_light 任务 → insert + run_next_task
  → [③] WebSocket 收到任务 → run_ai_light_distributed
  → [③] async_run_ue: -run=DoodleLighting -Params=<IR json>
  → [④] 执行器 apply_ops → render_preview(PNG + EXR)
  → [③] 回传渲染结果 + put_job_info
  → [①] 区间满足率 + VLM 判分 → 未达标则生成下一轮 IR → 再建任务（循环，工作机无 key）
  → [①] 达标 → 落 ai_light_rig + 通知前端
```

---

## 14. 浏览器前端负责什么

### 14.1 现状：它是什么

- 前端是 **Kitsu 的 fork**（cgwire 开源动画协作平台），Vue 3 + Pinia + vue-router + Bulma + Vite。
- 由主服务器作为**静态资源**托管：`http_method/kitsu/kitsu_front_end.cpp` 从 `root_path_`（= `kitsu_ctx_t::kitsu_front_end_path_`，生产 `D:/kitsu/dist`）读文件，非 `/api` 路径回落到 `index.html`（SPA 兜底）。注册在 `create_kitsu_route_2` 的**最后一行**（`kitsu.cpp:425`）。
- 已经有一层很厚的 Doodle 扩展（构建产物里 `/api/doodle/*` 出现 71 处）：`ai_image`、`model_library/*`、`pictures/*`、`task/*`（含 log / inspect / restart）、`computing_time/*`、`attendance/*`、`deepseek/key`、`key/ji_meng`、`tool/version`。
- **已经会提交 UE 装配任务**：`/api/actions/projects/{project_id}/shots/{id}/run-ue-assembly`。
- **已经在浏览器里直接调 LLM**：产物里打包了 OpenAI JS SDK（`OpenAI` 出现 68 处），key 从 `/api/doodle/deepseek/key` 取（对应 `http_method/other/other.cpp:21`）。

### 14.2 决定性的约束：浏览器看不到 UE 视口

这条约束决定职责划分。若要让浏览器做「实时改灯」，必须额外补两样东西：

1. 一条**服务器 → 编辑器**的反向命令通道（编辑器进程常驻并主动连服务器）
2. **视口回传**（低帧率截图推送，或上 UE Pixel Streaming）

这会把 UE 编辑器降格成「被远程驱动的渲染服务」，延迟与复杂度都显著上升。

所以按「交互优先」的决策，正确切法是：

- **浏览器管「方案与评审」（异步面）**
- **UE 面板管「现场执行」（同步面）**
- 两者共用同一份 IR、Spec 和会话记录

### 14.3 浏览器前端负责的内容

| 能力 | 内容 | 复用 / 新增 |
|---|---|---|
| 任务提交 | 选镜头 → 填提示词 → 传多张参考图 → 提交灯光任务 | 复用 `run-ue-assembly` 提交页与上传组件；新增一个 lighting action |
| 参考素材管理 | 多张参考图上传、关联镜头与会话、缩略图、标注角度 | 复用 `/api/data/entities/{}/preview-files`、`/api/doodle/pictures/*` |
| **Spec 确认与冲突裁决** | 展示 intent + 目标区间 + 冲突维度，人工裁决 | **新增**（浏览器端更适合做异步裁决） |
| 会话与版本浏览 | 每轮迭代的 rig（op 列表 + 参数）、预览图、指标、接受/拒绝状态 | 新增 `ai_light_session` / `ai_light_rig` 页面 |
| A/B 对比 | 参考 vs 各版本并排、直方图/调色板叠加（**显示空间**） | 新增对比视图 |
| 指标看板 | 区间满足率 / 调色板 / 分位 / 对比 / 色温 / VLM 分趋势 | 新增；同时是 M6 的回归基线 |
| 预设库管理 | 灯光预设 CRUD、标签、示例图、embedding 重建、启用/停用 | **直接照抄 `/api/doodle/model_library/*` 的页面模式** |
| 批量派发 | 一集/一个序列批量生成、农场进度、失败重试 | 复用 `/api/doodle/task/*`（log / inspect / restart） |
| 评审与审批 | 美术/总监签核、评论、附件、状态流转 | **复用 Kitsu 的 task status / comment / attachment 机制** |
| 对话式改灯（可选） | 聊天式提需求（"再冷一点"） | 复用现有 AIScript 的交互形态，但**改为调服务端的 plan 接口** |

### 14.4 明确不属于浏览器的部分（属 UE 面板）

- 实时视口反馈下的改灯
- op 逐个接受/拒绝/撤销
- 本机渲染预览
- SceneFacts 采集（相机、角色、现有灯清单、调色状态）

理由：这些都需要与编辑器同进程，延迟要求 <100ms。

### 14.5 三条必须守住的接口约定

1. **EXR 不下发浏览器**。浏览器显示不了线性 EXR。服务端给两份：PNG（显示空间，主判据 + 显示 + VLM 判分）+ 线性 EXR（工程检查，留在服务端）。前端只拿 PNG + 数值指标。
2. **规划逻辑不要放到前端**。现有 AIScript 的模式是浏览器直连 DeepSeek，但灯光规划**不能照抄**：农场自动模式必须在服务端调 LLM，如果前端再实现一份，提示词模板、Spec 词表、IR schema 校验就会在 JS 与 C++ 各存一份，必然漂移。所以前端只调 `/api/doodle/ai/light/*`，由服务端统一持有 prompt 与 schema。浏览器仍可保留对话式交互体验，只是后端换了。
   （`/api/doodle/deepseek/key` 那条把 key 交给浏览器的接口，灯光功能不要复用。）
3. **对比视图不要做差分图**。参考是异视角且被调过色，逐像素差分没有意义、还会误导美术。用**并排 + 统计指标对照 + 目标区间标记**。

### 14.6 需要新增的前端页面/路由

- `/lighting/spec/:specId` — **Spec 确认与冲突裁决**（新增，浏览器端核心页）
- `/lighting/session/:sessionId` — 会话详情：op 列表、版本时间线、接受/拒绝
- `/lighting/compare` — 参考 vs 版本对比（并排 / 统计对照 / 区间标记）
- `/lighting/presets` — 预设库管理
- `/lighting/jobs` — 批量任务与农场进度（也可复用现有任务列表页加过滤）
- 镜头页（Shot）增加「AI 灯光」入口按钮，与现有 `run-ue-assembly` 并列

### 14.7 可选演进：浏览器全权驱动编辑器

若以后确实想让浏览器成为唯一界面（不在 UE 里开面板），需要补：

1. UE 插件常驻 WebSocket 客户端（复用 `DoodleNetWork` 的 socket.io 模式）连主服务器
2. 服务器 → 插件的 op 下发协议（IR 增量）
3. 视口回传：低帧率 JPEG 推送（5–10 fps 足够调灯），或 UE Pixel Streaming（重，不建议）
4. 会话状态改为「服务器持有」，编辑器变成无状态执行器

代价：延迟从 <100ms 升到 200ms+，且多一条常驻连接与一套流协议。**建议先不做**，等交互模式跑顺、明确有远程调灯需求时再评估。

---

## 15. 剩余待确认

1. **参考图的 AI/PS 调整以什么为主？** 全局调色（色阶/曲线/暗角）为主，还是局部改图（加减光、重绘、去物）为主？
   —— 全局为主则 Tier B（M7）多视角联合反解可行性大幅提升；局部为主则 Tier A 是唯一可靠路径。
2. **后处理/调色是否确认允许工具修改？**（决策 5 已按"允许"设计）
   —— 若实际不允许，参考中"调色负责"的那部分目标无法达成，需要提前把预期讲清。
3. **每张参考是否值得让美术人工对位一次相机？**（每张 1–2 分钟）
   —— 这是 Tier B 最务实的位姿来源，也是决定 Tier B 是否值得做的关键成本项。
4. **性能预算**：单镜头允许的动态阴影灯上限、单次预览渲染可接受的秒数。
5. **验收标准**：区间满足率定在多少算达标？还是以"美术可接受"为准？
   —— 决定 M5 的验收门槛。
6. **是否允许改动几何/材质**：当前范围限定在灯光 + 环境 + 后处理/调色。若允许改材质反照率，闭环会更强但风险显著上升（建议先不做）。
7. **预设库首批规模**：是否安排美术整理一批（三点布光 / 伦勃朗 / 剪影 / 夜景霓虹…），还是先只用现有三档对比度环境起步。
