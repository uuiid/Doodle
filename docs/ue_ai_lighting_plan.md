# LLM 驱动的 UE 灯光工具 —— 方案设计与软件架构

> 状态：**方案设计 v2**（四项关键边界已确认，见 §2）
> 环境：UE 源码树 `E:\UnrealEngine`（5.7.4，已完整编译）、UE 工程 `E:\ue_main`、Doodle 插件源码 `E:\Doodle\script\uePlug\Doodle`
> 输入：文本提示 + 参考图片 + 参考视频
> 输出：对场景灯光的增 / 删 / 移动 / 改参，使渲染氛围接近输入意图

---

## 1. 核心判断

**判断一：纯 LLM 开环做不到，必须闭环。**
LLM 能可靠决定「要有主光、来自左前方 45°、冷色、硬光、夜景霓虹」这类**语义与结构**，但无法预测「强度 2.3 还是 5.7」这类**连续数值**。渲染链路上还有自动曝光、Lumen 间接光、材质反照率等强非线性环节，任何数值猜测都会被打偏。

**判断二：不让 LLM 直接生成 UE Python/C++ 或直接调 API，而是定义 IR。**
否则不可复现、不可校验、不可撤销、不可 diff、不可回归测试，也无法在没有 LLM 的情况下单测执行器。

**判断三：交互优先，但第一天就用同一套 IR 和同一个执行器承载两种模式。**
既然最终要推到农场，交互模式就不能是「先做个编辑器小工具、以后再重写一遍」。做法是：执行器只有一份（`apply_ops` 等原语），交互模式下它跑在编辑器进程内被 Slate 面板直接调用，自动模式下它被包进 commandlet 由农场调用。IR 既是两者之间的传输载荷，也是交互会话的记录格式——**会话结束时导出的 rig 就是农场任务的输入**，不存在转换环节。

**判断四：参考是同机位预演 → 可以做像素级与区域级优化，这是本方案最大的性能杠杆。**
同机位意味着渲染图与参考图逐像素对齐，于是：误差可以定位到图像区域，区域可以反查到「哪盏灯照亮了这些像素」（可见性 / 阴影贴图），从而做**区域引导的信用分配**，而不是盲目黑箱搜索。这把收敛从「几十次渲染」压到「几次」。

---

## 2. 已确认的设计决策

| # | 决策 | 架构影响 |
|---|---|---|
| 1 | 参考视频/图片**多数为本场景预演**（同机位），少数为异构参考 | 主路径启用像素级/区域级损失；仍需自动识别制度（§5.4） |
| 2 | **优先美术在编辑器内交互调整**，之后转为农场自动任务 | 交互模式是 M1；执行器必须能在编辑器进程内调用；IR = 会话记录 |
| 3 | **LLM 使用云端 deepseek** | 文本与视觉同一把 key（`deepseek-flash` 支持图片输入）；无需本地模型 |
| 4 | **允许修改环境中所有灯光** | 去掉「只动自己管理的灯」护栏；改为全场景快照 + 完整回滚 + 事务；环境光/雾/天空/后处理全在可改范围内 |

---

## 3. 总体架构

