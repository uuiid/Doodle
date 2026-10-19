// dsh-drop-caret client half: teach DSH's own drag-drop intake resolve a
// dropped file's real path, so a drop becomes an `@path` reference instead of
// an attachment upload (or an "unsupported type" rejection).
//
// DSH's `IntakeFiles` reads only `globalThis.__DSH_HOST_PATHS__.pathFor(file)`:
// a non-empty path becomes a reference chip; otherwise the file is uploaded as
// an attachment. That bridge exists for a desktop host, but `dsh web` -- and
// the official Electron shell 0.3.23 -- never install it, so `pathFor` is
// undefined and every drop falls back to the attachment channel.
//
// This plugin installs the missing bridge. It never intercepts, cancels, or
// stops propagation of any drag event: DSH's own listeners keep running, and
// the only thing added is a `pathFor` that can answer for a given File.
//
// Path sources, in order:
//   1. `text/uri-list` / `application/vnd.code.tree` carried by the drag,
//      matched to `dataTransfer.files` positionally and memoised by File
//      identity (that is the same object DSH later hands to `pathFor`).
//   2. `File.path`, which Electron-style hosts attach to dropped files.
//
// There is no UI half: the plugin registers no slot component at all.

window.__ModuleLoader__.load({ id: 'dsh-drop-caret', factory: () => {
  var module = { exports: {} }
  var exports = module.exports

  /** 「发送选中内容」产生的引用的来源名，序列化时按它找回原文。 */
  var VSCode_SOURCE = 'dsh-drop-caret'

  function basenameOf(path) {
    var i = Math.max(path.lastIndexOf('\\'), path.lastIndexOf('/'))
    return i === -1 ? path : path.slice(i + 1)
  }

  function toArray(list) {
    return Array.prototype.slice.call(list || [])
  }

  function normalizeWinPath(p) {
    if (/^[A-Za-z]:[\\/]/.test(p)) {
      return p.charAt(0).toUpperCase() + ':' + p.slice(2).replace(/\//g, '\\')
    }
    return p
  }

  function fileUriToPath(uri) {
    if (typeof uri !== 'string') return null
    var u = uri.trim()
    if (u.indexOf('file://') === 0) {
      var p
      try { p = decodeURIComponent(u.slice('file://'.length)) } catch (e) { p = u.slice('file://'.length) }
      if (/^\/[A-Za-z]:[\\/]/.test(p)) p = p.slice(1)
      return normalizeWinPath(p)
    }
    if (/^[A-Za-z]:[\\/]/.test(u)) return normalizeWinPath(u)
    if (u.charAt(0) === '/') return u
    return null
  }

  function parseUriList(data) {
    return String(data).split(/\r?\n/)
      .map(function (l) { return l.trim() })
      .filter(function (l) { return l !== '' && l.charAt(0) !== '#' })
  }

  function extractUrisFromTree(data) {
    var out = []
    try { walk(JSON.parse(data)) } catch (e) { /* not valid JSON */ }
    function walk(node) {
      if (node === null || typeof node !== 'object') return
      if (Array.isArray(node)) {
        for (var i = 0; i < node.length; i++) walk(node[i])
        return
      }
      var u = node.resource || node.resourceUri || node.uri
      if (typeof u === 'string') out.push(u)
      for (var k in node) {
        if (k === 'resource' || k === 'resourceUri' || k === 'uri') continue
        walk(node[k])
      }
    }
    return out
  }

  /** 一次拖拽携带的 URI（VS Code 资源管理器优先用 code.tree，它带完整选区）。 */
  function resolveUris(dataTransfer) {
    if (!dataTransfer) return []
    var pick = function (uris) {
      return uris.map(fileUriToPath).filter(function (p) {
        return typeof p === 'string' && p !== ''
      })
    }
    var tree = dataTransfer.getData('application/vnd.code.tree')
    if (tree) {
      var fromTree = pick(extractUrisFromTree(tree))
      if (fromTree.length > 0) return fromTree
    }
    var list = dataTransfer.getData('text/uri-list')
    if (!list) return []
    return pick(parseUriList(list))
  }

  /**
   * 给拖入的文件补上真实路径。
   *
   * 监听全部注册在 window 的冒泡阶段，且从不调用 preventDefault /
   * stopPropagation / stopImmediatePropagation——DSH 的 document 级
   * dragenter / dragover / drop 处理器照常运行，本插件只负责在它取路径之前
   * 把路径准备好。
   */
  function installHostPathBridge() {
    var byFile = new WeakMap()

    function bindFiles(dataTransfer, paths) {
      if (paths.length === 0) return
      var files = toArray(dataTransfer.files)
      for (var i = 0; i < files.length && i < paths.length; i++) {
        byFile.set(files[i], paths[i])
      }
    }

    // dragenter 时 Files 权限已可读，且早于内置 drop，先把映射建好。
    function onDragEnter(event) {
      bindFiles(event.dataTransfer, resolveUris(event.dataTransfer))
    }

    // drop 再补一次：某些来源只在 drop 阶段带全 URI 列表。
    function onDrop(event) {
      bindFiles(event.dataTransfer, resolveUris(event.dataTransfer))
    }

    function pathFor(file) {
      if (file === null || typeof file !== 'object') return ''
      var fromUri = byFile.get(file)
      if (typeof fromUri === 'string' && fromUri !== '') return fromUri
      if (typeof file.path === 'string' && file.path !== '') return file.path
      return ''
    }

    var bridge = { pathFor: pathFor }
    var previous = globalThis.__DSH_HOST_PATHS__
    if (previous === undefined) globalThis.__DSH_HOST_PATHS__ = bridge

    window.addEventListener('dragenter', onDragEnter)
    window.addEventListener('drop', onDrop)
    return function () {
      window.removeEventListener('dragenter', onDragEnter)
      window.removeEventListener('drop', onDrop)
      if (previous === undefined && globalThis.__DSH_HOST_PATHS__ === bridge) {
        delete globalThis.__DSH_HOST_PATHS__
      }
    }
  }

  /**
   * VS Code 扩展「发送选中内容到 DSH」：扩展把
   * `{ type: 'insert-selection', filePath, startLine, endLine, content, language }`
   * postMessage 进 iframe，需要落进对话框。
   *
   * 走浏览器原生插入：`ctx.conversation` 的 `slash/input-insert-reference`
   * 要求 span 是编辑器 detect 投影坐标，而 detect 投影（chip 只占 1 字符）
   * 与服务端可读的 clipboard 投影（chip 展开成路径）不同，外部算不出来，
   * 空 span 会被 `selectSpan` 判越界而静默拒绝。因此这里不改编辑器模型，
   * 直接在 composer 的 contenteditable 上按光标插入纯文本 `@路径:行号`。
   */
  function installSelectionIntake() {
    function editorRoot() {
      var host = document.querySelector('[data-composer-input]')
      var scope = host === null ? document : host
      var editable = scope.querySelector('[contenteditable="true"]')
      if (editable !== null) return editable
      return document.querySelector('[data-lexical-editor="true"]')
    }

    function caretRange(root) {
      var selection = window.getSelection()
      if (selection !== null && selection.rangeCount > 0) {
        var live = selection.getRangeAt(0)
        if (root.contains(live.startContainer)) {
          var range = live.cloneRange()
          range.collapse(false)
          return range
        }
      }
      var tail = document.createRange()
      tail.selectNodeContents(root)
      tail.collapse(false)
      return tail
    }

    function insertText(text) {
      var root = editorRoot()
      if (root === null) return false
      root.focus()
      var range = caretRange(root)
      var selection = window.getSelection()
      if (selection === null) return false
      selection.removeAllRanges()
      selection.addRange(range)
      var inserted = false
      try {
        inserted = document.execCommand('insertText', false, text)
      } catch (e) {
        inserted = false
      }
      if (inserted) return true
      range.deleteContents()
      range.insertNode(document.createTextNode(text))
      range.collapse(false)
      selection.removeAllRanges()
      selection.addRange(range)
      root.dispatchEvent(new InputEvent('input', { bubbles: true, inputType: 'insertText', data: text }))
      return true
    }

    function onMessage(event) {
      var data = event.data
      if (!data || data.type !== 'insert-selection') return
      if (typeof data.filePath !== 'string' || data.filePath === '') return
      var start = Number(data.startLine) || 1
      var end = Number(data.endLine) || start
      var range = end > start ? ':' + start + '-' + end : ':' + start
      insertText('@' + data.filePath + range + ' ')
    }

    window.addEventListener('message', onMessage)
    return function () {
      window.removeEventListener('message', onMessage)
    }
  }

  function apply(ctx) {
    ctx.effect(function () {
      return installHostPathBridge()
    })
    ctx.effect(function () {
      return installSelectionIntake()
    })
    ctx.effect(function () {
      return ctx.inputTriggers.registerSource({
        trigger: '@',
        name: VSCode_SOURCE,
        candidates: async function () { return [] },
        onPick: function () { return undefined },
        codec: {
          clipboardText: function (ref) { return ref },
          serialize: async function (ref) { return ref }
        }
      })
    })
  }

  exports.apply = apply
  exports.inject = ['inputTriggers']
  return module.exports
} })
