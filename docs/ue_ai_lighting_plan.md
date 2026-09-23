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
| 12 | **美术 4 小时以上不判断算挂起** | 会话转挂起，不占资源；恢复后继续 |
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
- 需要超时/放弃机制（美术长期不判断 → 会话挂起，不占资源）

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
  "schema_version": "1.2",
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
  "tier": "stats_only",              // stats_only | prompt_only
  "degrade_level": "full|stats_only|prompt_only"
}
```

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
                  "tier": "stats_only" },
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
    "decision": "accept|continue|reject_all",
    "selected_rig_id": "...",          // 选哪个作为下一轮基准
    "feedback_text": "主光再低一点，色温再冷一点",
    "feedback_axes": {                 // 结构化反馈（由 LLM 从 feedback_text 提取）
      "key_elevation_delta": -15, "kelvin_delta": -600
    }
  }
}
```

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

**受光方向为什么只作软提示**：PS 的加减光和渐变会伪造方向线索。方向主要由提示词与预设库决定，或由美术在候选挑选中隐式选定。

**已确认参考以全局调色为主**，这提升了统计量的可信度：全局调色不改变空间结构，调色板、亮度分位、对比度这些量都能被完整迁移；相反，若参考含大量局部改图（加减光、重绘），这些量会被局部编辑污染。因此当前默认对统计量给予较高置信度，同时保留外点剔除能力以防局部编辑。

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
| `LightingSpec.tier` | `stats_only \| multiview \| prompt_only` → `stats_only \| prompt_only` |
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

- 任务类型：`server_task_info_type::ai_light`
- 表：
  - `ai_light_job`：输入（提示、参考文件、场景）、状态、迭代轮次
  - `ai_light_spec`：LightingSpec（含 `tier` / `degrade_level` / `conflicts` / `human_confirmed`）+ 多参考图与聚合中间产物
  - `ai_light_session`：会话（当前轮次、状态 `awaiting_judgement` 等、跨会话可恢复）
  - `ai_light_iteration`：轮次（候选列表、渲染产物、人工判断记录）
  - `ai_light_rig`：版本化 IR + 是否被选中/接受
  - `light_preset`：预设库 + embedding
- 资产路径复用 `get_shots_auto_lighting_upload_path` / `get_shots_auto_lighting_upload_movie_path`（`core/entity_path.cpp:425-445`）

### 10.3 轮次编排（新增，核心）

服务器侧的编排逻辑：

```
生成候选 → 建任务 → 派发 → 等 worker 回传 → 落 iteration
  → 置 session.state = awaiting_judgement → 通知前端
  → 等人工判断（可能数小时/数天）
  → 收到判断：
       accept        → 走定稿（⑤）
       continue      → 据 feedback_axes 生成下一轮候选 → 回到派发
       reject_all    → 回到规划层重新出方案（或结束）
```

- 「等待人工判断」是**会话状态**，不是任务状态（避免污染 `server_task_info_status`）
- **美术 4 小时以上不判断 → 会话转挂起**（不占资源；恢复后从 `awaiting_judgement` 继续）

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
| **M3 参考驱动** | 多图统计聚合 + 冲突检测 + 人工裁决 + `degrade_level` 判定 | 目标区间稳定、冲突能被人正确裁决 |
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
| **美术长期不判断 → 会话挂起** | `awaiting_judgement` 状态 + 超时/挂起机制，不占资源 |
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
| `prompt_only` | 参考不可用/完全冲突 | 只用提示词 + 预设库检索 |

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
2. **提交端点**：新增 `/api/actions/projects/{}/shots/{}/ai-light`，照 `http_method/kitsu/distributed_task.cpp:49-74` 的 `actions_projects_shots_run_ue_assembly::post` 写：构造 `server_task_info`，`type_ = ai_light`，`command_ = {baseline, candidates[]}`（`command_` 是自由 JSON，可直接承载，无需改表结构），insert 后调 `run_next_task()` + `socket_io::broadcast`。
3. **派发逻辑**：**零改动**。`computers.cpp:376-378` 已按 `computer.allowed_task_types_` 通用过滤。
4. **LLM/VLM 客户端**：新增 `deepseek_client`（继承 `http_client/ai_client_base.h`，OpenAI 兼容，支持 chat + vision）。**必须在主服务器**——`--local` 的 `kitsu_ctx_t` 是两参构造，没有 key。
5. **AI 路由**：
   - `/api/doodle/ai/light/spec`：提示词 + 多张参考 → LightingSpec（含聚合与冲突检测）
   - `/api/doodle/ai/light/spec/confirm`：人工闸门 1（确认/冲突裁决）
   - `/api/doodle/ai/light/iterate`：生成一轮候选并派发
   - `/api/doodle/ai/light/judge`：**人工闸门 2**（accept / continue / reject_all + 反馈）
   - `/api/doodle/ai/light/session/{id}`：轮次与候选查询
   - `/api/doodle/ai/light/preset`：预设库 CRUD
6. **轮次编排**：`awaiting_judgement` 会话状态、超时挂起、跨会话恢复（§10.3）。
7. **数据表**：`ai_light_job` / `ai_light_spec` / `ai_light_session` / `ai_light_iteration` / `ai_light_rig` / `light_preset`，进 `sqlite_database` 的 upgrade 与 ORM 注册。
8. **参考文件接收**：复用 `http_method/up_file.h` 的 `doodle_data_shots_file_auto_light` 模式；DeepSeek Files API 的 `file_id` 缓存也放这里。
9. **反馈结构化**：`/judge` 收到的自然语言反馈经 LLM 转成 `feedback_axes`。

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
| 会话挂起 | **美术 4 小时以上不判断算挂起** | §2 决策 12、§10.3 |
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
