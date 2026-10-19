// dsh-drop-caret client half: a document-level file drop handler that inserts
// each dropped file reference into the composer instead of appending to it.
//
// Two drop sources (URI 来源优先——拖入即引用源文件本身，不产生副本):
//   1. VS Code Explorer — `text/uri-list` / `application/vnd.code.tree` →
//      parse the local file:// URI to a path, insert it directly (源文件路径).
//   2. OS file manager — only `Files` in dataTransfer；浏览器拿不到真实路径时
//      退化为上传落盘到 `.dsh-drop/<sessionId>/` 并插入该副本路径。
// NOTE: VS Code Explorer 拖拽同时携带 Files 与 text/uri-list，故类型判定必须
// 先判 URI 类型，否则会被误判为 OS 拖入而去落盘副本。
//
// The composer is a Lexical contenteditable div (`[data-composer-input]`), not a
// textarea: it has no selectionStart (so the old caret read was always -1) and
// firing `slash/input-insert-reference` with `emit` never reached the shell's
// bail listener. The drop point is now translated into the editor's detect
// coordinates and applied through `bail`, re-reading draftRev right before each
// insert so an upload round-trip cannot invalidate the span.
//
// There is no UI half: the plugin registers no slot component at all.

window.__ModuleLoader__.load({ id: 'dsh-drop-caret', factory: () => {
  var module = { exports: {} }
  var exports = module.exports

  var SOURCE_NAME = 'dsh-drop-caret'

  /** composer 的根选择器（Lexical 渲染出的 contenteditable div）。 */
  var COMPOSER = '[data-composer-input]'

  function basenameOf(path) {
    var i = Math.max(path.lastIndexOf('\\'), path.lastIndexOf('/'))
    return i === -1 ? path : path.slice(i + 1)
  }

  function composerRoot() {
    return document.querySelector(COMPOSER)
  }

  function toArray(list) {
    return Array.prototype.slice.call(list || [])
  }

  /** 在 detect 投影里占一个 U+FFFC 的节点：引用/命令 chip、换行前缀、`<br>`。 */
  function isAtomic(node) {
    return node.nodeType === 1 && (node.hasAttribute('data-lexical-decorator') || node.tagName === 'BR')
  }

  /** 根之下每个 block（paragraph）之间都贡献一个换行断点。 */
  function isBlock(node) {
    return node.nodeType === 1 && node.parentNode !== null && node.parentNode.hasAttribute('data-composer-input')
  }

  /** 根内文档的 detect 长度（与 composer 的 clipboard 投影一一对齐）。 */
  function detectLength(root) {
    var length = 0
    var blocks = 0
    var walker = document.createTreeWalker(root, NodeFilter.SHOW_TEXT | NodeFilter.SHOW_ELEMENT)
    for (var current = walker.nextNode(); current !== null; current = walker.nextNode()) {
      if (current.nodeType === 3) {
        length += current.length
        continue
      }
      if (isAtomic(current)) {
        length += 1
        continue
      }
      if (isBlock(current)) blocks += 1
    }
    return length + Math.max(0, blocks - 1)
  }

  /** 把浏览器选区折叠到 (node, offset)，chip 之类元素节点也要落到元素边界上。 */
  function placeCaret(node, offset) {
    var selection = window.getSelection()
    if (selection === null) return
    var range = document.createRange()
    if (node.nodeType === 3) range.setStart(node, offset)
    else if (offset < node.childNodes.length) range.setStart(node, offset)
    else range.setStartAfter(node)
    range.collapse(true)
    selection.removeAllRanges()
    selection.addRange(range)
  }

  /** 拖放点 → 把 composer 的插入点移动到拖放处；坐标不在 composer 内则不动。 */
  function moveCaretToPoint(root, x, y) {
    var caret = document.caretRangeFromPoint(x, y)
    if (!caret || caret.startContainer === null || caret.startContainer === undefined) return
    if (!root.contains(caret.startContainer)) return
    placeCaret(caret.startContainer, caret.startOffset)
  }

  /**
   * 拖放点对应的 detect 偏移：从根遍历到落点所在位置为止，文本算码元、
   * chip/换行算一个 U+FFFC、block 之间算一个换行——与 composer 的 clipboard
   * 投影一一对齐。落点不在根内时返回 null。
   */
  function detectOffsetOf(root, caret) {
    var container = caret.startContainer
    if (!root.contains(container)) return null
    var offset = caret.startOffset
    var probe = document.createRange()
    probe.setStart(root, 0)
    if (container.nodeType === 3 || offset < container.childNodes.length) probe.setEnd(container, offset)
    else probe.setEndAfter(container)
    var total = 0
    var blocks = 0
    var walker = document.createTreeWalker(root, NodeFilter.SHOW_TEXT | NodeFilter.SHOW_ELEMENT)
    for (var current = walker.nextNode(); current !== null; current = walker.nextNode()) {
      if (current.nodeType === 3) {
        if (current === container) {
          total += Math.min(probe.endOffset, current.length)
          return total + Math.max(0, blocks - 1)
        }
        total += current.length
        continue
      }
      if (isAtomic(current)) {
        if (current === container) return total + Math.min(probe.endOffset, 1) + Math.max(0, blocks - 1)
        total += 1
        continue
      }
      if (isBlock(current)) blocks += 1
    }
    return total + Math.max(0, blocks - 1)
  }

  /**
   * 一次插入的地址：draftRev 与位置在同一次读取里拿到，插入前现取，
   * 因此上传/列目录往返期间用户继续输入也不会让 span 的 revision 校验失败。
   * @param ctx - 插件上下文。
   * @param sessionId - 目标会话。
   * @param anchor - 拖放点的 detect 偏移；null 表示落在文档末尾。
   */
  function insertionSpan(ctx, sessionId, anchor) {
    var root = composerRoot()
    if (root === null) return null
    var actx = ctx.sessions.scope(sessionId)
    if (actx === undefined) return null
    var conversation = actx.get('conversation')
    if (conversation === undefined) return null
    var state = conversation.input.for(actx).state.getSnapshot()
    if (state.phase === 'adjudicating' || state.phase === 'submitting') return null
    var at = anchor === null ? detectLength(root) : Math.min(anchor, detectLength(root))
    return { actx: actx, span: { start: at, end: at, draftRev: state.draftRev } }
  }

  /** 在 anchor 处插入一条文件引用（label 显示文件名，clipboardText 为路径）。 */
  function insertReference(ctx, sessionId, anchor, ref, label) {
    var current = insertionSpan(ctx, sessionId, anchor)
    if (current === null) return false
    return current.actx.bail(current.actx, 'slash/input-insert-reference', {
      reference: {
        source: SOURCE_NAME,
        ref: ref,
        label: label,
        clipboardText: ref
      },
      span: current.span
    }) === true
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

  async function collectFilesFromEntry(entry) {
    if (entry.isFile) {
      return new Promise(function (resolve) {
        entry.file(function (file) {
          resolve([file])
        }, function () {
          resolve([])
        })
      })
    }
    if (entry.isDirectory) {
      var reader = entry.createReader()
      var all = []
      var readBatch = function () {
        return new Promise(function (resolve) {
          reader.readEntries(function (entries) { resolve(entries) }, function () { resolve([]) })
        })
      }
      for (;;) {
        var batch = await readBatch()
        if (batch.length === 0) break
        for (var k = 0; k < batch.length; k++) {
          all = all.concat(await collectFilesFromEntry(batch[k]))
        }
      }
      return all
    }
    return []
  }

  /** 同步 snapshot DataTransfer 的 FileSystemEntry；drop 事件结束后 items 会失效。 */
  function snapshotEntries(dataTransfer) {
    var items = dataTransfer && dataTransfer.items
    if (!items) return []
    var entries = []
    for (var i = 0; i < items.length; i++) {
      if (items[i].kind !== 'file') continue
      var entry = typeof items[i].webkitGetAsEntry === 'function' ? items[i].webkitGetAsEntry() : null
      if (entry) entries.push(entry)
    }
    return entries
  }

  async function collectFilesFromEntries(entries) {
    var out = []
    for (var i = 0; i < entries.length; i++) {
      out = out.concat(await collectFilesFromEntry(entries[i]))
    }
    return out
  }

  async function uploadOne(sessionId, file) {
    var res = await fetch('/api/dsh-drop', {
      method: 'POST',
      headers: {
        'x-file-name': encodeURIComponent(file.name),
        'x-session-id': sessionId
      },
      body: file
    })
    if (!res.ok) throw new Error('HTTP ' + res.status)
    var payload = await res.json()
    if (typeof payload.path !== 'string') throw new Error('missing path in response')
    return payload.path
  }

  async function listDirectory(path) {
    var res = await fetch('/api/dsh-drop-list-dir', {
      method: 'POST',
      headers: { 'content-type': 'application/json' },
      body: JSON.stringify({ path: path })
    })
    if (!res.ok) throw new Error('HTTP ' + res.status)
    var payload = await res.json()
    if (typeof payload.isDirectory !== 'boolean') throw new Error('missing isDirectory in response')
    return payload
  }

  /**
   * 一条引用插完后的新锚点：chip 占一个 U+FFFC，后面跟着一个分隔空格
   * （composer 在下一个字符不是空格时会补一个），所以推进两位。
   */
  function advance(anchor, root) {
    var tail = root === null ? 0 : detectLength(root)
    var next = anchor + 2
    return next > tail ? tail : next
  }

  /**
   * 把一次拖拽落成引用。每个引用轮到自己时才取 span（revision 现读现用），
   * 并且每插一条就把锚点推到新 chip 之后，多条拖入保持拖拽顺序。
   * @param anchor - 拖放点对应的 detect 偏移。
   */
  async function handleDrop(ctx, sessionId, payload, anchor) {
    var at = anchor
    if (payload.kind === 'files') {
      // files 为空说明是「文件夹」拖拽：走 webkitGetAsEntry 递归收集目录里的文件。
      var files = payload.files
      if (files.length === 0 && payload.entries.length > 0) {
        files = await collectFilesFromEntries(payload.entries)
      }
      for (var i = 0; i < files.length; i++) {
        var file = files[i]
        try {
          // 优先使用真实路径（部分运行环境仍暴露 File.path）；拿不到时才上传落盘。
          var realPath = (typeof file.path === 'string' && file.path !== '') ? file.path : null
          var path = realPath || await uploadOne(sessionId, file)
          if (insertReference(ctx, sessionId, at, path, file.name)) at = advance(at, composerRoot())
        } catch (e) {
          console.error('[dsh-drop-caret] reference insert failed:', e)
        }
      }
      return
    }
    if (payload.kind === 'uris') {
      // VS Code Explorer 拖拽：可能是文件或文件夹。
      for (var j = 0; j < payload.uris.length; j++) {
        var p = fileUriToPath(payload.uris[j])
        if (p === null) continue
        try {
          var info = await listDirectory(p)
          if (info.isDirectory) {
            // 文件夹：只插入文件夹路径本身（显示为「@文件夹名」），不展开子文件。
            if (insertReference(ctx, sessionId, at, p, basenameOf(p))) at = advance(at, composerRoot())
            continue
          }
          var paths = info.files || []
          for (var k = 0; k < paths.length; k++) {
            if (insertReference(ctx, sessionId, at, paths[k], basenameOf(paths[k]))) at = advance(at, composerRoot())
          }
        } catch (e) {
          console.error('[dsh-drop-caret] reference insert failed:', e)
        }
      }
      return
    }
    if (payload.kind === 'code') {
      // VS Code 扩展发来的选中代码段：插入「路径:起始行-结束行」引用，让模型能定位。
      var range = payload.endLine > payload.startLine ? '-' + payload.endLine : ''
      try {
        insertReference(ctx, sessionId, at, payload.path + ':' + payload.startLine + range,
          basenameOf(payload.path) + ':' + payload.startLine + range)
      } catch (e) {
        console.error('[dsh-drop-caret] reference insert failed:', e)
      }
    }
  }

  /** 拖拽携带的引用来源；null 表示这次拖拽不归本插件管。 */
  function dropKind(dataTransfer) {
    if (!dataTransfer) return null
    var types = toArray(dataTransfer.types)
    // URI 来源优先：VS Code Explorer 拖拽会同时带 text/uri-list 与 Files，
    // 先判 URI 才能拿到源文件真实路径（否则会走 Files 分支落盘副本）。
    if (types.indexOf('text/uri-list') >= 0 || types.indexOf('application/vnd.code.tree') >= 0) return 'uris'
    if (types.indexOf('Files') >= 0) return 'files'
    return null
  }

  /** 从一次 drop 事件取出 payload；拖拽数据只能在事件同步阶段读。 */
  function payloadOf(kind, dataTransfer) {
    if (kind === 'files') {
      // 无 URI 来源（真正的 OS 拖入）：浏览器拿不到真实路径，只能回退上传暂存。
      return { kind: 'files', files: toArray(dataTransfer.files), entries: snapshotEntries(dataTransfer) }
    }
    var uriList = dataTransfer.getData('text/uri-list')
    if (uriList) {
      var uris = parseUriList(uriList)
      if (uris.length > 0) return { kind: 'uris', uris: uris }
    }
    var treeData = dataTransfer.getData('application/vnd.code.tree')
    if (treeData) {
      var treeUris = extractUrisFromTree(treeData)
      if (treeUris.length > 0) return { kind: 'uris', uris: treeUris }
    }
    return null
  }

  /**
   * 在 window 捕获阶段接管拖拽（抢在 DSH 内置图片 drop 处理器之前），
   * 返回卸载函数。
   */
  function installDropCapture(ctx) {
    function sessionIdOf() {
      return ctx.sessions.current.getSnapshot()
    }
    function onDragOver(event) {
      var kind = dropKind(event.dataTransfer)
      if (kind === null) return
      var root = composerRoot()
      var over = event.target
      if (root === null || !(over instanceof Node) || !root.contains(over)) return
      event.preventDefault()
      event.dataTransfer.dropEffect = 'copy'
      event.stopPropagation()
    }
    function onDrop(event) {
      var kind = dropKind(event.dataTransfer)
      if (kind === null) return
      var root = composerRoot()
      var target = event.target
      if (root === null || !(target instanceof Node) || !root.contains(target)) return
      // DSH 内置处理器收到非图片文件会报「仅支持 PNG/JPG/WebP/GIF」并弹全屏遮罩。
      event.preventDefault()
      event.stopPropagation()
      if (typeof event.stopImmediatePropagation === 'function') event.stopImmediatePropagation()
      var payload = payloadOf(kind, event.dataTransfer)
      if (payload === null) return
      var caret = document.caretRangeFromPoint(event.clientX, event.clientY)
      var anchor = caret === null ? null : detectOffsetOf(root, caret)
      moveCaretToPoint(root, event.clientX, event.clientY)
      handleDrop(ctx, sessionIdOf(), payload, anchor)
    }
    // VS Code 扩展（经 webview 转发）发来的「选中代码段」。
    function onMessage(event) {
      var data = event.data
      if (!data || data.type !== 'insert-selection') return
      if (typeof data.filePath !== 'string' || data.filePath === '') return
      var start = Number(data.startLine) || 1
      var end = Number(data.endLine) || start
      handleDrop(ctx, sessionIdOf(), { kind: 'code', path: data.filePath, startLine: start, endLine: end }, null)
    }
    window.addEventListener('dragover', onDragOver, true)
    window.addEventListener('drop', onDrop, true)
    window.addEventListener('message', onMessage)
    return function () {
      window.removeEventListener('dragover', onDragOver, true)
      window.removeEventListener('drop', onDrop, true)
      window.removeEventListener('message', onMessage)
    }
  }

  function apply(ctx) {
    // 注册引用源：让 composer 认识 source=dsh-drop-caret 的引用，
    // 并提供序列化 codec（发送时把路径转成文本交给模型）。
    ctx.effect(function () {
      return ctx.inputTriggers.registerSource({
        trigger: '@',
        name: SOURCE_NAME,
        candidates: async function () { return [] },
        onPick: function () { return undefined },
        codec: {
          clipboardText: function (ref) { return ref },
          serialize: async function (ref) { return ref }
        }
      })
    })
    ctx.effect(function () {
      return installDropCapture(ctx)
    })
  }

  exports.apply = apply
  exports.inject = ['inputTriggers', 'sessions']
  return module.exports
} })
