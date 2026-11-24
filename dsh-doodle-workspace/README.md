# dsh-doodle-workspace

Doodle 的两个 DSH 辅助插件（`dsh-cmake`、`dsh-kitsu-supplement`）共用的**工作区解析层**。

它不是 DSH 插件：没有 `apply()`、不注册工具、不读 `ctx`，只是被两个插件 `import` 的一组纯函数。

## 为什么存在

两个插件都要回答同两个问题，而且答案必须一致：

1. **发起这次调用的会话，工作区根目录是哪一个？**
2. **这个根目录对应的 git 分支与构建目录怎么算？**

构建目录的公式写在 [CMakePresets.json](../CMakePresets.json) 里：

```text
${sourceParentDir}/build/$env{DOODLE_GIT_BRANCH}/${presetName}
```

分支名来自 `build_script/build/set_env.ps1` 的 `git rev-parse --abbrev-ref HEAD`。
`dsh-kitsu-supplement` 要从这个目录里找 `bin\doodle_kitsu_supplement_test.exe`，
`dsh-cmake` 要把构建落在这里——两边各自实现一次就一定会漂移，所以收敛到本包。

## 导出

| 分组 | 函数 |
| --- | --- |
| 路径 | `normalizeRoot` / `parentOf` / `joinWindowsPath` / `splitPathParts` / `toForwardPath` / `toWindowsPath` / `isAbsolutePath` / `isSeparatorCode` / `trimText` |
| 工作区 | `chooseWorkspaceRoot` / `sessionCwdOf` |
| 分支与构建目录 | `parseGitBranch` / `buildDirOf` / `presetBinaryDirExpression` / `expandBinaryDirExpression` / `resolveBuildDir` / `parseJsonObject` |
| 可执行文件 | `resolveExeName` / `judgeExeResolution` / `exePathOf` |
| playwright | `choosePlaywrightCwd` / `playwrightCwdFailure` |
| CMake 缓存 | `cacheHomeDirectory` / `buildDirOwnership` |
| 常量 | `DEFAULT_TEST_EXE_NAME` / `DEFAULT_PRODUCTION_EXE_NAME` / `EXE_NAMES` / `KITSU_PRESET_DIR` / `EXE_STOP_NOTICE` / `WORKSPACE_STOP_NOTICE` |

## 工作区根目录的解析顺序

1. `ctx.get("workingDirectory").get(agent)` —— 会话**当前**目录（随 workingDirectory 工具切换）；
2. 会话创建时记录的项目根 `session.header.cwd`；
3. 部署兜底目录 `workingDirectory.defaultDirectory`。

都拿不到就返回空串，由调用方报错并停下，**绝不**回落到写死的仓库路径。

## 两条实现约定

1. **只用字符码判断分隔符（92 / 47）**，不用反斜杠字面量与正则转义。`dsh-kitsu-supplement` 的
   `test/pure.mjs` 会用 `node:vm` 按源码文本求值，那种求值方式下字符串里的 `\d` 会退化成 `d`。
2. **内部一律用 `/` 拼接，出口才折回 `\`**。即使某层被求值方式削掉了反斜杠，也不会把路径拼成一坨。

## 自测

```text
node test/pure.mjs
```

直接 `import` 本包的 `lib/workspace.js`，不依赖任何第三方模块。