```
┌──────────────────────────────────────────────────────────────────────────┐
│  输入：文本提示 · 参考图片(1..N) · 参考视频(1..N) · 场景/相机上下文        │
└────────┬───────────────┬───────────────────┬────────────────────┬────────┘
         │               │                   │                    │
┌────────▼───────────────▼───────────────────▼────────────────────▼────────┐
│ ① 感知层 Perception                                                       │
│  · 文本 → LLM 归一化为 LightingBrief                                      │
│  · 图片 → 低层统计(调色板/直方图/对比比/白平衡/受光方向) + VLM 语义描述    │
│  · 视频 → 分镜 → 关键帧 → 逐镜聚合 → 时序曲线 + 相机运动分离              │
│  · 制度识别：参考是否为同机位（决定用像素级还是统计级损失）               │
└────────────────────────────────┬─────────────────────────────────────────┘
                                 │
┌────────────────────────────────▼─────────────────────────────────────────┐
│ ② 规划层 Planner（deepseek + 灯光预设库检索）                             │
│  · 检索 top-k 灯光预设（RAG over Rig）                                    │
│  · function calling → LightingPlan = 有序 op 列表 + rationale             │
│  · schema 约束 + 性能护栏                                                 │
└────────────────────────────────┬─────────────────────────────────────────┘
                                 │  LightingRig IR (JSON, 版本化)
        ┌────────────────────────┴────────────────────────┐
        │                                                 │
┌───────▼──────────────────────────┐   ┌──────────────────▼────────────────┐
│ ③A 交互模式（M1，优先）           │   │ ③B 自动模式（M6，农场）            │
│  Slate 面板（编辑器进程内）        │   │  UnrealEditor-Cmd                  │
│   · 直接调用执行器（同进程）       │   │  -run=DoodleLighting -Params=<json>│
│   · 视口实时看效果                 │   │   · 同一执行器、同一 IR            │
│   · 逐个 op 接受/拒绝/撤销         │   │   · 无人在环 → 全自动闭环          │
│   · 会话 = IR 版本序列             │   │   · 分布式派发                    │
│  Doodle 服务 ← WebSocket 取方案    │   │                                    │
└───────┬──────────────────────────┘   └──────────────────┬────────────────┘
        │                                                 │
        └────────────────────┬────────────────────────────┘
                             │  渲染预览（EXR 线性 + PNG 审阅）
┌────────────────────────────▼─────────────────────────────────────────────┐
│ ④ 评判层 Critic                                                           │
│  · 同机位：逐像素差 → 区域误差图 → LPIPS / 区域统计                       │
│  · 异构：调色板 EMD / 直方图 / key:fill / 白平衡 / 裁剪率 + VLM 判官       │
│  · VLM 反馈 → 结构化 delta（不是自由文本）                                │
└────────────────────────────┬─────────────────────────────────────────────┘
                             │
┌────────────────────────────▼─────────────────────────────────────────────┐
│ ⑤ 优化器 Optimizer（仅自动模式必须；交互模式由美术的眼睛替代）             │
│  · 区域引导信用分配：误差区域 → 影响该区域的灯 → 定向调参                 │
│  · 残余连续参数：坐标下降 / CMA-ES / 贝叶斯                                │
│  · warm start：复用上一版 rig（同 shot 多版本是常态）                      │
└──────────────────────────────────────────────────────────────────────────┘
```

---

## 4. 数据契约：LightingRig IR（全系统唯一真相）

