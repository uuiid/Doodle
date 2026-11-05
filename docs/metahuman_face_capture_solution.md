# MetaHuman 面部捕捉方案（修订版 · 纯 C++ + Doodle 插件模块）

> 状态：已拍板（2026-11-04），待实现
> 环境：UE 5.7.4（`E:\UnrealEngine`）
> 落地位置：`E:\Doodle\script\uePlug\Doodle` 新增 Editor 模块 `DoodleMetaHumanFace`
> 上游调用方（Doodle 后端）见 [face_capture_camera_stream_plan.md](face_capture_camera_stream_plan.md)：它负责把 `-FramesDir` 的 PNG 序列按本文 §6.2 的口径产出，并在进程退出后读 `face_capture_log.json`。

**已拍板的五条**（后文全部按此展开）：

1. **ingest 走 B 路线** —— Doodle 侧出逐帧图像，UE 只构造 `UImgMediaSource` + `UFootageCaptureData`。A 路线（CaptureManager 设备链）不实现，仅留附录 B 备查。
2. **`.dhfc` 不含平滑前数据** —— `flags` 保留为扩展位，`bit0` 恒为 0。
3. **V1 宿主用 `E:\ue_main`** —— 只验证能编译、能跑通，不作为部署目标。
4. **引擎插件在 `Doodle.uplugin` 的 `"Plugins"` 段启用** —— 借 `Doodle.uplugin:13` 的 `EnabledByDefault: true`，Doodle 插件在任意工程自动加载，因此宿主工程**不必改自己的 `.uproject`**。
5. **不导出 FBX / AnimSequence** —— 唯一交付格式是 `.dhfc` 曲线。
6. **入口是命令行 commandlet，一次一个序列** —— 无常驻进程、无 HTTP、无队列（§6）。

---

## 0. 结论先行

| 问题 | 答案 |
| --- | --- |
| 必须写 Python 吗？ | **不必**。上一版的 Python 只是照抄官方示例；ingest / 求解 / 导出三件事全部有公开 C++ API。 |
| 能放进现有 Doodle 插件吗？ | **能**，加一个 `Type: Editor` 的新模块即可，不需要新进程、不需要新工程。 |
| 入口形态？ | **命令行 commandlet**，一次进程只转换一个序列，进程退出即出结果。无 HTTP、无常驻、无 IPC。 |
| 视频解帧在哪做？ | **Doodle 侧做**（B 路线）。UE 只负责把图像序列包成 `UFootageCaptureData`，因此不启用 CaptureManager 那一串插件。 |
| 主交付用自定义格式？ | **是，唯一交付格式**。UE 的 AnimSequence 导出会丢掉全部面部曲线（见 §3.4），所以不导出 FBX / AnimSequence。 |
| 需要 Identity / DNA 吗？ | 单目素材**不需要**。`MonoFootage` 分支只校验图像序列（`MetaHumanPerformance.cpp:1728-1753`）。 |
| 新增工程接入成本？ | 只需 `Doodle.uplugin` 里加 1 个插件条目（`MetaHumanAnimator`），宿主工程的 `.uproject` 一个字都不用改。 |

---

## 1. 为什么不用官方那套「Live Link Hub + Capture Manager UI」

官方标准流程：在 **Live Link Hub** 里加 `Mono Video Ingest` 设备 → 指定视频目录 → ingest → 在编辑器里把 take 交给 **MetaHuman Performance** → 导出。对动画人员有三个硬伤：

1. 要开两个程序并在两者之间配对设备；
2. ingest 产物散落在 `Content/CaptureManager/**`，命名由 slate/take 表达式推导，同名直接报错；
3. 求解失败只给一句泛泛的诊断，说不出是「人脸太小」还是「帧率不对」。

关键事实：**官方明确支持在编辑器内用脚本替代 Live Link Hub**（文档 *Scripting and Automating MetaHuman Animator*，管线 = download → ingest → calibration → process → export）。所以我们的做法是把这条脚本管线搬进 C++，接管设备配对、资产落点、错误诊断。

