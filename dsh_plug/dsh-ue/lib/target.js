//#region 目标解析与命令行构造（纯函数，可被 test/pure.mjs 通过源码切片单测）

export const DEFAULT_PLATFORM = 'Win64';
export const DEFAULT_CONFIGURATION = 'Development';
export const CONFIGURATIONS = ['Debug', 'DebugGame', 'Development', 'Shipping', 'Test'];

export const EDITOR_EXES = { cmd: 'UnrealEditor-Cmd.exe', editor: 'UnrealEditor.exe' };
export const EDITOR_EXE_KINDS = ['cmd', 'editor'];
export const DEFAULT_EDITOR_EXE = 'cmd';
export const EDITOR_BINARIES_RE = /Engine[\\/]Binaries[\\/]Win64$/i;

const RE_PLATFORM = /^[A-Za-z][A-Za-z0-9_]{0,40}$/;
const RE_TARGET = /^[A-Za-z][A-Za-z0-9_]{0,120}$/;
const RE_CONFIGURATION = /^[A-Za-z][A-Za-z]{0,40}$/;
const RE_FORWARDABLE = /^-{1,2}[A-Za-z][A-Za-z0-9_=:.,/-]*$/;
// 常规前缀（会让 UBT 丢掉「按项目解析 target」的默认行为）、以及 -Mode 取值选项。
const RE_FORBIDDEN_PREFIX = /^[-/](project|target|platform|configuration|mode)(=|:|$)/i;
const RE_SAFE_RELATIVE = /^[^<>:"|?*\u0000]+$/;

/**
 * @param {unknown} value
 * @returns {boolean}
 */
export function isAbsolutePath(value) {
  if (typeof value !== 'string' || value.length === 0) return false;
  if (/^[A-Za-z]:[\\/]/.test(value)) return true;
  if (value.startsWith('\\\\') || value.startsWith('//')) return true;
  return false;
}

/**
 * 相对 repoRoot 的安全相对路径：不许逃逸、不许盘符、不许 UNC。
 *
 * @param {unknown} value
 * @returns {boolean}
 */
export function isSafeRelativePath(value) {
  if (typeof value !== 'string' || value.length === 0 || value.length > 400) return false;
  if (!RE_SAFE_RELATIVE.test(value)) return false;
  if (isAbsolutePath(value)) return false;
  for (const part of value.split(/[\\/]+/)) {
    if (part === '..') return false;
  }
  return true;
}

/**
 * .uproject 路径解析：绝对路径原样使用，相对路径相对 repoRoot 拼接。
 *
 * @param {unknown} value
 * @param {unknown} repoRoot
 * @returns {string}
 */
export function resolveProjectPath(value, repoRoot) {
  const text = String(value ?? '').trim();
  if (text.length === 0) return '';
  const normalized = text.replace(/\//g, '\\');
  if (isAbsolutePath(normalized)) return normalized;
  const root = stripTrailingSeparators(repoRoot);
  return root.length > 0 ? `${root}\\${normalized}` : normalized;
}

/**
 * 平台名：UBT 的 platform 是标识符形状（Win64 / Android_ASTC / HTML5）。
 *
 * @param {unknown} value
 * @returns {boolean}
 */
export function isSafePlatform(value) {
  return typeof value === 'string' && RE_PLATFORM.test(value);
}

/**
 * Target 名：UE 的 target 必须是合法标识符，且脚本里建议全大写（TargetName vs TargetName_CamelCase 也是合法的）。
 *
 * @param {unknown} value
 * @returns {boolean}
 */
export function isSafeTargetName(value) {
  return typeof value === 'string' && RE_TARGET.test(value);
}

/**
 * Configuration 名：Debug / Development / Shipping 之类，只允许字母。
 *
 * @param {unknown} value
 * @returns {boolean}
 */
export function isSafeConfiguration(value) {
  return typeof value === 'string' && RE_CONFIGURATION.test(value);
}

/**
 * 允许透传给 UBT 的额外参数形状：必须像开关（`-WaitMutex` / `--cook` / `-Foo=bar`），
 * 且不能覆盖我们用位置参数决定的 target / platform / configuration / mode。
 *
 * @param {unknown} value
 * @returns {boolean}
 */
export function isForwardableUbtArg(value) {
  if (typeof value !== 'string' || value.length === 0 || value.length > 300) return false;
  if (!RE_FORWARDABLE.test(value)) return false;
  if (RE_FORBIDDEN_PREFIX.test(value)) return false;
  if (/[\r\n]/.test(value)) return false;
  return true;
}

/**
 * 校验 target 名与 configuration 一起给出的场景（进一步可选）。
 *
 * @param {{target?:unknown, platform?:unknown, configuration?:unknown, extraArgs?:unknown}} input
 * @returns {string[]} 问题列表，空数组表示通过
 */
export function validateBuildArgs(input) {
  /** @type {string[]} */
  const problems = [];
  if (input.target !== undefined && !isSafeTargetName(input.target)) {
    problems.push(`target 非法：只允许字母、数字、下划线，且必须以字母开头。收到：${JSON.stringify(input.target)}`);
  }
  if (input.platform !== undefined && !isSafePlatform(input.platform)) {
    problems.push(`platform 非法：只允许字母、数字、下划线，且必须以字母开头。收到：${JSON.stringify(input.platform)}`);
  }
  if (input.configuration !== undefined && !isSafeConfiguration(input.configuration)) {
    problems.push(`configuration 非法：只允许字母，例如 Development / Debug / Shipping。收到：${JSON.stringify(input.configuration)}`);
  }
  if (input.extraArgs !== undefined) {
    if (!Array.isArray(input.extraArgs)) {
      problems.push('extraArgs 必须是字符串数组。');
    } else {
      for (const arg of input.extraArgs) {
        if (!isForwardableUbtArg(arg)) {
          problems.push(
            `extraArgs 含非法参数：${JSON.stringify(arg)}。只接受 -Foo / -Foo=bar 形状，` +
            '且不接受 -project / -target / -platform / -configuration / -mode。',
          );
        }
      }
    }
  }
  return problems;
}

/**
 * 规则解析的上下文（由插件读取磁盘后传入）。
 *
 * @typedef {object} ResolveContext
 * @property {string} repoRoot                      仓库根（Doodle 仓库）
 * @property {string} defaultProjectFileRel         repoRoot 下的默认 .uproject 相对路径（可为空字符串）
 * @property {string} engineRoot                    引擎根，例如 E:\UnrealEngine
 * @property {string} targetProjectFileRel          引擎目标（UnrealEditor）时使用的 .uproject 相对路径
 * @property {string} pluginBuildScriptRel          repoRoot 下的插件工程脚本（如 script/uePlug/BuildDooodlePlug.bat）
 * @property {string} [defaultPlatform]             默认平台（缺省 Win64）
 * @property {string} [defaultConfiguration]        默认配置（缺省 Development）
 * @property {string} [defaultTarget]               默认 target 名；为空时回退到 `<工程名>Editor`
 */

/**
 * @typedef {object} ResolveInput
 * @property {string} [target]
 * @property {boolean} [engine]
 * @property {string} [projectFile]     相对 repoRoot 或绝对路径的 .uproject
 * @property {string} [projectAbsolute] 绝对路径 .uproject（scope=absolute）
 * @property {'repo'|'absolute'} [scope]
 * @property {string} [platform]
 * @property {string} [configuration]
 * @property {string[]} [extraArgs]
 */

/**
 * @typedef {object} ResolvedTarget
 * @property {'ubt'|'script'} mode
 * @property {string} target
 * @property {string} platform
 * @property {string} configuration
 * @property {string} projectFile            生效的 .uproject（绝对路径；script 模式可能为空）
 * @property {boolean} engineTarget
 * @property {string} cwd
 * @property {string[]} ubtArgs
 * @property {string} explanation
 */

/**
 * 把模型给出的松散参数解析成一次确定的 UE 构建：
 * 决定走 Build.bat（UBT）还是仓库里的工程脚本，并算出 cwd、argv 与 UBT 参数。
 *
 * 规则（按顺序）：
 *   1. scope=absolute 且给了 projectAbsolute → 用该 .uproject，cwd 落在它所在目录。
 *   2. engine=true，或 target 命中 UnrealEditor / Unreal（引擎编辑器）。
 *   3. 给了 projectFile（相对 repoRoot 或绝对路径）→ 用该 .uproject。
 *   4. 仓库配置了 pluginBuildScript → 走该脚本（UE 插件工程的编译入口）。
 *   5. 否则用 defaultProjectFileRel。
 *
 * @param {ResolveInput} input
 * @param {ResolveContext} context
 * @param {(absolutePath:string)=>boolean} exists 注入式存在性检查
 * @returns {{ok:true, resolved:ResolvedTarget} | {ok:false, problems:string[]}}
 */
export function resolveTarget(input, context, exists) {
  /** @type {string[]} */
  const problems = validateBuildArgs(input);
  const defaultPlatform = typeof context.defaultPlatform === 'string' && context.defaultPlatform.length > 0
    ? context.defaultPlatform
    : DEFAULT_PLATFORM;
  const defaultConfiguration = typeof context.defaultConfiguration === 'string' && context.defaultConfiguration.length > 0
    ? context.defaultConfiguration
    : DEFAULT_CONFIGURATION;
  const platform = typeof input.platform === 'string' ? input.platform : defaultPlatform;
  const configuration = typeof input.configuration === 'string' ? input.configuration : defaultConfiguration;
  const extraArgs = Array.isArray(input.extraArgs) ? input.extraArgs.slice() : [];
  const requestedTarget = typeof input.target === 'string' ? input.target : undefined;

  if (input.scope !== undefined && input.scope !== 'repo' && input.scope !== 'absolute') {
    problems.push(`scope 只能是 "repo" 或 "absolute"。收到：${JSON.stringify(input.scope)}`);
  }
  if (problems.length > 0) return { ok: false, problems };

  const repoRoot = stripTrailingSeparators(context.repoRoot);
  const engineRoot = stripTrailingSeparators(context.engineRoot ?? 'E:\\UnrealEngine');
  const engineBuildBat = `${engineRoot}\\Engine\\Build\\BatchFiles\\Build.bat`;
  const defaultProject = typeof context.defaultProjectFileRel === 'string' ? context.defaultProjectFileRel : '';
  const engineProject = typeof context.targetProjectFileRel === 'string' && context.targetProjectFileRel.length > 0
    ? context.targetProjectFileRel
    : defaultProject;
  const configuredTarget = typeof context.defaultTarget === 'string' && context.defaultTarget.length > 0
    ? context.defaultTarget
    : undefined;

  /**
   * target 名的取值顺序：显式 target → config.defaultTarget → `<工程名>Editor`。
   * 有些工程只有一个 `FooTarget` target 类（没有 `FooEditorTarget`），
   * UBT 的 target 名就是 `Foo`，此时必须靠 config.defaultTarget 或显式 target 指定。
   *
   * @param {string} projectAbsolute
   * @returns {string}
   */
  const pickTarget = (projectAbsolute) => requestedTarget ?? configuredTarget ?? `${moduleTargetName(projectAbsolute)}Editor`;

  /** @param {string} rel @returns {string} */
  const repoPath = (rel) => `${repoRoot}\\${rel.replace(/\//g, '\\')}`;
  /** @param {string} project @returns {string} */
  const projectPath = (project) => resolveProjectPath(project, repoRoot);

  if (!exists(engineBuildBat)) {
    return {
      ok: false,
      problems: [`引擎构建脚本不存在：${engineBuildBat}。检查插件 config 的 engineRoot。`],
    };
  }

  const wantsEngine = input.engine === true
    || (requestedTarget !== undefined && /^(UnrealEditor|Unreal)$/i.test(requestedTarget));

  /** 走 Build.bat 的共同出口 */
  const ubtMode = (projectAbsolute, targetName, explanation) => {
    /** @type {string[]} */
    const ubtArgs = [targetName, platform, configuration];
    if (projectAbsolute.length > 0) ubtArgs.push(`-Project=${projectAbsolute}`);
    ubtArgs.push(...extraArgs);
    return {
      ok: /** @type {const} */ (true),
      resolved: {
        mode: /** @type {const} */ ('ubt'),
        target: targetName,
        platform,
        configuration,
        projectFile: projectAbsolute,
        engineTarget: wantsEngine,
        cwd: projectAbsolute.length > 0 ? dirOf(projectAbsolute) : engineRoot,
        ubtArgs,
        explanation,
      },
    };
  };

  if (input.scope === 'absolute') {
    const absolute = typeof input.projectAbsolute === 'string' ? input.projectAbsolute.trim() : '';
    if (absolute.length === 0) {
      return { ok: false, problems: ['scope=absolute 时必须给 projectAbsolute（.uproject 的绝对路径）。'] };
    }
    if (!isAbsolutePath(absolute)) {
      return { ok: false, problems: [`projectAbsolute 必须是绝对路径。收到：${JSON.stringify(absolute)}`] };
    }
    if (!exists(absolute)) {
      return { ok: false, problems: [`projectAbsolute 指向的 .uproject 不存在：${absolute}`] };
    }
    const targetName = pickTarget(absolute);
    return ubtMode(absolute, targetName, `absolute 工程 ${absolute}`);
  }

  if (wantsEngine) {
    if (engineProject.length === 0) {
      return {
        ok: false,
        problems: ['engine=true（编译 UnrealEditor）需要插件 config 里的 projectFile 指向一个 .uproject。'],
      };
    }
    const projectAbsolute = projectPath(engineProject);
    if (!exists(projectAbsolute)) {
      return { ok: false, problems: [`engine 模式使用的 .uproject 不存在：${projectAbsolute}`] };
    }
    const targetName = requestedTarget ?? 'UnrealEditor';
    return ubtMode(projectAbsolute, targetName, `引擎目标 ${targetName}（工程 ${projectAbsolute}）`);
  }

  if (input.projectFile !== undefined) {
    const given = typeof input.projectFile === 'string' ? input.projectFile.trim() : input.projectFile;
    if (!isAbsolutePath(given) && !isSafeRelativePath(given)) {
      return { ok: false, problems: [`projectFile 必须是 repoRoot 内的相对路径或不含 .. 的绝对路径。收到：${JSON.stringify(input.projectFile)}`] };
    }
    if (!/\.uproject$/i.test(given)) {
      return { ok: false, problems: [`projectFile 必须是 .uproject 文件。收到：${JSON.stringify(input.projectFile)}`] };
    }
    const projectAbsolute = projectPath(given);
    if (!exists(projectAbsolute)) {
      return { ok: false, problems: [`projectFile 指向的 .uproject 不存在：${projectAbsolute}`] };
    }
    const targetName = pickTarget(projectAbsolute);
    return ubtMode(projectAbsolute, targetName, `工程 ${projectAbsolute}`);
  }

  const scriptRel = typeof context.pluginBuildScriptRel === 'string' ? context.pluginBuildScriptRel : '';
  if (scriptRel.length > 0) {
    const scriptAbsolute = repoPath(scriptRel);
    if (!exists(scriptAbsolute)) {
      return { ok: false, problems: [`插件工程脚本不存在：${scriptAbsolute}。检查插件 config 的 pluginBuildScript，或改成传 projectFile。`] };
    }
    return {
      ok: true,
      resolved: {
        mode: 'script',
        target: scriptAbsolute,
        platform,
        configuration,
        projectFile: '',
        engineTarget: false,
        cwd: repoRoot,
        ubtArgs: [],
        explanation: `插件工程脚本 ${scriptAbsolute}（pluginBuildScript）`,
      },
    };
  }

  if (defaultProject.length === 0) {
    return {
      ok: false,
      problems: ['没有可编译的目标：传 projectFile（相对 repoRoot 或绝对路径的 .uproject）、scope=absolute + projectAbsolute、target=UnrealEditor，'
        + '或在插件 config 里配置 projectFile / pluginBuildScript。'],
    };
  }

  const projectAbsolute = projectPath(defaultProject);
  if (!exists(projectAbsolute)) {
    return { ok: false, problems: [`默认工程不存在：${projectAbsolute}`] };
  }
  const targetName = pickTarget(projectAbsolute);
  return ubtMode(projectAbsolute, targetName, '默认工程 ' + projectAbsolute);
}

/**
 * @param {string} value
 * @returns {string}
 */
function stripTrailingSeparators(value) {
  return String(value ?? '').replace(/[\\/]+$/, '');
}

/**
 * 从 .uproject 路径推默认 target 名：MyGame.uproject → MyGameEditor。
 *
 * @param {string} projectAbsolute
 * @returns {string}
 */
export function moduleTargetName(projectAbsolute) {
  const base = String(projectAbsolute).replace(/[\\/]+$/, '').split(/[\\/]/).pop() ?? '';
  const name = base.replace(/\.uproject$/i, '');
  return name.length > 0 ? name : 'UnrealEditor';
}

/**
 * 组装最终 argv：`cmd.exe /d /s /c <bat> <args...>`，每个参数是独立的 argv 项。
 *
 * 不能把整条命令拼成一个带引号的字符串交给 `/c`：宿主用 CommandLineToArgvW 规则
 * 给 argv 项转义，会把内部的 `"` 变成 `\"`，而 cmd 的 `/s /c` 不认 `\"`，
 * 于是程序名里带上字面反斜杠、cmd 报「找不到程序」。
 *
 * @param {{comspec:string, buildBat:string, args:string[]}} spec
 * @returns {string[]}
 */
export function buildLaunchArgv(spec) {
  return [spec.comspec, '/d', '/s', '/c', spec.buildBat, ...spec.args];
}

/**
 * 把 argv 渲染成人类可复制的一行命令。
 *
 * @param {string[]} argv
 * @returns {string}
 */
export function renderCommandLine(argv) {
  return argv
    .map((part) => (/[\s"]/.test(part) ? `"${part.replace(/"/g, '\\"')}"` : part))
    .join(' ');
}

/**
 * 编辑器可直接复制的完整命令行：exe + .uproject + 其余参数。
 *
 * @param {{exePath:string, projectFile:string, args?:string[]}} spec
 * @returns {string}
 */
export function renderEditorCommandLine(spec) {
  return renderCommandLine(buildEditorLaunchArgv(spec));
}

/**
 * 编辑器二进制档位：`cmd`（UnrealEditor-Cmd.exe）或 `editor`（UnrealEditor.exe）。
 *
 * @param {unknown} value 省略时取默认档位
 * @returns {'cmd'|'editor'}
 */
export function editorExeName(value) {
  const text = String(value ?? '').trim().toLowerCase();
  return text === 'editor' ? 'editor' : 'cmd';
}

/**
 * 编辑器可执行文件的绝对路径：`<engineRoot>\Engine\Binaries\Win64\UnrealEditor[-Cmd].exe`。
 *
 * @param {unknown} kind
 * @param {unknown} engineRoot 允许直接给出 `...\Engine\Binaries\Win64` 这一层目录
 * @returns {string}
 */
export function editorExePath(kind, engineRoot) {
  const root = stripTrailingSeparators(engineRoot);
  const binaries = EDITOR_BINARIES_RE.test(root) ? root : `${root}\\Engine\\Binaries\\Win64`;
  return `${binaries}\\${EDITOR_EXES[editorExeName(kind)]}`;
}

/**
 * 启动编辑器的 argv：exe 在前，`.uproject` 作为第一个参数，其余参数按给出的顺序原样追加。
 *
 * @param {{exePath:string, projectFile:string, args?:string[]}} spec
 * @returns {string[]}
 */
export function buildEditorLaunchArgv(spec) {
  return [spec.exePath, spec.projectFile, ...(Array.isArray(spec.args) ? spec.args : [])];
}

/**
 * 编辑器参数只拒绝会让解析产生歧义的值，其余（裸词、带引号的 UE 命令、含空格的路径）原样透传。
 *
 * @param {unknown} value
 * @returns {boolean}
 */
export function isForwardableEditorArg(value) {
  if (typeof value !== 'string' || value.length === 0 || value.length > 2048) return false;
  return !/[\r\n\u0000]/.test(value);
}

/**
 * 从干净环境启动时会被拒绝的编辑器参数：另起一个 `.uproject` 或改写日志目标都会与启动参数冲突。
 *
 * @param {unknown} value
 * @returns {string|undefined} 拒绝理由；可透传时返回 undefined
 */
export function unsafeEditorArg(value) {
  const text = String(value ?? '').trim();
  if (/^-{1,2}(abs)?project(=|:)/i.test(text)) {
    return '不接受改写工程的 -project / -absproject：工程已由插件配置固定为第一个参数';
  }
  if (/^-{1,2}(abs)?log(=|:)/i.test(text)) {
    return '不接受改写日志目标的 -abslog：编辑器会按自己的日志规则写入工程的 Saved/Logs';
  }
  return undefined;
}

/**
 * 校验 ue_editor 的参数形状（不涉及磁盘）。
 *
 * @param {{exe?:unknown, extraArgs?:unknown, timeoutMs?:unknown}} input
 * @returns {string[]} 问题列表，空数组表示通过
 */
export function validateEditorArgs(input) {
  /** @type {string[]} */
  const problems = [];
  if (input.exe !== undefined && !EDITOR_EXE_KINDS.includes(input.exe)) {
    problems.push(`exe 只能是 ${EDITOR_EXE_KINDS.map((kind) => JSON.stringify(kind)).join(' 或 ')}。收到：${JSON.stringify(input.exe)}`);
  }
  if (input.extraArgs !== undefined) {
    if (!Array.isArray(input.extraArgs)) {
      problems.push('extraArgs 必须是字符串数组（每项一个编辑器参数）。');
    } else {
      for (const arg of input.extraArgs) {
        if (!isForwardableEditorArg(arg)) {
          problems.push(`extraArgs 含无法透传的参数：${JSON.stringify(arg)}。参数必须非空、不超过 2048 字符，且不含换行或 \\0。`);
          continue;
        }
        const reason = unsafeEditorArg(arg);
        if (reason !== undefined) problems.push(`${reason}：${JSON.stringify(arg)}`);
      }
    }
  }
  if (input.timeoutMs !== undefined
    && !(typeof input.timeoutMs === 'number' && Number.isInteger(input.timeoutMs) && input.timeoutMs > 0)) {
    problems.push(`timeoutMs 必须是正整数毫秒数。收到：${JSON.stringify(input.timeoutMs)}`);
  }
  return problems;
}

/**
 * @typedef {object} EditorResolveContext
 * @property {string} repoRoot                 仓库根（Doodle 仓库）
 * @property {string} engineRoot               引擎根，例如 E:\\UnrealEngine
 * @property {string} editorProjectFileRel     编辑器启动时使用的 .uproject（相对 repoRoot 或绝对路径）
 */

/**
 * @typedef {object} ResolvedEditor
 * @property {'editor'} mode
 * @property {'cmd'|'editor'} exeKind
 * @property {string} exeName
 * @property {string} exePath
 * @property {string} projectFile              固定为第一个参数的 .uproject（绝对路径）
 * @property {string} cwd
 * @property {string[]} launchArgs             [exe, projectFile, ...extraArgs]
 * @property {string[]} editorArgs             [projectFile, ...extraArgs]
 * @property {string} explanation
 */

/**
 * 把 ue_editor 的松散参数解析成一次确定的编辑器启动。
 *
 * @param {{exe?:unknown, extraArgs?:unknown, timeoutMs?:unknown}} input
 * @param {EditorResolveContext} context
 * @param {(absolutePath:string)=>boolean} exists 注入式存在性检查
 * @returns {{ok:true, resolved:ResolvedEditor} | {ok:false, problems:string[]}}
 */
export function resolveEditor(input, context, exists) {
  /** @type {string[]} */
  const problems = validateEditorArgs(input);
  if (problems.length > 0) return { ok: false, problems };

  const repoRoot = stripTrailingSeparators(context.repoRoot);
  const engineRoot = stripTrailingSeparators(context.engineRoot ?? 'E:\\UnrealEngine');
  const exeKind = editorExeName(input.exe);
  const exePath = editorExePath(exeKind, engineRoot);
  const projectFile = resolveProjectPath(context.editorProjectFileRel, repoRoot);
  const editorArgs = Array.isArray(input.extraArgs) ? input.extraArgs.slice() : [];

  if (projectFile.length === 0) {
    return {
      ok: false,
      problems: ['没有可启动的工程：插件 config 的 projectFile / editorProjectFile 必须指向一个 .uproject。'],
    };
  }
  if (!exists(exePath)) {
    return {
      ok: false,
      problems: [`编辑器不存在：${exePath}。检查插件 config 的 engineRoot，或确认编辑器目标已经编译通过。`],
    };
  }
  if (!exists(projectFile)) {
    return { ok: false, problems: [`工程文件不存在：${projectFile}。检查插件 config 的 projectFile / editorProjectFile。`] };
  }

  return {
    ok: true,
    resolved: {
      mode: 'editor',
      exeKind,
      exeName: EDITOR_EXES[editorExeName(exeKind)],
      exePath,
      projectFile,
      cwd: dirOf(projectFile),
      launchArgs: buildEditorLaunchArgv({ exePath, projectFile, args: editorArgs }),
      editorArgs: [projectFile, ...editorArgs],
      explanation: `编辑器 ${EDITOR_EXES[editorExeName(exeKind)]}（工程 ${projectFile}）`,
    },
  };
}

/**
 * 取出路径的所在目录（同时接受 `\` 与 `/`）。
 *
 * @param {string} absolute
 * @returns {string}
 */
export function dirOf(absolute) {
  const text = String(absolute ?? '');
  const index = Math.max(text.lastIndexOf('\\'), text.lastIndexOf('/'));
  return index > 0 ? text.slice(0, index) : text;
}

/**
 * 从 .uproject 内容里读出 EngineAssociation（用于报告冲突，不做语义判断）。
 *
 * @param {string} text
 * @returns {string|undefined}
 */
export function readEngineAssociation(text) {
  if (typeof text !== 'string' || text.length === 0) return undefined;
  try {
    const parsed = JSON.parse(text);
    const value = parsed?.EngineAssociation;
    return typeof value === 'string' && value.length > 0 ? value : undefined;
  } catch {
    return undefined;
  }
}

//#endregion