IR 必须同时能被 LLM 生成、被 C++ 校验、被 UE 消费、被数据库版本化，并且**同时充当交互会话的记录格式**（决策 2）。

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
  "postprocess": { "bloom": 0.0, "sharpen": 1.5, "lumen_final_gather": 4 },
  "environment": { "sky_light": {...}, "sky_atmosphere": {...},
                   "height_fog": {...}, "volumetric_cloud": {...} },
  "units": { "length": "cm", "color_space": "linear_srgb" }
}
```

### 4.2 灯光方案 LightingRig（LLM 输出 / 执行器输入 / 会话记录 / 数据库存储）

```jsonc
{
  "schema_version": "1.0",
  "rig_id": "uuid",
  "session_id": "uuid",                  // 交互会话
  "shot_task_id": "uuid",
  "iteration": 3,
  "provenance": { "model": "deepseek-flash", "prompt_hash": "...",
                  "reference": ["ref_01.exr"], "seed": 12345,
                  "reference_regime": "same_camera" },
  "baseline_snapshot": "snapshot_000",   // 全场景快照 id（决策 4：完整回滚）
  "globals": {
    "exposure": { "method": "manual", "ev100_bias": -0.3 },
    "white_balance": { "kelvin": 5600, "tint": 0.0 },
    "environment": { "sky_light_intensity": 0.6, "fog_density": 0.02 }
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
    { "op": "remove", "light": "art_rim_02", "rationale": "与参考图无轮廓光" }
  ]
}
```

**为什么 `anchor.space=camera`**：绝对世界坐标对 LLM 无意义（它不知道场景尺度），而「相机左前方 45°、抬高 30°、距离 3m」是电影布光的通用语言，且对场景尺寸鲁棒。执行器负责换算成世界坐标。同机位预演下，参考图的受光方向估计也能直接映射到这个空间。

**为什么 `id` 与 UE actor 名解耦**：美术会重命名、复制、删除 actor。语义 id 是稳定主键，映射表存在 UE 侧（`Tags` 标记）或 Doodle 数据库。

---

## 5. ① 感知层 Perception

### 5.1 文本提示 → LightingBrief

LLM 的任务是**归一化**，不是自由发挥。输出固定字段：

| 字段 | 取值域 |
|---|---|
| `time_of_day` | dawn / morning / noon / afternoon / dusk / night / midnight |
| `weather` | clear / overcast / rain / fog / snow / haze |
| `source_motivation` | sun / window / practical_lamp / fire / neon / moon / screen / none |
| `mood` | 受控词表选 1–3 个（柔和/冷硬/高对比/低对比/温暖/忧郁/神圣/压抑…） |
| `key_direction` | front / front_left / left / back_left / back / back_right / right / front_right / top / bottom |
| `contrast_ratio` | 数值区间，如 2:1 … 16:1 |
| `color_tendency` | 色温区间 + 主色调 |
| `exposure_feel` | 欠曝 / 正常 / 过曝（映射到 EV 偏移区间） |

未提及字段留 `null`，由参考图统计或预设库默认值补全。**禁止 LLM 输出自然语言段落作为执行依据。**

### 5.2 图片 → 特征

- **低层统计（确定性，数值可信）**：线性空间调色板（k-means）与占比；亮度直方图/百分位；key:fill 比（主体区 vs 背景区中位亮度）；白平衡估计；高光/暗部裁剪率；受光方向估计（shape-from-shading / 梯度朝向直方图）→ 主光方位角初值；主体/人脸检测（供 `anchor.target`）。
- **高层语义（VLM，`deepseek-flash`）**：描述光源、时间、材质、氛围 → 映射到同一套 LightingBrief。
- **合并策略**：数值以统计为准，语义以 VLM 为准；冲突记入 rationale 供人工审阅。
- **工程优化**：参考图先用 DeepSeek [Files API](https://api-docs.deepseek.com/zh-cn/guides/vision/) 上传一次拿 `file_id`，闭环内每轮引用而不重复上传（同一张图在几十轮迭代里会被反复送进 VLM）。

### 5.3 视频 → 时序

```
分镜检测 → 每镜关键帧(首/中/尾 + 内容变化帧) → 逐帧统计 → 镜内聚合(中位数)
        → 相机运动估计(光流/特征) → 分离"灯光变化"与"相机运动"
        → 输出：逐镜 LightingBrief + 关键光时序曲线
