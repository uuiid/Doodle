// dsh-drop-caret client bundle (ModuleLoader format)
// dsh-drop-caret client bundle (ModuleLoader format)
// dsh-drop-caret client half: document-level file drop handler that maps the
// drop point to a caret offset in the composer and inserts each file reference
// exactly there (instead of appending to the end).
//
// Two drop sources (URI 来源优先——拖入即引用源文件本身，不产生副本):
//   1. VS Code Explorer — `text/uri-list` / `application/vnd.code.tree` →
//      parse the local file:// URI to a path, insert it directly (源文件路径).
//   2. OS file manager — only `Files` in dataTransfer；浏览器（现代 Electron 已
//      移除 File.path）无法取得真实路径，此时才退化为上传落盘到
//      `.dsh-drop/<sessionId>/` 并插入该副本路径；若运行环境仍提供 file.path
//      则直接使用真实路径。
// NOTE: VS Code Explorer 拖拽同时携带 Files 与 text/uri-list，故类型判定必须
// 先判 URI 类型，否则会被误判为 OS 拖入而去落盘副本。
//
// IMPORTANT: listeners run in the CAPTURE phase and stop propagation, so the
// DSH built-in image-drop handler never sees a non-image file (which is what
// produced the "仅支持PNG/JPG/WebP/GIF" error + blue box).