- [MetaHuman Animator in Unreal Engine](https://dev.epicgames.com/documentation/metahuman/metahuman-animator-in-unreal-engine)
- [Scripting and Automating MetaHuman Animator](https://dev.epicgames.com/documentation/metahuman/scripting-and-automating-metahuman-animator-in-unreal-engine)
- [Download and Ingest takes with Blueprints and Python](https://dev.epicgames.com/documentation/metahuman/download-and-ingest-metahuman-animator-takes-with-blueprints-and-python)

另一条被本方案排除的路线（旧文档走的）：自己抽出 `MetaHumanCoreTechLib` 编独立 Program，出 `MHTechProbe.exe`。已验证能编出来，但卡在 `GEngineLoop::PreInit` 的 ShaderCodeLibrary 缺失，还有 EULA 再分发合规问题。5.7 的官方 API 已经暴露了同样的能力，没必要重复造。

---

## 2. 插件与模块设计

### 2.1 插件启用项

`E:\Doodle\script\uePlug\Doodle\Doodle.uplugin` 的 `"Plugins"` 段当前只有 10 个（EditorScriptingUtilities / AlembicImporter / GeometryCache / LevelSequenceEditor / MovieRenderPipeline / MediaPlayerEditor / GeometryScripting / HairStrands / Interchange / WebBrowserWidget），**没有任何 MetaHuman 与 CaptureManager 插件**。B 路线只需新增一条：

```json
{ "Name": "MetaHumanAnimator", "Enabled": true }
```

说明：
- 引擎里 MetaHuman 插件目录下的 uplugin 叫 `MetaHuman.uplugin`（不是 MetaHumanAnimator.uplugin），但**插件名就是 `MetaHumanAnimator`**（`FriendlyName` 为 "MetaHuman Animator"），`EnabledByDefault` 未设即默认关闭，必须显式启用。
- `MetaHuman.uplugin` 会自行拉起它声明的依赖：`RigLogic`、`NNERuntimeORT`、`ImgMedia`、`CaptureData`、`CaptureManagerEditor`、`LiveLinkHub`、`DNACalib`、`MetaHumanCoreTech`、`MetaHumanSDK` 等 —— 这些**都由引擎按依赖解析自动启用，不用写进 `Doodle.uplugin`**。
- **`CaptureManagerApp` / `CaptureManagerCore` / `CaptureManagerDevices` 全部不启用**。B 路线不用设备链，用不到 `UMonoVideoIngestDevice`，也就省掉了 `ElectraPlayer`、`ControlFlows`、`NamingTokens` 与设备注册时序问题（对比见附录 B）。
- 明确**不要**启用：`CaptureManagerDevices`、`CaptureProtocolStack`（Live Link Hub 跨进程那套）、`MetaHumanCharacter`、`MetaHumanCalibrationProcessing`（深度/标定流程，本方案用不到）。

**宿主工程为什么不用改**：`Doodle.uplugin:13` 是 `"EnabledByDefault": true`，Doodle 插件在任意工程里自动加载；它的 `"Plugins"` 段是一次性的、对全部宿主生效。所以 `E:\ue_main` 以及以后接入的每个工程，`.uproject` 里**一个字都不用加**。这是 B 路线相对 A 路线的核心部署优势 —— A 路线要在每个工程的 `.uproject` 里写 4~6 条插件条目，且每个工程都要单独验证设备能注册上。

### 2.2 新模块 `DoodleMetaHumanFace`

路径 `E:\Doodle\script\uePlug\Doodle\Source\DoodleMetaHumanFace\`，`Doodle.uplugin` 的 `Modules` 段追加：

```json
{
  "Name": "DoodleMetaHumanFace",
  "Type": "Editor",
  "LoadingPhase": "PostConfigInit",
  "WhitelistPlatforms": [ "Win64" ]
}
```

**必须是 `PostConfigInit`，不能用 `PostEngineInit`**。原因（`Engine\Source\Runtime\Launch\Private\LaunchEngineLoop.cpp`）：

- `:3877` 起 commandlet 分支先启动 `UEditorEngine`（`:3971` `GEngine = GEditor = NewObject<UEditorEngine>(...)`，`:3976` `GEditor->InitEditor(this)`）→ 然后才在 `:4010-4012` 加载 `PostEngineInit` 阶段模块 → 最后 `:4034` 才查找 commandlet 类。
- 所以 `LoadingPhase: PostEngineInit` 的模块**能**被加载、类也能注册。但 **`MetaHumanPerformance` 模块本身就是 `Editor/PostEngineInit`**（`MetaHuman.uplugin`），而它的代码強依赖 `GEditor`（`MetaHumanPerformance.cpp:798` 用 `GEditor->GetTimerManager()`）。把我们的模块也放 `PostEngineInit` 就会和它抢同一个阶段，加载顺序不确定。
- `PostConfigInit` 在 `:6762` 加载，远早于引擎启动，是 commandlet 宿主模块的常规选择。

`Type: Editor` + `IsEditor = true` 的 commandlet：`LaunchEngineLoop.cpp:3941` 用 `UCommandlet::IsEditor` 决定造 `UEditorEngine` 还是 `UGameEngine`，`MetaHumanPerformance` 需要真正的编辑器。

`DoodleMetaHumanFace.Build.cs` 依赖（**已逐个核实模块实存**，B 路线 + commandlet 版）：

```
Public:  Core, CoreUObject, Engine
Private: MetaHumanPerformance, MetaHumanCoreTech, MetaHumanCoreTechLib, MetaHumanPipelineCore,
         CaptureDataCore, ImgMedia, MediaAssets,
         Projects, Json, JsonUtilities,
         UnrealEd, AssetRegistry
```

不在这个列表里的就是不需要的：`HTTP` / `Sockets` / `Networking` / `Slate` / `SlateCore` / `DeveloperSettings` 在改成命令行入口后全部移除（没有服务端、没有 UI、没有配置类）。

相比 A 路线，B 路线**去掉**了 `CaptureManagerEditor`、`CaptureUtils`、`DataIngestCore`、`LiveLinkDevice`、`LiveLinkCapabilities`、`LiveLinkHub`、`LiveLinkHubMessaging`、`MonoVideoIngestDevice`、`VideoLiveLinkDeviceCommon`、`MetaHumanCore`、`MetaHumanPipeline`、`ToolWidgets` 共 12 个模块依赖。

**别写这些名字**（引擎里不存在同名模块，写了直接编译失败）：
`CaptureManagerCore`、`CaptureManagerDevices`（是插件名）、`CaptureData`（是 `CaptureDataCore`）、`RigLogic`（是 `RigLogicLib` / `RigLogicDeveloper` / `RigLogicEditor`）、`DNACalib`（是 `DNACalibLib` / `DNACalibModule`）、`XmlRpc`（不存在，上一版方案里的 xmlrpc 是 Python 标准库）。

### 2.3 模块内文件

| 文件 | 职责 |
| --- | --- |
| `DoodleMetaHumanFaceModule.cpp/.h` | `IModuleInterface` 空实现，只是让 commandlet 类被加载 |
| `MetaHumanFaceTypes.h` | `FMetaHumanFaceJobRequest` / `FMetaHumanFaceJobResult` / 阶段与错误码枚举 |
| `MetaHumanFaceCommandlet.cpp/.h` | `UMetaHumanFaceCommandlet : UCommandlet`，进程入口 `int32 Main(const FString& Params)` |
| `MetaHumanFaceCommandLine.cpp/.h` | 把 `-FramesDir=` 之类的 token 解析成 `FMetaHumanFaceJobRequest`，含校验与 `Usage()` |
| `MetaHumanFaceCaptureDataBuilder.cpp/.h` | B 路线：frames 目录 → `UImgMediaSource` + `UFootageCaptureData`（替代 ingest） |
| `MetaHumanFaceSolver.cpp/.h` | `UMetaHumanPerformance` 的创建 / 处理 / 诊断 / 取曲线 |
| `MetaHumanFaceCurveWriter.cpp/.h` | 写 `.dhfc`（§5） |

---

## 3. 三段管线（B 路线）

### 3.1 P1：Doodle 侧出逐帧图像

**这是我们不做的那一段**，但要把它对 UE 侧的约束写清楚，因为它是 B 路线唯一的耦合点。

Doodle 侧负责：读视频 → 逐帧解码 → 写图像文件到 `<OutRoot>/<jobId>/frames/`。UE 侧只接受这个目录，不再碰视频。

必须满足的约定：

| 项 | 要求 | 为什么 |
| --- | --- | --- |
| 图像格式 | Windows 上 `UImgMediaSource` 走 ImgMedia 的 `EXRSequence`/`JPEGSequence` 等 `IImgMediaReader`，默认可用的是 **JPG**（引擎自带 `UJpegImageWrapper`）；PNG 要确认 ImgMedia 侧 reader 是否注册 | 格式不支持时 `FUEImageLoadNode` 在启动阶段就 exit，报 `Failed to find image sequence`，表现为"0 帧"，排查很费劲 |
| 命名 | 遵循 ImgMedia 的序列 token 规则：`<Slate>_<Take>_<Frame>`，帧号至少 4 位、补零、连续。**V1 必须以引擎实测为准一次性钉死，并写成断言** | `FTrackingPathUtils::GetTrackingFilePathAndInfo` 解析失败 → `MetaHumanPerformance.cpp:876-890` 直接把管线 exit，`StartPipeline` 仍返回 `None` |
| 帧率 | 由 UE 侧以有理数写进 `UImgMediaSource::FrameRateOverride`（`FFrameRate`），不经过 float | ingest 那条路的 `AsDecimal()` 会让 29.97 失真；B 路线没有这个问题 |
| 色彩 | 求解链不读 EXIF，不做自动旋转。**Doodle 侧必须把画面摆正、sRGB**，否则人脸检测直接失败 | `FHyprsenseRealtimeNode` 只在正立画面上工作 |
| 掉帧 | Doodle 侧自己在写图前剔除，或把区间写进一个 `frames_meta.json` 由我们转成 `CaptureExcludedFrames` | `FFrameRange::ContainsFrame(Frame, CaptureExcludedFrames)`（`MetaHumanPerformance.cpp:658`）会把这些帧排除掉 |

### 3.2 P2：frames → `UFootageCaptureData`（B 路线核心）

不建设备、不 ingest，直接构造：

```cpp
UImgMediaSource* Source = NewObject<UImgMediaSource>(Outer);
Source->SetTokenizedSequencePath(SequenceDirectory);   // 与 IngestAssetCreator.cpp:299-310 同款
Source->FrameRateOverride = FrameRate;                 // 有理数，代替 ingest 的 float
Source->StartTimecode     = Timecode;

UFootageCaptureData* Data = NewObject<UFootageCaptureData>(Outer);
Data->ImageSequences.Add(Source);                      // 单目求解只需要这一项
Data->CaptureExcludedFrames = ExcludedRanges;          // 可为空
```

`UFootageCaptureData` 的公开成员（`CaptureData.h:199-244`）：

```cpp
TArray<TObjectPtr<UImgMediaSource>> ImageSequences;        // 单目求解的必需项
TArray<TObjectPtr<UImgMediaSource>> DepthSequences;        // 单目留空
TArray<TObjectPtr<USoundWave>>      AudioTracks;           // 有音频才挂 TongueSolve
TArray<FFrameRange>                 CaptureExcludedFrames; // 掉帧区间
FFootageCaptureMetadata             Metadata;
```

**B 路线的两个专属风险（A 路线自动规避，这里必须自己兜）**：

1. **文件命名与 `FFramePathResolver` 对不上**。ingest 那条路由 `IngestAssetCreator` 保证命名，B 路线由 Doodle 写文件，规则错一位就 `GetTrackingFilePathAndInfo` 失败 → 管线 exit → `StartPipeline` 仍返回 `None`、`GetNumberOfProcessedFrames()` 为 0。V1 冒烟必须把它们钉死并写成启动断言。
2. **`Metadata` / `CameraCalibrations` 得自己填**。`CanProcess` 的单目分支只做 `VerifyData(EInitializedCheck::ImageSequencesOnly)`，**不校验元数据自洽性**。`FrameRateOverride` 决定 `FFrameNumberTransformer` 的换算，填错时求解不报错但结果整体偏移。

资产落点自己指定，全程不碰工程 `/Game/`：

```
<OutRoot>/<jobId>/frames/     ← Doodle 侧产出（保留，失败时可人工回看）
<OutRoot>/<jobId>/capture/    ← UFootageCaptureData + UImgMediaSource（可 Transient，结束即弃）
<OutRoot>/<jobId>/curves/     ← .dhfc 输出
<OutRoot>/<jobId>/log/        ← 失败时保留
```

**资产创建的两个坑**（`MetaHumanPerformance.cpp:189-287` 的 `PostEditChangeProperty`）：

- 新建的 `UMetaHumanPerformance` **默认 `InputType == DepthFootage`**，必须显式改 `MonoFootage` 并让 `PostEditChangeProperty` 跑一遍，否则帧率校验、帧范围重算、`ResetOutput` 都不会发生。Python 示例注释里说的"必须用 `set_editor_property` 而不是直接赋值"就是指这个。
- 同一处会调 `FMessageDialog::Open`（`:261`）弹「帧率非法」对话框。**commandlet 里没人点确定，会一直挂着**。所以我们的顺序是：先构造并校验好 `UFootageCaptureData`（帧率分子非零），再赋给 `FootageCaptureData`。
- 反过来，直接 `Perf->StartFrameToProcess = X` 不做 `PostEditChangeProperty` 是**可以**的（`:204` 那条分支只做 Clamp），但 Sprite 顺序上要在 `FootageCaptureData` 赋值之后。**默认值不用管**：`FootageCaptureData` 一赋值，`:267-268` 就已经把整个 `ProcessingLimitFrameRange` 填好了。

**帧率**：B 路线用 `FFrameRate` 有理数，`29.97` 写 `30000/1001`，`23.976` 写 `24000/1001`。`.dhfc` 里存分子/分母，不存 double。

### 3.3 P3：求解（核心）

```cpp
UPackage* Pkg = CreatePackage(*PackageName);
UMetaHumanPerformance* Perf = NewObject<UMetaHumanPerformance>(Pkg, UMetaHumanPerformance::StaticClass());

Perf->InputType          = EDataInputType::MonoFootage;  // 新建默认是 DepthFootage，必须显式改
Perf->FootageCaptureData = CaptureData;
Perf->bHeadStabilization = true;     // 头位姿减抖动；要「头不动只动表情」时尤其重要
Perf->bSkipTongueSolve   = true;     // 没有音频时本来也不挂这个节点，显式关掉更安全
Perf->bShowFramesTheyAreProcessed = false;

if (!Perf->CanProcess()) { return fail(阶段 Solve, 码 NoFrames); }

const EStartPipelineErrorType Err = Perf->StartPipeline(true);
```

必须记住的 7 条：

1. **不需要 Identity / DNA / Identity 训练**。`MetaHumanPerformance.cpp:1728-1753` 的 `CanProcess` 在 `InputType == MonoFootage` 时只调 `FootageCaptureData->VerifyData(EInitializedCheck::ImageSequencesOnly)`；只有 `DepthFootage` 分支才要求 `IFaceTrackerNodeImplFactory`（Fab 付费的 Depth Processing 插件）+ `Identity` + `RunPredictiveSolverTraining`。
2. **`SetBlockingProcessing(true)` 对单目无意义**。`MetaHumanPerformance.cpp:777-790`：`if (InputType == Audio || InputType == MonoFootage) { StartPipelineStage(); }` —— 单目本来就在当前栈上同步跑完。
3. **`StartPipeline` 的返回值不可信**，只有 `{None, NoFrames, Disabled}` 三种。真正的成败用硬判据：
   ```cpp
   const bool bOk = Perf->ContainsAnimationData() && Perf->GetNumberOfProcessedFrames() > 0;
   ```
   再加 `DiagnosticsIndicatesProcessingIssue(FText& OutText)` 拿诊断文本。
4. **全局单实例**：`MetaHumanPerformance.cpp:1715-1717` 有 `CurrentlyProcessedPerformance`，一次只能跑一个。命令行形态下「一次进程一个序列」天然满足（§6）。
5. **杆位校验一次要早**：`FHyprsenseRealtimeNode::LoadModels()`（`MetaHumanCoreTechLib/Source/MetaHumanPipelineCore/Private/Nodes/HyprsenseRealtimeNode.cpp:688-719`）硬性依赖三件事，任一失败只打一条 `LogMetaHumanPipeline` Warning 然后返回 false —— 上层看到的就是「帧数 0」这个空洞的失败。三件事：
   - `LoadObject("/MetaHumanCoreTech/GenericTracker/FaceDetector.FaceDetector")`
   - `LoadObject("/MetaHumanCoreTech/RealtimeMono/landmark_tracker_v0_6.landmark_tracker_v0_6")`
   - `LoadObject("/MetaHumanCoreTech/RealtimeMono/generic_rig_solver_v0_5_pt_onnx_fp16.generic_rig_solver_v0_5_pt_onnx_fp16")`
   - `GetRuntime<UE::NNE::INNERuntimeGPU>("NNERuntimeORTDml")`

   → 必须在任务开始前做一次 `LoadObject` + `GetRuntime` 预热检查，失败就报「MetaHuman 模型/推理后端缺失（NNERuntimeORTDml）」，而不是让用户看到一个空洞的 0 帧。
6. **求解链**（`MetaHumanPerformance.cpp:860-885`）：`FUEImageLoadNode("Color")` → `FNeutralFrameNode` → `FHyprsenseRealtimeNode("RealtimeMonoSolver")` → `[TongueSolve]` → `FHyprsenseRealtimeSmoothingNode`。`TongueSolve` 只在 `GetAudioForProcessing() != nullptr && !bSkipTongueSolve` 时挂上。
7. **首帧中性脸**：`FNeutralFrameNode` 取第一有效帧作中性基。素材前 0.5 秒必须是自然表情，否则整段表情整体偏移。这是素材标准里最容易踩的坑。

另外两个细节：
- `FocalLength` 是 `VisibleAnywhere`（`MetaHumanPerformance.h:298-299`），外部写不了；C++ 里可以调非 UFUNCTION 的 `Perf->EstimateFocalLength(OutErrorMessage)`，把估计值写进结果日志。
- `MonoSmoothingParams`（`MetaHumanPerformance.h:306-307`）默认从 `/MetaHumanCoreTech/RealtimeMono/DefaultSmoothing` 加载（`MetaHumanPerformance.cpp:111-112`），不设置就用默认。

### 3.4 P4：取数据 / 导出

**A. 直接读曲线（默认，主交付）**

```cpp
for (int32 i = 0; i < Perf->GetNumberOfProcessedFrames(); ++i)
{
    const FFrameAnimationData& Frame = Perf->GetAnimationData(i);
    // Frame.AnimationData:    TMap<FString, float>   ← 251 条面部控制曲线名 → 权重（平滑后）
    // Frame.Pose:             FTransform             ← 头部位姿（根骨，平滑后）
    // Frame.RawAnimationData ← 单目场景恒空，不要依赖（附录 C）
}
```

`FFrameAnimationData` 在 `MetaHumanCoreTechLib/Source/MetaHumanCoreTech/Public/FrameAnimationData.h:31`，`AnimationData` 是 `BlueprintReadOnly` 的 `TMap<FString,float>`。**这是完整的 MetaHuman 控制集合**，比 AnimSequence 拿到的多得多。我们直接把这套值写进 `.dhfc`，不经 AnimSequence 中转。

**为什么不走 AnimSequence / FBX**：`UMetaHumanPerformanceExportUtils::ExportAnimationSequence` 写完曲线后执行 `RemoveAllBoneTracks()`，然后只 `AddBoneCurve("root")`（为挂 timecode 属性，`MetaHumanPerformanceExportUtils.cpp:2279-2285`）。面部相关只剩 7 条头部曲线：

`HeadYaw` / `HeadPitch` / `HeadRoll` / `HeadTranslationX` / `HeadTranslationY` / `HeadTranslationZ` / `HeadControlSwitch`

而且**只有 Skeleton 的 `CurveMetaData` 里已存在同名曲线时才写**（`:2199-2250` 的 `SkeletonCurveNames.Contains(...)`），`IsTargetSkeletonCompatible` 只在 UI 的 `ExportClicked` 里被调用（`:355-380`，弹对话框），**无 UI 路径完全不校验** —— 曲线名对不上就静默少写。再经 `UAnimSequenceExporterFBX` 转 FBX，拿到的还是一模一样的东西。

结论：**不实现 AnimSequence / FBX 导出，也不依赖 `Face_Archetype_Skeleton`**。唯一交付产物是 `.dhfc`（`.dhfc.json` 只是人读摘要）。

---

## 4. 素材标准（写进给动画人员的说明 + Doodle 侧出图的约束）

**注意章节归属**：B 路线下，"容器/编码/解码"这几项属于 **Doodle 侧**（它解帧），UE 侧只从图像序列开始接手。所以下表分两段。

### 4.1 Doodle 侧（视频 → 图像）

| 项 | 要求 | 违反时 |
| --- | --- | --- |
| 容器 / 编码 | 由 Doodle 的 decoder 决定，不再受 `mp4`/`mov` 限制 | 解不出即失败 |
| 输出格式 | **PNG**（已定），文件名 `frame_000000.png`；命名与数字位数的校验口径见 `MetaHumanFaceCaptureDataBuilder.cpp:20,212-222` | V1 实测确认 `FTrackingPathUtils` 接受该命名 |
| 输出命名 | 6 位补零、全序列位数一致、帧号从 0 连续无空洞 | 位数不一致或帧号有空洞 → `FramesFormatMismatch` / `FramesNotFound` |
| 画面 | 正立、sRGB，不依赖 EXIF 旋转 | 人脸检测直接失败 |
| 帧率 | 以有理数 `FFrameRate` 传给 UE | 变帧率失败 |

### 4.2 UE 侧预检（拿到 frames 目录之后）

| 项 | 要求 | 违反时 |
| --- | --- | --- |
| 分辨率 | 短边 ≥ 640，建议 1280×720 起 | 仅警告 |
| 人脸占比 | 正脸，头部高度 ≥ 画面高度 20% | 仅警告 |
| 首段 | 前 0.5 秒中性脸（闭口、自然表情） | 警告（影响整段基准） |
| 时长 | 单段 ≤ 10 分钟 | 超长警告 |
| 音频 | 可选；有音频且未 `bSkipTongueSolve` 才走舌头求解 | 无音频正常 |

---

## 5. 交付格式 `.dhfc`（Doodle Human Face Curves）

自定义二进制，**小端**。

```
offset  type       field
0       char[8]    magic           "DHFC0001"
8       uint32     version         1
12      uint32     flags           **恒为 bit1（含头部位姿）**；bit0 保留给「含平滑前 RawAnimationData」，当前不写
16      uint32     frame_count
20      uint32     curve_count
24      uint32     fps_num         帧率分子（如 30000）
28      uint32     fps_den         帧率分母（如 1001）
32      float32    start_time_sec
36      uint32     reserved[3]
48      curve_count × { char name[64]; }          定长零填充，ASCII
48+K    frame_count × curve_count × float32      行优先：先第 0 帧全部曲线
尾部（flags bit1）：frame_count × { float32 tx,ty,tz; float32 qx,qy,qz,qw; }   ← FFrameAnimationData::Pose
```

- **帧率用"分子/分母"两个 uint32，不用 double 也不用 float**。B 路线全程以 `FFrameRate` 有理数流转，`29.97` 就是 `30000/1001`，不存在 ingest 那条路 `AsDecimal()` 的失真。
- **不含平滑前数据**（已拍板）。`flags` 的 bit0 只是占位：以后若要做，按附录 C 的方案 3 跑第二条管线，`bit0 = 1` 时在尾部追加一段与主数据段同构的曲线块，读端向后兼容。
- `name` 直接复制 `FFrameAnimationData::AnimationData` 的 key（`HyprsenseRealtimeNode.cpp:1202` 的 `check == 251` 说明单目固定 251 条）。
- **曲线集合以首次跑通时的实际 key 为准，不在代码里硬编码**。V1 冒烟的核心产出就是把它打印出来固化成常量表 + 「引擎升级后 key 集合变化」的启动校验。
- 伴随 `xxx.dhfc.json`（人可读摘要：帧数、fps、曲线名、每曲线 min/max/首末值），给动画人员核对。
- 可选压缩：`flags bit2` 时按帧分块 zlib（块头 `uint32 uncompressed_size`）。先不做，看实测体积再定。

---

## 6. 入口形态：命令行 commandlet，一次一个序列

**没有服务端，没有常驻进程，没有 HTTP，没有队列。**

```
UnrealEditor-Cmd.exe <Project>.uproject -run=MetaHumanFace -FramesDir=... -OutDir=... -FPS=24000/1
```

`UMetaHumanFaceCommandlet : public UCommandlet`，`int32 Main(const FString& Params)` 就是整个程序。

### 6.1 为什么 commandlet 可行（已核实）

| 环节 | 位置 | 结论 |
| --- | --- | --- |
| 类发现 | `LaunchEngineLoop.cpp:3885` `StaticFindFirstObject<UClass>`，`:3887-3892` 找不到且带 `.` 时按前缀 `LoadModule` 再试 | 宿主模块在 `PostConfigInit`（`:6762`）加载，早于 commandlet 分支，类必然已注册 |
| 引擎类型 | `:3930-3941` 用 `UCommandlet::IsEditor` 决定造 `UEditorEngine` 还是 `UGameEngine` | `IsEditor = true` → 真正的编辑器引擎，`MetaHumanPerformance` 需要的 `GEditor` 存在 |
| 参数 | `:4033 Commandlet->ParseParms(CommandletCommandLine)`，`CommandletCommandLine` 来自 `:2194` 把命令行 token 重新拼装 | `Params` 就是整条命令行的 token 串，可自由解析；**也能用 `FCommandLine::Get()` 直接读全局** |
| 退出码 | `:4199 return ErrorLevel`；`:4191-4196` `FastExit` → `RequestExitWithStatus` | `Main` 的返回值就是进程退出码 |
| 日志 | UAT 的 `RunCommandlet`（`AutomationTool/AutomationUtils/CommandletUtils.cs:321-329`）用 `-run={0} {1} -abslog={2} -stdout -CrashForUAT -unattended -NoLogTimes` | `-abslog=xxx.log` 写盘（`GenericPlatformOutputDevices.cpp:84-90`，**扩展名必须是 `.log` 或 `.txt`，否则被忽略**），`-stdout` 同时回显 |

### 6.2 命令行约定

| 参数 | 必填 | 说明 |
| --- | --- | --- |
| `-FramesDir=<绝对路径>` | 是 | 逐帧图像目录 |
| `-OutDir=<绝对路径>` | 是 | 输出根目录，每次运行独立 |
| `-FPS=<num>/<den>` | 否 | 有理数帧率，默认 `24000/1` |
| `-Range=<first>:<count>` | 否 | 默认全序列 |
| `-SequencePattern=<模板>` | 否 | 默认 `{frame}`；**调用方可以完全不传**（只校验"传了但不含 `{frame}`"），帧命名固定为 `frame_000000.png` |
| `-PackagePath=<长包名>` | 否 | 中间资产落点，默认 `/Temp/MetaHumanFace/<runId>`；调用方通常显式传 `/Game/...` |
| `-NoHeadStabilize` | 否 | 关掉 `bHeadStabilization` |
| `-KeepTongueSolve` | 否 | 打开 `bSkipTongueSolve=false` |

**起始时间码**：`ResolveStartTimecode`（`MetaHumanFaceCaptureDataBuilder.cpp:312-340`）优先读 `-OutDir` 下的 `frames_meta.json` 的 `first_frame`，其次用扫描到的起始帧号，最后才用墙钟。调用方不写 `frames_meta.json` 时求解结果本身不会报错，但与序列动画对不齐，所以**调用方应当在 `-OutDir` 里写 `{"first_frame": n}`**。

**退出码**：`0` 成功，`2` 参数/素材/CaptureData 失败，`3` 模型或推理后端缺失，`4` 求解失败，`5` 导出或写盘失败（`MetaHumanFaceCommandlet.cpp:449-469`）。结果同时写在 `-OutDir/face_capture_log.json`，**调用方应优先读它而不是只看退出码**。

**路径用 `-Key=Value` 形式、整体加引号**（`"-FramesDir=E:\素材\Shot_010"`）。因为引擎自己会扫命令行，值里以 `-` 开头的裸 token 会被当成引擎开关；`=` 形式把值绑在同一个 token 里，不会被引擎的 tokenizer 拆开。相对路径一律拒绝，只收绝对路径。

### 6.3 每次一个序列的语义

- **一个进程 = 一个序列 = 一个输出目录。** 冷启动 30-60 秒的代价由调用方批量调用时逐次付出；换来的是完全无状态的隔离 —— 上一次运行残留的资产/全局状态不可能污染下一次。
- `CurrentlyProcessedPerformance` 单实例限制（`MetaHumanPerformance.cpp:1715-1717`）**天然满足**，不需要队列。
- 中间资产落在 `/Temp/...`（不写工程 `Content`），进程结束时删包；`-KeepIntermediate` 才保留。
- 失败即失败：退出码非 0，产物（frames 目录、日志）原样保留。**退出码约定**：`0` 成功，`1` 参数错，`2` 素材错，`3` 模型/推理后端缺失，`4` 求解失败，`5` 写盘失败。
- 诊断信息同时出现在两处：`-abslog` 日志里（人看），以及 `-OutDir/result.json`（机器读）。

### 6.4 并发策略

并发由调用方控制粒度：**一个序列一个进程**，同时跑 N 个序列就是 N 个 `UnrealEditor-Cmd`。代价是 N × 内存与 N × 冷启动，收益是零串行协调、零共享状态。因为 B 路线下求解是纯 CPU/GPU 计算，没有 DDC 写冲突也没有资产包冲突（各自 `/Temp` 独立），并发是安全的。

---

## 7. 宿主工程

插件本身**不编译** —— `E:\Doodle\script\uePlug` 下没有 `.uproject`，插件必须由外部宿主工程加载。

**关键前提**：`Doodle.uplugin:13` 是 `"EnabledByDefault": true`。这意味着 Doodle 插件在**任意工程**里都会自动加载，而它的 `"Plugins"` 段（§2.1 里加的 `MetaHumanAnimator`）也随之一并对所有宿主生效。**所以宿主工程的 `.uproject` 不需要任何改动** —— 这是 B 路线在"会有多个工程接入"这个现实下的最大优势。

### 7.1 V1 宿主：`E:\ue_main`

- 现状：`.uproject` 只启用 5 个插件，**没有显式列 `Doodle`**；但 `E:\UnrealEngine\Engine\Plugins\Doodle\Binaries\Win64\` 下已有 8 个编译好的模块 DLL（`UnrealEditor-doodle.dll` 等），且 `E:\ue_main\Config\DefaultEditor.ini:33` 里有 `[/Script/DoodleLightingEditor.DoodleLightingSettings]` 段 —— 说明它确实在加载 Doodle 插件，走的是 `EnabledByDefault` 自动加载。
- 用途：**只验证能编译、能跑通冒烟**，不作为部署目标。
- 需要做的：改完 `Doodle.uplugin` 后用 `ue_build` 编一次 `ue_mainEditor`，确认新模块编进 `Doodle/Binaries/Win64/UnrealEditor-DoodleMetaHumanFace.dll`。
- 遗留小问题：是否在 `ue_main.uproject` 的 `Plugins` 里显式补一条 `"Name": "Doodle"`。我倾向补（自包含、可复现），但不阻塞实现。

### 7.2 部署形态

- 哪个工程要跑面捕就用哪个工程当宿主；模块是 `Editor` 类型，不影响打包产物。
- 一次调用一个序列：

```
UnrealEditor-Cmd.exe <Project>.uproject -run=MetaHumanFace ^
    "-FramesDir=E:\DoodleFaceTmp\Shot_010\frames" ^
    "-OutDir=E:\DoodleFaceTmp\Shot_010\curves" ^
    -FPS=24000/1 -abslog="E:\DoodleFaceTmp\Shot_010\run.log" -stdout ^
    -unattended -nologo -nosplash -NoLogTimes
```

注意：
- 不要用 `-nullrhi` 赌：求解走 `NNERuntimeORTDml`（DirectML），本身不依赖 RHI，但 MetaHuman 其它资产加载要正常初始化的引擎；先带 RHI 跑通，再试能否省掉。
- `-abslog` 的路径**扩展名必须是 `.log` 或 `.txt`**（`GenericPlatformOutputDevices.cpp:92-97` 直接丢弃其他扩展名）。
- 路径里的空格与中文：整体加引号即可，但**不要**把业务参数拼成裸 token（见 §6.2）。
- **不要用 `-ExecutePythonScript`** 传业务参数：UE 自己会解析命令行，业务参数一律走 `-Key=Value`。

---

## 8. 验证计划（三步，每步可单独交付）

### V1 冒烟：确认 commandlet 能跑起来、曲线集合、图像命名（半天，宿主 `E:\ue_main`）

不做完整任务流：先只做「解析命令行 + 读 frames 目录 + 跑求解 + 打印曲线」。直接喂一份现成 JPEG 序列目录，跑：

`FUEImageLoadNode → HyprsenseRealtimeSolver → Smoothing → GetAnimationData`

验收物（5 条，缺一不可）：

1. **`-run=MetaHumanFace` 能被找到**（模块 `PostConfigInit` 加载）并返回预期退出码；日志里出现 `Executing ...MetaHumanFaceCommandlet`。
2. `AnimationData` 的**完整 key 列表**（应为 251 条；决定 `.dhfc` 曲线表）。这份列表同时作为「引擎升级后曲线表变化」的基线。
3. **图像序列命名规则**：用 `SetTokenizedSequencePath` 能正确解析的最简文件命名。这条要写成断言，是 B 路线唯一的强耦合点。
3. 单帧/整段耗时 → 校验 20 分钟约束。
4. `NNERuntimeORTDml` + 3 个 `LoadObject`（FaceDetector / landmark_tracker / generic_rig_solver）是否全部可用（§3.3 第 5 条）。
5. 顺带：打印平滑前后的差异（同一帧同一条曲线，`HyprsenseRealtimeSolver` 输出 vs `Smoothing` 输出各打一条日志），为将来是否要做平滑前数据留判断依据。

### V2 任务链：frames → CaptureData → 曲线（1 天）

在 V1 基础上补 `MetaHumanFaceCaptureDataBuilder` 与 `MetaHumanFaceCurveWriter`，串成一次完整调用：
frames 目录 + 有理数帧率 → `UFootageCaptureData` → `UMetaHumanPerformance` → `.dhfc`。

验收：同一批素材跑两次结果 bit 级一致；帧率 29.97 存成 `30000/1001`；掉帧区间正确进 `CaptureExcludedFrames`；失败时 frames 目录与日志完整保留。

### V3 命令行收口 + Doodle 接入（2 天）

补 `MetaHumanFaceCommandline` 的完整解析与校验、退出码映射、`result.json`、失败诊断文案；Doodle 后端改成起进程 + 读退出码 + 读 `result.json`。

---

## 9. 风险与对策

| 风险 | 影响 | 对策 |
| --- | --- | --- |
| Epic 账号/EULA 授权 | 首次启用 MetaHumanAnimator 可能要登录验证 | 官方 ReadMe 明确：**只有身份创建与 level sequence 导出**需要引擎带 UI 在后台；**我们的链路（capture data + mono performance + anim sequence 导出）都在 unattended 范围内**。依然要在一台工程师机器上先手工启一遍并提交产出的模板资产 |
| 模型/NNE 后端缺失 | 求解静默返回 0 帧 | 启动时预热校验 3 个 `LoadObject` + `NNERuntimeORTDml`，失败即明确报错（§3.3 第 5 条） |
| **图像序列命名与 `FFramePathResolver` 不匹配**（B 路线首风险） | 管线启动即 exit，`StartPipeline` 仍返回 `None`，表现为 0 帧 | V1 用实测钉死命名规则并写成启动断言（§3.2、§8 V1-3） |
| **`UFootageCaptureData` 元数据自洽性无校验**（B 路线） | `FrameRateOverride` 填错时求解不报错、结果整体偏 | 自己校验帧率/尺寸/时间码；`.dhfc` 存分子分母 |
| **无 UI 下 `PostEditChangeProperty` 弹窗卡死** | commandlet 卡死 | 先构造并校验好 CaptureData 再赋值给 `FootageCaptureData`（§3.2） |
| 面部控制曲线集合随版本变 | `.dhfc` 曲线表对不上 | 启动比对常量表与实际 key，不一致直接失败并输出 diff |
| 冷启动开销 | 每序列 30-60 秒 | 调用方掌控批量粒度；序列切分越粗总开销越小 |
| 20 分钟约束 | 长素材超时 | 单段 ≤ 10 分钟硬限制；预检按时长估算耗时并提前失败 |

---

## 10. 与旧方案的取舍

`docs/metahuman_face_capture_plan.md` 走的是「自己抽 `MetaHumanCoreTechLib` 编独立 Program，直接出 blendshape 权重」，能编出 208MB 的 `MHTechProbe.exe`，但卡在 `GEngineLoop::PreInit` 的 ShaderCodeLibrary 缺失，还有 EULA 再分发合规问题。

本次核实证明：**5.7 的官方 C++ API 已经把同样的能力暴露出来了**（`UMetaHumanPerformance` 自己编排 `FHyprsenseRealtimeNode`）。所以：

- 放弃「独立 Program」路线，改为「编辑器内模块」——一个 UE 进程解决全部问题。
- 旧的 DNA/RigLogic 侧投入**不浪费**：`vcpkg` 的 riglogic/dnacalib、`src/maya_plug/maya_comm/dna_calib_import.cpp` 将来要把曲线落到具体 MetaHuman 的 DNA rig 上、或在 Maya 侧直接驱动时仍要用。
- 旧文档所述路线已放弃，不再推进。

---

## 11. 决策记录

| # | 决策 | 结论 | 理由 |
| --- | --- | --- | --- |
| 1 | ingest 路线 | **B 路线**（Doodle 出 frames，UE 只构造 CaptureData） | 少 12 个模块 / 4 个插件依赖；无 `Private` 头；无设备注册时序；帧率有理数无失真；错误完全可控；多工程接入只需改一次 `Doodle.uplugin`（附录 B 有逐项对比） |
| 2 | `.dhfc` 是否含平滑前数据 | **不含** | 单目管线里 `RawAnimationData` 恒空（写它的是 DepthFootage 专属节点）；要拿真正的平滑前值必须绕过 `UMetaHumanPerformance` 自搭第二条管线，等于跑两遍模型，与 20 分钟约束冲突。`flags` bit0 留作扩展（附录 C） |
| 3 | V1 宿主 | **`E:\ue_main`** | 现成工程，只验证能编译能跑通，不作为部署目标 |
| 4 | 引擎插件启用位置 | **`Doodle.uplugin` 的 `"Plugins"` 段**，只加 `MetaHumanAnimator` 一条 | `Doodle.uplugin:13` 的 `EnabledByDefault: true` 让 Doodle 在任意工程自动加载，其依赖段对所有宿主生效，因此各工程的 `.uproject` 无需改动 |
| 5 | 是否导出 FBX / AnimSequence | **不导出，只出 `.dhfc`** | AnimSequence 经 `RemoveAllBoneTracks()` 后只剩 7 条头部曲线、没有面部骨骼；FBX 是它的下游，同样没有脸；而且都要绑 `Face_Archetype_Skeleton` 与其曲线表，多一层无用依赖 |
| 6 | 调用入口形态 | **命令行 commandlet，一次一个序列** | 删除 HTTP 服务端、常驻队列、`FHttpServerModule` 三块；无 IPC、无端口、无状态泄漏；单实例限制天然满足；失败=非零退出码，调用方按进程管理即可 |
| 7 | 模块 LoadingPhase | **`PostConfigInit`**（不是 `PostEngineInit`） | `MetaHumanPerformance` 自己是 `PostEngineInit` 且依赖 `GEditor`，同阶段会抢加载顺序；`PostConfigInit` 早于引擎启动，commandlet 查找类时必然已注册 |

**仍未定（不阻塞实现）**：

1. 是否在 `E:\ue_main` 的 `.uproject` 里显式补 `"Name": "Doodle"`（我倾向补，自包含可复现）。
2. 是否需要「一段素材 → 多段区间 / 多个演员」的批量能力。

---

## 附录 A：B 路线相对 A 路线省掉的依赖

| 依赖项 | A 路线（设备链） | B 路线 |
| --- | --- | --- |
| 需启用的插件 | `MetaHumanAnimator` + `CaptureManagerApp` + `CaptureManagerCore` + `CaptureManagerDevices` + `CaptureManagerEditor`（+ 自动带 `ElectraPlayer`/`ControlFlows`/`NamingTokens`/`LiveLinkHub`） | **只需 `MetaHumanAnimator`** |
| V1 的 `Build.cs` 模块 | +12 个（`CaptureManagerEditor`/`CaptureUtils`/`DataIngestCore`/`LiveLinkDevice`/`LiveLinkCapabilities`/`LiveLinkHub`/`LiveLinkHubMessaging`/`MonoVideoIngestDevice`/`VideoLiveLinkDeviceCommon`/`MetaHumanCore`/`MetaHumanPipeline`/`ToolWidgets`） | 无 |
| `Private` 头 | `UMonoVideoIngestDevice` / `UMonoVideoIngestDeviceSettings` 都在 `Private/` | 全部用 Public 头 |
| 设备注册时序 | 要保证 `MonoVideoIngestDevice` 先于 `ULiveLinkDeviceSubsystem` 加载 | 无 |
| 帧率精度 | `AsDecimal()` → float，29.97 失真 | `FFrameRate` 有理数 |
| 错误粒度 | `UIngestCapability_ProcessResult::Error(FText)`，拿不到"第几帧为什么" | 只有求解一段会失败，分类干净 |
| 工程 Content 污染 | 默认往 `/Game/CaptureManager/...` 写资产 | 零 |

## 附录 B：A 路线（设备链）的技术要点，仅备查

不实现。若将来 Doodle 侧出图质量与 CaptureManager 不一致（色彩空间、EXIF 旋转），可照此补回。

- 建设备：`GEngine->GetEngineSubsystem<ULiveLinkDeviceSubsystem>()->CreateDeviceOfClass(UMonoVideoIngestDevice::StaticClass(), Settings)`，范式抄 `ULiveLinkHubCaptureDevice.cpp:318-335`。
- 连接：`ILiveLinkDeviceCapability_Connection::Execute_Connect` 在 `AnyThread` 异步，必须挂 `ConnectionChanged` 委托 + `FEventRef::Wait`（`ULiveLinkHubCaptureDevice.cpp:100-129`）。
- 不要用 `ULiveLinkHubCaptureDevice` 本体：头在 `CaptureManagerEditor/Private/Scripting/`，`ImplPtr` 私有，`Stop()` 还会 `RemoveDevice`。
- 设备设置：`UMonoVideoIngestDeviceSettings{DisplayName, TakeDirectory, VideoDiscoveryExpression="<Auto>"}`。
- 硬限制：`MonoVideoIngestDevice.cpp:32-36` 只认 `mp4`/`mov`。
- 帧率失真点：`MonoVideoIngestDevice.cpp:283`。
- 掉帧：`IngestCaptureDataProcess.cpp:330-352` 聚合进 `CaptureExcludedFrames`。
- 资产落点默认值：`CaptureManagerEditorSettings.cpp:31-32`（`CaptureManager/Media/{project}/{device}/{slate}_{take}`），必须用 `UIngestCapability_Options.WorkingDirectory/DownloadDirectory` 重定向。
- 设备头在 `Private/` 的绕法：`FindObject<UClass>(nullptr, TEXT("/Script/MonoVideoIngestDevice.MonoVideoIngestDevice"))`。

## 附录 C：平滑前数据的真实情况（供将来决策）

- `FHyprsenseRealtimeNode` 只写 `AnimationData`（`HyprsenseRealtimeNode.cpp:1195`），**从不写 `RawAnimationData`**；写 `RawAnimationData` 的是 `FFaceTrackerNode`（`FaceTrackerNode.cpp:150`、`:449`），那是 `DepthFootage` 专属节点。**所以单目场景下 `RawAnimationData` 恒空** —— 这是"暂不做"的根本原因。
- 单目真正的"平滑前 vs 平滑后"在 `FHyprsenseRealtimeNode` 与 `FHyprsenseRealtimeSmoothingNode` 之间：
  - 求解节点输出 251 条表情曲线（`HyprsenseRealtimeNode.cpp:1202` 的 `check`）+ 头部位姿；
  - 平滑节点在此基础上**再加 6 条头部曲线一起平滑**（`HyprsenseRealtimeSmoothingNode.cpp:45-50`：`HeadRoll/HeadPitch/HeadYaw/HeadTranslationX/Y/Z`），结果**写回同一个 `AnimationData`**（`:77-80`），位姿折算回 `Pose`（`:83-95`）。
- `UMetaHumanPerformance` 保存的是**平滑后**的值：`AnimationResultsPinName` 在 `MetaHumanPerformance.cpp:912` 指向 `Smoothing.Animation Out`，`:2016-2017` 直接从该 pin 取。
- 将来若真需要平滑前值，三条路：
  1. **调平滑参数**（`MonoSmoothingParams`，`MetaHumanPerformance.h:306-307`）—— 不改格式、不跑两遍，能满足绝大多数调参诉求。**首选**。
  2. 管线外重算 —— **走不通**，拿不到平滑前输入。
  3. 绕过 `UMetaHumanPerformance`，照 `MetaHumanPerformance.cpp:860-916` 自搭 `FUEImageLoadNode → FNeutralFrameNode → FHyprsenseRealtimeNode` —— 唯一能拿到真正原始值的办法，但要自己复刻 `LoadModels`、`FocalLength` 估计、`HeadToRoot` 位姿变换（`:2019-2033`），且**必须跑两遍模型**（一遍拿原始、一遍走官方拿平滑后）。
- 判断依据：V1 冒烟第 6 条会打印同一帧同一曲线在平滑前后的差值。若差值 > 10% 且集中在关键帧附近，才考虑方案 3；若只是高频抖动，方案 1 足够。