```

**显式区分两种视频语义**（输入参数 `video_role`）：

- `scene_previz`（本场景预演，主路径）：相机已知且与当前一致 → 可比对，走同机位制度
- `reference_film`（异构参考片）：只比统计与语义

### 5.4 制度识别（自动，必须做）

虽然多数参考是同机位，但**不能假设**。执行器在导入参考时自动判定：

| 判据 | 结论 |
|---|---|
| 参考为 EXR 且内嵌相机元数据与当前一致 | `same_camera` |
| 参考分辨率/宽高比与相机 filmback 一致 + 特征点单应矩阵近似恒等 | `same_camera` |
| 否则 | `heterogeneous` |

结果写入 `provenance.reference_regime`，评判层据此选择损失函数。**误判的代价是不收敛**，所以判据要保守：不确定时降级为 `heterogeneous`。

### 5.5 两种对比制度

| 制度 | 可用的比较方式 |
|---|---|
| `same_camera` | 逐像素差 + 区域误差图 + LPIPS/SSIM + 全部结构化指标 |
| `heterogeneous` | 仅结构化指标 + VLM 语义判分 + 区域级（主体 vs 背景）比较 |

不做这个区分，优化器会去追一个不可能达到的像素目标，loss 永不收敛。

---

## 6. ② 规划层 Planner

### 6.1 LLM 的职责边界

| LLM 做 | LLM 不做 |
|---|---|
| 决定灯的**数量、角色、类型、相对方位** | 决定最终精确强度/色温/距离 |
| 决定**环境光/雾/天空/曝光**的整体倾向 | 直接写 UE API 调用或代码 |
| 决定**增删改**哪些灯（结构决策） | 逐帧关键帧的精确数值 |
| 读 VLM/美术反馈，给出**结构级修正** | 预测渲染结果 |
| 选择/组合**预设模板** | 绕过 schema 自由输出 |

### 6.2 知识注入：灯光预设库（Rig Library）

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

流程：**参考图/提示 embedding → top-k 预设 → 作为 LLM 候选骨架 → LLM 裁剪/组合/微调**。保证物理合理性与项目内风格一致性，并大幅降低 LLM 的输出维度。

### 6.3 工具面（function calling schema）

| 工具 | 作用 | 副作用 |
|---|---|---|
| `inventory_lights()` | 读当前灯光与环境清单 | 无 |
| `query_scene(anchor)` | 取对象包围盒/位置/朝向 | 无 |
| `list_presets(query)` | 检索预设 | 无 |
| `add_light(spec)` | 新增一盏灯 | 进 op 队列 |
| `set_light(light_id, patch)` | 改参数 | 进 op 队列 |
| `remove_light(light_id)` | 删灯 | 进 op 队列 |
| `set_globals(patch)` | 曝光/白平衡/雾/天空 | 进 op 队列 |
| `render_preview(camera, frames)` | 出预览图 | 重 |
| `score(target)` | 打分 | 无 |
| `commit(rig)` | 落库 + 写序列 | 有 |

所有写操作先进 **op 队列**，由执行器统一校验后应用 —— 天然获得 dry-run、diff、逐步审阅、回滚。交互模式下这正是美术逐个接受/拒绝的粒度。

### 6.4 护栏（硬约束，schema 层面强制）

决策 4 已放开「不能改美术灯」，但以下仍然强制：

- 灯数量上限、动态阴影灯数量上限（性能预算）
- 强度/色温/角度/距离的取值边界
- 光照通道约定（沿用现有 `OnCreateDirectionalLight` 的 `SetLightingChannels(false, true, false)` = Channel1 给主光）
- **任何变更前必须生成全场景快照**，支持一键完整回滚（因为现在什么都能改，回滚是唯一的安全网）
- 禁止破坏已有子序列绑定（`AddSequenceWorldToRenderWorld` 建立的层级）
- 曝光必须为手动（见 §7.3）

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
| `snapshot` | 全场景灯光/环境/后处理状态快照（决策 4 的安全网） |
| `restore` | 从快照完整回滚 |
| `inventory` | 扫描关卡 → 输出 SceneFacts |
| `apply_ops` | 按 op 列表增删改，全程 `FScopedTransaction` + `Modify()` |
| `render_preview` | 低分辨率预览，EXR(线性) + PNG(审阅) 双输出 |
| `export_rig` | 导出当前 rig 为 IR（支持从美术手工灯反向生成 IR → 作为会话起点与预设来源） |
| `commit` | 写入 Level Sequence 关键帧轨道 + 落库 |

### 7.3 实现要点（血泪项）

1. **幂等**：同一 rig 重复 apply 结果必须一致。
2. **`ClearAllLight()` 不能直接复用**：现有实现（`DoodleAutoAnimationCommandlet.cpp:535`）无条件遍历销毁所有 `ALight` + `APostProcessVolume`。交互模式下这是灾难性的。改为：先 `snapshot`，再按 op 精确增删，`restore` 兜底。
3. **曝光必须手动锁定**：现有 `PostProcessVolumeConfig()` 已把 `AutoExposureMinBrightness/MaxBrightness` 固定为 1.0、`AutoExposureBias=0.0`。这是对的，**闭环期间绝不能放开** —— 否则调强度会被自动曝光吃掉，闭环直接失效。
4. **Lumen 参数必须固定**：`LumenFinalGatherQuality=4` 等已在代码里固定，闭环期间不要变动，否则指标漂移。
5. **单位显式**：`ULightComponent::SetIntensityUnits(ELightUnits::Candela/Lux/EV)`，IR 里也带单位，避免流明/坎德拉歧义。
6. **环境光也算灯**：`SkyLight / SkyAtmosphere / ExponentialHeightFog / VolumetricCloud` 纳入 IR `environment`。夜景氛围一半来自它们，且决策 4 明确允许改。
7. **时间维度**：Level Sequence 光照轨道写关键帧；可复用项目已有的 `DoodleLightningPost` / `DoodleCluster` 轨道序列化能力。
8. **交互延迟预算**：面板操作要 <100ms 生效（纯属性写入）；`render_preview` 是唯一重操作，异步 + 可取消，美术可随时中断。

### 7.4 模块落点

新增两个模块，**不污染现有 `DoodleEditor`**：

- `DoodleLighting`（Runtime）：IR 数据结构、JSON 序列化、校验器、op 队列语义、快照/回滚、指标计算（可脱离 UE 单测）
- `DoodleLightingEditor`（Editor）：执行器、Slate 面板、预览渲染、WebSocket 客户端、命令

---

## 8. ④ 评判层 Critic

### 8.1 同机位制度（主路径）：区域级误差

因为参考与渲染逐像素对齐，可以做别的方案做不到的事：

```
误差图 D = |render_linear − reference_linear|
  → 分块/超像素分割 → 区域误差列表（位置 + 符号 + 幅度）
  → 每盏灯的影响图 M_i（可见性/阴影贴图，可从 UE 直接取）
  → 信用分配：区域 r 的误差主要由 argmax_i overlap(M_i, r) 的灯负责