window.__ModuleLoader__.load({ id: 'dsh-drop-caret', factory: (require) => {
  var module = { exports: {} }
  var exports = module.exports
  var React = require('react')

  var SOURCE_NAME = 'dsh-drop-caret'

  /** basename，兼容 Windows \ 与 POSIX / 分隔符。 */
  function basenameOf(path) {
    var i = Math.max(path.lastIndexOf('\\'), path.lastIndexOf('/'))
    return i === -1 ? path : path.slice(i + 1)
  }

  /** 把 drop 视口坐标映射成 composer textarea 的字符偏移（镜像 div + caretRangeFromPoint）。 */
  function caretOffsetFromPoint(x, y) {
    var ta = document.querySelector('textarea')
    if (ta === null) return -1
    var cs = getComputedStyle(ta)
    var r = ta.getBoundingClientRect()
    var m = document.createElement('div')
    var props = [
      'boxSizing', 'width', 'paddingTop', 'paddingRight', 'paddingBottom', 'paddingLeft',
      'borderTopWidth', 'borderRightWidth', 'borderBottomWidth', 'borderLeftWidth',
      'fontStyle', 'fontVariant', 'fontWeight', 'fontSize', 'fontFamily',
      'lineHeight', 'letterSpacing', 'tabSize', 'whiteSpace', 'wordBreak', 'textAlign'
    ]
    for (var i = 0; i < props.length; i++) {
      try { m.style[props[i]] = cs[props[i]] } catch (e) { /* ignore */ }
    }
    m.style.position = 'fixed'
    m.style.left = r.left + 'px'
    m.style.top = r.top + 'px'
    m.style.width = r.width + 'px'
    m.style.minHeight = r.height + 'px'
    m.style.whiteSpace = 'pre-wrap'
    m.style.wordBreak = 'break-word'
    m.style.color = 'transparent'
    m.style.background = 'transparent'
    m.style.zIndex = '2147483647'
    m.style.pointerEvents = 'none'
    m.contentEditable = 'true'
    m.textContent = ta.value || ''
    document.body.appendChild(m)

    var value = ta.value || ''
    var offset = value.length
    try {
      var rg = document.caretRangeFromPoint(x, y)
      if (rg && rg.startContainer && m.contains(rg.startContainer)) {
        offset = rg.startOffset
      }
    } catch (e) { /* fall back to append */ }
    m.remove()
    return offset
  }

  /** 在指定字符偏移处插入文件引用（label 显示文件名，clipboardText 为路径）。 */
  function insertReferenceAt(actx, ref, label, offset) {
    var conversation = actx.get('conversation')
    if (conversation === undefined) throw new Error('conversation service unavailable')
    var input = conversation.input.for(actx)
    var state = input.state.getSnapshot()
    var draftLen = state.draft.length
    var pos = offset < 0 || offset > draftLen ? draftLen : offset
    actx.emit('slash/input-insert-reference', {
      reference: {
        source: SOURCE_NAME,
        ref: ref,
        label: label,
        clipboardText: ref
      },
      span: { start: pos, end: pos, draftRev: state.draftRev }
    })
  }

  /** Windows 盘符路径统一为 `V:\a\b` 风格（盘符大写、反斜杠），与拖拽前路径一致。 */
  function normalizeWinPath(p) {
    if (/^[A-Za-z]:[\\/]/.test(p)) {
      return p.charAt(0).toUpperCase() + ':' + p.slice(2).replace(/\//g, '\\')
    }
    return p
  }

  /** file:// URI / Windows 盘符路径 / POSIX 绝对路径 → 本地路径；否则 null。 */
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

  /** 递归遍历 FileSystemEntry（目录/文件），收集所有文件（扁平化）。 */
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
          var sub = await collectFilesFromEntry(batch[k])
          all = all.concat(sub)
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
      var item = items[i]
      if (item.kind !== 'file') continue
      var entry = typeof item.webkitGetAsEntry === 'function' ? item.webkitGetAsEntry() : null
      if (entry) entries.push(entry)
    }
    return entries
  }

  /** 从已 snapshot 的 FileSystemEntry 列表收集文件（含目录递归），用于文件夹拖拽。 */
  async function collectFilesFromEntries(entries) {
    if (!entries) return []
    var out = []
    for (var i = 0; i < entries.length; i++) {
      var collected = await collectFilesFromEntry(entries[i])
      out = out.concat(collected)
    }
    return out
  }

  /** 从 DataTransferItemList 收集文件（含目录递归），用于文件夹拖拽（files 为空时）。 */
  async function collectFilesFromItems(items) {
    if (!items) return []
    var out = []
    for (var i = 0; i < items.length; i++) {
      var item = items[i]
      if (item.kind !== 'file') continue
      var entry = typeof item.webkitGetAsEntry === 'function' ? item.webkitGetAsEntry() : null
      if (!entry) continue
      var collected = await collectFilesFromEntry(entry)
      out = out.concat(collected)
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

  /** 调用 Host 端 API 判断路径是文件还是文件夹（不再递归展开文件夹）。 */
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

  async function handleDrop(ctx, sessionId, payload, caret) {
    var actx = ctx.sessions.scope(sessionId)
    var pos = caret
    if (payload.kind === 'files') {
      // files 为空说明是「文件夹」拖拽：走 webkitGetAsEntry 递归收集目录里的文件。
      var files = payload.files
      if (files.length === 0 && payload.entries && payload.entries.length > 0) {
        files = await collectFilesFromEntries(payload.entries)
      }
      for (var i = 0; i < files.length; i++) {
        var file = files[i]
        try {
          // 优先使用真实路径（部分运行环境仍暴露 File.path）；拿不到时才上传落盘。
          // 现代 Electron 已移除 File.path，OS 拖入无路径可用，故保留该兜底。
          var realPath = (typeof file.path === 'string' && file.path !== '') ? file.path : null
          var path = realPath || await uploadOne(sessionId, file)
          insertReferenceAt(actx, path, file.name, pos)
          pos += path.length + 1
        } catch (e) {
          console.error('[dsh-drop-caret] upload failed:', e)
        }
      }
    } else if (payload.kind === 'uris') {
      // VS Code Explorer 拖拽：可能是文件或文件夹。
      for (var j = 0; j < payload.uris.length; j++) {
        var p = fileUriToPath(payload.uris[j])
        if (p === null) continue
        try {
          var info = await listDirectory(p)
          if (info.isDirectory) {
            // 文件夹：只插入文件夹路径本身（显示为「@文件夹名」），不展开子文件。
            insertReferenceAt(actx, p, basenameOf(p), pos)
            pos += p.length + 1
          } else {
            var paths = info.files || []
            for (var k = 0; k < paths.length; k++) {
              insertReferenceAt(actx, paths[k], basenameOf(paths[k]), pos)
              pos += paths[k].length + 1
            }
          }
        } catch (e) {
          console.error('[dsh-drop-caret] list/insert failed:', e)
        }
      }
    } else if (payload.kind === 'code') {
      // VS Code 扩展发来的选中代码段：插入「路径:起始行-结束行」引用，让模型能定位。
      var ref = payload.path + ':' + payload.startLine + (payload.endLine > payload.startLine ? '-' + payload.endLine : '')
      var label = basenameOf(payload.path) + ':' + payload.startLine + (payload.endLine > payload.startLine ? '-' + payload.endLine : '')
      try {
        insertReferenceAt(actx, ref, label, caret)
      } catch (e) {
        console.error('[dsh-drop-caret] code insert failed:', e)
      }
    }
  }

  /** 当前 composer textarea 光标位置；未聚焦返回 -1（追加到末尾）。 */
  function getCurrentCaret() {
    var ta = document.querySelector('textarea')
    if (ta && typeof ta.selectionStart === 'number') return ta.selectionStart
    return -1
  }

  function DropCaret(props) {
    var handlerRef = React.useRef(props.onDrop)
    handlerRef.current = props.onDrop

    React.useEffect(function () {
      var depth = 0
      function kindOf(e) {
        var dt = e.dataTransfer
        if (!dt) return 'none'
        var types = dt.types || []
        // URI 来源优先：VS Code Explorer 拖拽会同时带 text/uri-list 与 Files，
        // 先判 URI 才能拿到源文件真实路径（否则会走 Files 分支落盘副本）。
        if (Array.prototype.indexOf.call(types, 'text/uri-list') >= 0 ||
            Array.prototype.indexOf.call(types, 'application/vnd.code.tree') >= 0) return 'uris'
        if (Array.prototype.indexOf.call(types, 'Files') >= 0) return 'files'
        return 'none'
      }
      function isOurs(k) {
        return k === 'files' || k === 'uris'
      }
      function onDragEnter(e) {
        var k = kindOf(e)
        if (!isOurs(k)) return
        e.preventDefault()
        e.stopPropagation()
        depth += 1
      }
      function onDragOver(e) {
        var k = kindOf(e)
        if (!isOurs(k)) return
        e.preventDefault()
        e.stopPropagation()
      }
      function onDragLeave(e) {
        var k = kindOf(e)
        if (!isOurs(k)) return
        e.stopPropagation()
        if (e.relatedTarget === null) depth = Math.max(0, depth - 1)
      }
      function onDrop(e) {
        var k = kindOf(e)
        if (!isOurs(k)) return
        // 抢在 DSH 内置图片 drop 处理器之前：preventDefault + 停止传播，
        // 避免「仅支持PNG…」报错与其全屏遮罩弹出/卡住。
        e.preventDefault()
        e.stopPropagation()
        if (typeof e.stopImmediatePropagation === 'function') e.stopImmediatePropagation()
        depth = 0
        var caret = caretOffsetFromPoint(e.clientX, e.clientY)
        var dt = e.dataTransfer

        if (k === 'files') {
          // 无 URI 来源（真正的 OS 拖入）：浏览器拿不到真实路径，只能回退上传暂存。
          console.warn('[dsh-drop-caret] 拖入无 text/uri-list，回退上传暂存；types=', dt.types)
          handlerRef.current({ kind: 'files', files: Array.prototype.slice.call(dt.files || []), entries: snapshotEntries(dt) }, caret)
          return
        }
        var uriList = dt.getData('text/uri-list')
        if (uriList) {
          var uris = parseUriList(uriList)
          if (uris.length > 0) {
            handlerRef.current({ kind: 'uris', uris: uris }, caret)
            return
          }
        }
        var treeData = dt.getData('application/vnd.code.tree')
        if (treeData) {
          var treeUris = extractUrisFromTree(treeData)
          if (treeUris.length > 0) {
            handlerRef.current({ kind: 'uris', uris: treeUris }, caret)
            return
          }
        }
      }
      function onDragEnd() {
        depth = 0
      }
      // 接收 VS Code 扩展（经 webview 转发）发来的「选中代码段」消息。
      function onMessage(event) {
        var data = event.data
        if (!data || data.type !== 'insert-selection') return
        if (typeof data.filePath !== 'string' || data.filePath === '') return
        var s = Number(data.startLine) || 1
        var e = Number(data.endLine) || s
        handlerRef.current({ kind: 'code', path: data.filePath, startLine: s, endLine: e }, getCurrentCaret())
      }

      // window 捕获阶段 + 全生命周期拦截：抢在 DSH 内置拖拽/图片处理器之前，
      // 且不让它的全屏遮罩弹出或卡住。
      window.addEventListener('message', onMessage)
      var W = window
      W.addEventListener('dragenter', onDragEnter, true)
      W.addEventListener('dragover', onDragOver, true)
      W.addEventListener('dragleave', onDragLeave, true)
      W.addEventListener('drop', onDrop, true)
      W.addEventListener('dragend', onDragEnd, true)
      return function () {
        window.removeEventListener('message', onMessage)
        W.removeEventListener('dragenter', onDragEnter, true)
        W.removeEventListener('dragover', onDragOver, true)
        W.removeEventListener('dragleave', onDragLeave, true)
        W.removeEventListener('drop', onDrop, true)
        W.removeEventListener('dragend', onDragEnd, true)
      }
    }, [])

    return null
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
    ctx.slots.inject('conversation.input.left', function () {
      return ctx.slots.register(
        {
          name: 'conversation.input.left',
          id: 'dsh-drop-caret',
          order: 0,
          inject: function (sessionId) {
            return {
              onDrop: function (payload, caret) {
                return handleDrop(ctx, sessionId, payload, caret)
              }
            }
          }
        },
        DropCaret
      )
    })
  }

  exports.apply = apply
  exports.inject = ['slots', 'inputTriggers', 'sessions']
  return module.exports
} })

// [end of dsh-drop-caret client bundle]
// [end of dsh-drop-caret client bundle]