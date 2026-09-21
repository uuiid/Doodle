# 面部捕捉(MetaHuman)任务看板

> 状态：**暂停中**（2026-09-21 暂停，等待继续）
> 关联归档文档：`011b370a`（实证与可复现配置）、`12c178eb`（源码分层调查）、`fe22fd6f`（UE 5.8 官方无标记捕捉的影响）、`60fe74ee`（方案设计）、`9433c0cd`（MeFaMo 反面参照）

## 路线

自研 C++ + 深度摄像头 → **blendshape 权重**（精度优先、离线）。不走 Live Link Face（已弃用），不打开 Live Link Hub，求解结果直接送 Maya。

Maya 侧已基本就绪：`dna_calib_import.cpp` 已能构建完整 DNA rig（joints + blendShape）并连 `output_blendshape_weights`，`dna_calib_node.cpp` 提供 `gui_control_list → RigLogic → joints + blendshape`。缺的只有求解器（视频 → GUI 控制值）。

注意：在 DoodleLiveLink/DNA 架构里 **blendshape 权重是 RigLogic 的输出而非输入**，真正的未知量是 `gui_control_list`。

## 已完成

- [x] 调研 MetaHuman Animator 源码分层，确认算法核心在 `MetaHumanCoreTechLib`（约 465 个源文件），**不在**两个名字叫 Solver 的模块里（`MetaHumanFaceFittingSolver` / `MetaHumanFaceAnimationSolver` 各只有 6 个文件，是包装层）
- [x] 实证 `MetaHumanCoreTechLib` 可在非编辑器 `TargetType.Program` 下**编译并链接成功**

    证据：739 个动作 0 错误 → 208MB 单体 exe；UHT manifest 115 个模块含 `MetaHumanCoreTechLib`，**不含 `UnrealEd`**，Type=Editor 的模块数为 0。

## 关键技术事实（避免重复踩坑）

1. **`Runtime` 类型的模块对 `TargetType.Program` 不兼容**。UBT 规则（`ModuleDescriptor.cs` ~808 行）：
   - `Runtime` → `TargetType != Program`
   - `RuntimeAndProgram` → `true`（唯一无条件兼容）
   - `Editor` → 仅 Editor
   所以改 `Editor → Runtime` **不够**，必须改 `RuntimeAndProgram`（它对 Editor 也返回 true，不会破坏现有 UnrealEditor 构建）。

2. **需改 4 个 uplugin / 6 处声明**（依赖链逐级放行）：
   `MetaHumanCoreTechLib`、`MetaHumanCoreTech`、`MetaHumanSDKRuntime`、`RigLogicLib`、`RigLogicModule`、`CaptureDataCore`。
   `Eigen` / `simde` 是 `ModuleType.External`，无需改动。

3. **探针模板抄 `Engine/Source/Programs/ReplicationSystemTest/`**，不要从 `BlankProgram` 改（后者 `bCompileAgainstEngine = false`，不含 Engine 引导）。
   四个已踩的坑：`IAutomationControllerModule.h` 缺失（需把 `AutomationController` 作为依赖）、`AutomationTestPlatform.generated.h` 不存在（**绝不能**把 `AutomationTest` 加进 include path）、`bBuildDeveloperTools = true` 会拉入在 `bBuildWithEditorOnlyData = false` 下编不过的 Developer 模块（Android/FileUtilities）、属性名是 `bCompileWithPluginSupport` 而非 `bBuildWithPluginSupport`。

4. **库的 API 面本身平台无关**：`MetaHumanCreatorAPI.h` 只暴露 `dna::Reader*` + `Eigen::Matrix<float,3,-1>` + `std::string` + `const char*`，不含任何 UE 类型。
   ```cpp
   static std::shared_ptr<MetaHumanCreatorAPI> CreateMHCApi(
       dna::Reader* InDnaReader, const char* InMhcDataPath,
       int numThreads = -1, dna::Reader* InBodyDnaReader = nullptr);
   ```
   `dna::Reader` 正是 vcpkg `dnacalib` 的那一套，两边天然对接。

## 待办

- [ ] **把探针从"能链接"推进到"能用"**：`MHTechProbe` 目前止于 `GEngineLoop.PreInit` 的 `ShaderCodeLibrary` 缺失（`missing from ../../../Engine/Programs/MHTechProbe/Content/`），加 `-nullrhi` 后在异步加载阶段崩溃。
      这是**裸 UE Program 缺 Content/Config 的通用问题**，与本库无关。
      正解：改为轻量初始化（走 `IMetaHumanCoreTechLib`，不进完整 engine loop），直接调 `CreateMHCApi` 跑通一次 `State → Evaluate`。
- [ ] 决定 `MetaHumanPipeline`（Editor 模块）是否必需 —— 当前探针未依赖它
- [ ] 确认 UE EULA：从源码构建中抽取模块、作为独立库再分发是否可行（内部工具风险低，产品分发需法务确认）
- [ ] 未解决的设计问题：求解空间选 `gui_control_list` 而非 raw blendshape 权重（后者绕过 PSD、破坏 rig 意图）；眼球注视需 iris 追踪（blendshape 做不出来）；jaw/tongue 是独立 mesh + joint

## 环境与产物

- UE 源码树：`E:\UnrealEngine`（UE 5.7.4，已完整编译）
- 探针：`E:\UnrealEngine\Engine\Source\Programs\MHTechProbe\`（`MHTechProbe.Target.cs`、`MHTechProbe.Build.cs`、`Private\MHTechProbe.cpp`，共约 2KB）
- 构建：`E:\UnrealEngine\Engine\Build\BatchFiles\Build.bat MHTechProbe Win64 Development -NoHotReload`
- 产物：`E:\UnrealEngine\Engine\Binaries\Win64\MHTechProbe.exe`（208MB）
- uplugin 改动均已备份为 `*.uplugin.bak`