```

这同时是**优化器的输入**（§9）和**给美术/LLM 的可读反馈**（"画面右侧过亮，主因是 fill_02"）。

### 8.2 异构制度：结构化 + 语义

| 指标 | 说明 |
|---|---|
| 调色板 EMD | 线性空间调色板分布的 Wasserstein 距离 |
| 亮度直方图距离 | 分位数曲线 L1 |
| key:fill 比差 | 主体区 / 背景区中位亮度比 |
| 白平衡差 | 估计色温差 |
| 裁剪率 | 高光/暗部越界像素占比 |
| 主体-背景分离度 | 角色是否从背景中"跳出来" |

### 8.3 VLM 判官

输入：参考图 + 当前渲染 + 原始提示 → 输出 `score 0–10` + **可执行的自然语言差异**。关键设计：自由文本必须再经一次结构化映射（LLM）转成 LightingBrief 字段的 delta，才能进入下一轮规划。

### 8.4 线性 EXR vs 色调映射 PNG

- **EXR（线性、未色调映射）**：所有数值指标。避免 tone curve 把强度差异压平。
- **PNG（色调映射后）**：人工审阅与 VLM 判分（VLM 见过的是显示图）。

两者必须分开，否则指标与人的观感脱节。

### 8.5 loss

```
L = w1·region_pixel + w2·palette + w3·luma + w4·contrast
  + w5·white_balance + w6·clip_rate + w7·(1 − VLM_score/10)
  + w8·constraint_penalty
```

`w1` 仅在 `same_camera` 制度下非零。权重需在项目自有 A/B 盲测集上校准（指标与人工评分的相关性）。

---

## 9. ⑤ 优化器 Optimizer

**仅在自动模式（③B）必须**——交互模式下美术的眼睛就是评判器，不需要 loss。

1. **区域引导信用分配（主算法，同机位时）**：误差区域 → 影响图 → 定向调参。通常 3–5 轮即可把大面积偏差压下去。
2. **残余连续参数**：坐标下降 / CMA-ES / 贝叶斯优化（高斯过程代理）。
3. **分阶段降维**：先全局（曝光 EV、整体强度、色温）→ 再逐灯；按 `role` 分组，主光优先。
4. **warm start**：同 shot 多版本是常态，直接用上一版 rig（或美术在交互模式里定稿的 rig）作初值。
5. **并行候选**：分布式派发同时评估多个候选。
6. **停止条件**：Δloss < ε / 迭代上限 / 人工接受。

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
  - `ai_light_session`：交互会话（IR 版本序列、美术的接受/拒绝记录）
  - `ai_light_rig`：版本化 IR + 预览图 + 分数 + 是否被接受
  - `light_preset`：预设库 + embedding
- 资产路径复用 `get_shots_auto_lighting_upload_path` / `get_shots_auto_lighting_upload_movie_path`（`core/entity_path.cpp:425-445`）

### 10.3 分布式派发必须遵守的既有契约

来自既有实现的教训（`computers.cpp` / `work.cpp`）：

- 派发判据 = 客户端上报 `online` **AND** 服务端自有事实「库里没有 `status_=running` 且 `run_computer_id_` 指向该机的任务」
- 客户端用 `running_task_count_` 计数，归零才报 `online`
- **任务结束前必须 `put_job_info` 上报终态**，否则任务永远留在 running 挡住该机器

灯光闭环会产生大量短任务（每次预览渲染都可能是一条），这条契约是硬性要求。

### 10.4 前端（doodle_vue）

参考文件上传、预设库管理、任务与版本浏览、A/B 对比查看。**提示输入与交互调灯的主战场在 UE 编辑器面板内**（决策 2），网页端主要负责文件与任务管理。

---

## 11. 分阶段里程碑（按决策 2 重排：交互优先）

| 阶段 | 内容 | 完成判据 |
|---|---|---|
| **M0 地基** | IR schema + 双端序列化 + `snapshot/restore/inventory/apply_ops/render_preview` 原语 | 手工写一份 IR → 应用 → 出图 → 回滚到原始状态；幂等性单测通过 |
| **M1 交互闭环** | Slate 面板 + 执行器同进程 + WebSocket 取 LLM 方案 + 逐个 op 接受/拒绝/撤销 + 视口实时生效 | 美术能在编辑器里用自然语言改灯，全程可撤销，会话可存可读 |
| **M2 参考驱动** | 参考图/视频导入 + 制度识别 + 统计特征 + VLM 语义 + 预设库检索 | 同机位参考下，面板能并排比对并给出区域误差定位 |
| **M3 会话 → 任务** | 交互会话导出为农场任务 IR；`-run=DoodleLighting` 分支打通 | 交互定稿的 rig 原样在农场跑出同结果 |
| **M4 自动评判** | 区域级损失 + 结构化指标 + VLM 判官 | 无人在环时指标可量化、可复现 |
| **M5 自动优化** | 区域引导信用分配 + 残余黑箱优化 | 同机位参考下 5 轮内收敛到人工可接受 |
| **M6 生产化** | 分布式派发、性能预算、回归测试集、A/B 盲测校准 | 固定场景 + 参考 → 指标基线纳入 CI |

M3 之所以这么早：决策 2 要求「交互优先，之后转自动任务」，而**只要 IR 和会话记录从一开始就是农场任务的输入格式，M3 就几乎是免费的**；反过来如果交互阶段自己造了一套临时状态，M3 就会变成重写。

---

## 12. 关键风险与对策

| 风险 | 对策 |
|---|---|
| LLM 数值不可靠 | 闭环 + 优化器；LLM 只给结构与初值；schema 硬边界 |
| 自动曝光 / Lumen 使强度不可预测 | 强制手动曝光、固定 Lumen 参数（已有代码基础） |
| 全场景可改 → 误操作代价高 | 每次变更前 `snapshot`；完整 `restore`；`FScopedTransaction` 撤销栈 |
| 参考制度误判 | 保守判据，不确定时降级为异构；`provenance` 记录以便复盘 |
| 渲染成本高、迭代慢 | 低分辨率预览、EXR/PNG 分离、warm start、区域引导减少搜索次数、并行候选 |
| 交互延迟破坏手感 | 属性写入 <100ms；预览异步可取消；面板不阻塞编辑器 |
| 语义歧义（"电影感"） | 受控词表归一化 + 预设检索 + 人工在环选择 |
| 视频中相机运动被误判为灯光变化 | 显式 `video_role`；利用已知相机（项目已有相机 fbx 导入） |
| 评测主观 | 项目 A/B 盲测集，校准指标权重与人工评分的相关性 |
| VLM 反复上传同一参考图 | DeepSeek Files API 上传一次，闭环内用 `file_id` 引用 |

---

## 13. 改动归属：四类程序分别负责什么

### 13.0 一张表看懂

| 角色 | 入口 | 一句话职责 | 是否持有 deepseek key | 灯光改动的性质 |
|---|---|---|---|---|
| ① 主服务器 | `kitsu_supplement`（无参数） | 真库 + 全量 API + 任务派发 + **LLM/VLM 调用** | **是** | 新增：任务类型、提交端点、AI 路由、数据表、预设库 |
| ② 客户端 | `kitsu_supplement --local` | 工位本地后端（内存库 + 本地 API + socket.io） | 否 | 基本无改动（仅插件版本分发被动受影响） |
| ③ 工作程序 | `--local` + POST `/api/actions/local/task/run` | 连服务器 WebSocket、领任务、跑任务、报终态 | 否 | 新增：`run_task` 分派分支 + 新任务类 + allowed 类型 |
| ④ UE 插件 | `E:\Doodle\script\uePlug\Doodle` | 场景内实际增删改灯、渲染、面板 | 否（回调服务器） | 新增两个模块 + 执行器 + 面板 + commandlet 入口 |

**核心结论：改动集中在 ①③④，②几乎不动。**

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
   - `/api/doodle/ai/light/plan`：提示 + 参考 → 返回 op 列表
   - `/api/doodle/ai/light/score`：渲染图 vs 参考 → 指标 + VLM 判分
   - `/api/doodle/ai/light/preset`：预设库 CRUD
6. **数据表**：`ai_light_job` / `ai_light_session` / `ai_light_rig` / `light_preset`，进 `sqlite_database` 的 upgrade 与 ORM 注册。
7. **参考文件接收**：复用 `http_method/up_file.h` 的 `doodle_data_shots_file_auto_light` 模式；DeepSeek Files API 的 `file_id` 缓存也放这里。
8. **闭环归属（重要）**：自动模式的迭代循环建议**整体放主服务器**，工作机只做「执行 + 渲染」。理由：key 不落到工作机、迭代历史集中可查、评分口径统一。工作机只需把渲染结果回传。

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

1. **新增两个模块**（改 `Doodle.uplugin`）：`DoodleLighting`(Runtime，IR + 校验 + 快照 + 指标) / `DoodleLightingEditor`(Editor，执行器 + 面板 + 预览)。
2. **IR 双端一致**：UE 侧 `FJsonObject` 解析必须与 C++ 侧 nlohmann 序列化严格对齐（单位枚举、`anchor.space`、`origin`）。建议落一份 JSON Schema，两端各写校验器，CI 跑同一组 golden 用例。
3. **交互面板**（M1 核心）：提示输入、参考导入、op 列表与 diff、逐个接受/拒绝、撤销、与参考图并排比对。
4. **WebSocket 客户端**：复用 `Source/BatchRender/Public/DoodleNetWork.h` 的 `IWebSocket` 模式，新增 lighting 消息类型，**连主服务器**（不是本地客户端）。
5. **执行器原语**：`snapshot / restore / inventory / apply_ops / render_preview / export_rig / commit`。
6. **必须改的现有实现**：
   - `ClearAllLight()`（`DoodleAutoAnimationCommandlet.cpp:535`）无条件销毁所有 `ALight` + `APostProcessVolume`，不能用于交互模式
   - `PostProcessVolumeConfig()` 的手动曝光（AutoExposure 固定 1.0/1.0）必须保持
   - `OnCreateDirectionalLight()` 的 `SetLightingChannels(false, true, false)`（Channel1 主光）要成为 IR 默认约定
7. **commandlet 入口**：建议在 `Params` 的 JSON 里加字段，而不是新增命令行 key，减少分支。
8. **`render_preview` 双输出**：线性 EXR（指标用）+ 色调映射 PNG（审阅/VLM 用），MovieRenderPipeline 配置需相应调整。
9. **插件版本发布**：模块变更后必须发新版本包（对应 13.2 第 2 条）。

---

### 13.5 一次交互的完整调用链（M1）

```
美术在 UE 面板输入提示 + 拖入参考图
  → [④ 插件] 面板收集 SceneFacts（inventory）+ 参考图
  → [④→①] WebSocket/HTTP 到主服务器
  → [①] deepseek 归一化提示 + VLM 读参考图 + 预设库检索 → 返回 op 列表(IR)
  → [④] 面板展示 op 列表与 diff
  → 美术逐个接受 → 同进程直接 apply_ops（FScopedTransaction，可撤销）
  → [④] 视口实时生效；点"对比参考" → 本机渲染预览 → 回传
  → [①] 计算指标 + VLM 判分 → 返回可读差异
  → 美术继续说"再冷一点" → 回到第 2 步
  → 定稿 → [①] 会话 IR 落库
```

### 13.6 自动模式的完整调用链（M6）

```
[①] 从 ai_light_session 取定稿 IR（或从零生成）→ 建 ai_light 任务 → insert + run_next_task
  → [③] WebSocket 收到任务 → run_ai_light_distributed
  → [③] async_run_ue: -run=DoodleLighting -Params=<IR json>
  → [④] 执行器 apply_ops → render_preview(EXR+PNG)
  → [③] 回传渲染结果 + put_job_info
  → [①] 评分 → 未收敛则生成下一轮 IR → 再建任务（循环，工作机无 key）
  → [①] 收敛 → 落 ai_light_rig + 通知前端
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
- 两者共用同一份 IR 和同一个会话记录

### 14.3 浏览器前端负责的内容

| 能力 | 内容 | 复用 / 新增 |
|---|---|---|
| 任务提交 | 选镜头 → 填提示词 → 传参考图/视频 → 提交灯光任务 | 复用 `run-ue-assembly` 提交页与上传组件；新增一个 lighting action |
| 参考素材管理 | 参考图/视频上传、关联镜头与会话、缩略图 | 复用 `/api/data/entities/{}/preview-files`、`/api/doodle/pictures/*` |
| 会话与版本浏览 | 每轮迭代的 rig（op 列表 + 参数）、预览图、指标分数、接受/拒绝状态 | 新增 `ai_light_session` / `ai_light_rig` 页面 |
| A/B 对比 | 参考 vs 各版本并排、差分图、直方图/调色板叠加 | 新增对比视图 |
| 指标看板 | 调色板 EMD / 直方图 / key:fill / 白平衡 / 裁剪率 / VLM 分的趋势 | 新增；同时是 M6 的回归基线 |
| 预设库管理 | 灯光预设 CRUD、标签、示例图、embedding 重建、启用/停用 | **直接照抄 `/api/doodle/model_library/*` 的页面模式** |
| 批量派发 | 一集/一个序列批量生成、农场进度、失败重试 | 复用 `/api/doodle/task/*`（log / inspect / restart） |
| 评审与审批 | 美术/总监签核、评论、附件、状态流转 | **复用 Kitsu 的 task status / comment / attachment 机制** |
| 对话式改灯（可选） | 聊天式提需求（"再冷一点"） | 复用现有 AIScript 的交互形态，但**改为调服务端的 plan 接口** |

### 14.4 明确不属于浏览器的部分（属 UE 面板）

- 实时视口反馈下的改灯
- op 逐个接受/拒绝/撤销
- 本机渲染预览
- SceneFacts 采集（相机、角色、现有灯清单）

理由：这些都需要与编辑器同进程，延迟要求 <100ms。

### 14.5 两条必须守住的接口约定

1. **EXR 不下发浏览器**。浏览器显示不了线性 EXR。服务端必须给两份：线性 EXR（只用于指标计算，留在服务端）+ 色调映射 PNG（给浏览器显示、给 VLM 判分）。前端只拿 PNG + 数值指标。
2. **规划逻辑不要放到前端**。现有 AIScript 的模式是浏览器直连 DeepSeek，但灯光规划**不能照抄**：农场自动模式必须在服务端调 LLM，如果前端再实现一份，提示词模板、LightingBrief 词表、IR schema 校验就会在 JS 与 C++ 各存一份，必然漂移。所以前端只调 `/api/doodle/ai/light/plan`，由服务端统一持有 prompt 与 schema。浏览器仍可保留对话式交互体验，只是后端换了。
   （`/api/doodle/deepseek/key` 那条把 key 交给浏览器的接口，灯光功能不要复用。）

### 14.6 需要新增的前端页面/路由

- `/lighting/session/:sessionId` — 会话详情：op 列表、版本时间线、接受/拒绝
- `/lighting/compare` — 参考 vs 版本对比（并排 / 差分 / 直方图）
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

1. **性能预算**：单镜头允许的动态阴影灯上限、单次预览渲染可接受的秒数。
2. **验收标准**：是「氛围接近、美术可接受」，还是要「指标收敛到某阈值」？—— 决定 M5 的验收门槛。
3. **是否允许改动几何/材质**：当前范围限定在灯光 + 环境 + 后处理。若允许改材质反照率，闭环会更强但风险显著上升（建议先不做）。
4. **预设库首批规模**：是否安排美术整理一批（三点布光 / 伦勃朗 / 剪影 / 夜景霓虹…），还是先只用现有三档对比度环境起步。
