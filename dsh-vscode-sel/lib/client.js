// dsh-vscode-sel client half: pull the selection the VS Code extension left in
// the host mailbox and type it into the composer at the caret.
//
// The insertion is plain text, inserted with the browser's own editing
// primitives, so the caret is wherever the user left it. DSH's structured
// `slash/input-insert-reference` channel is deliberately unused: its span is an
// editor detect-projection offset, and that projection (one character per
// reference chip) is not reachable from outside the editor model, so an empty
// span is rejected as out of bounds without any error surfacing.
//
// Polling is focus-driven: nothing runs while the page is hidden, and one pull
// happens each time the window becomes visible again.

window.__ModuleLoader__.load({ id: 'dsh-vscode-sel', factory: () => {
  var module = { exports: {} }
  var exports = module.exports

  var PENDING_PATH = '/api/dsh-vscode-sel/pending'
  var ENDPOINT_GLOBAL = '__DSH_VSCODE_SEL_ENDPOINT__'

  function endpoint() {
    var injected = globalThis[ENDPOINT_GLOBAL]
    if (typeof injected === 'string' && injected !== '') return injected.replace(/\/$/, '')
    return location.origin
  }

  function composerRoot() {
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

  function typeText(text) {
    var root = composerRoot()
    if (root === null) return false
    root.focus()
    var range = caretRange(root)
    var selection = window.getSelection()
    if (selection === null) return false
    selection.removeAllRanges()
    selection.addRange(range)
    var applied = false
    try {
      applied = document.execCommand('insertText', false, text)
    } catch {
      applied = false
    }
    if (applied) return true
    range.deleteContents()
    range.insertNode(document.createTextNode(text))
    range.collapse(false)
    selection.removeAllRanges()
    selection.addRange(range)
    root.dispatchEvent(new InputEvent('input', { bubbles: true, inputType: 'insertText', data: text }))
    return true
  }

  function lineSuffix(entry) {
    var start = typeof entry.startLine === 'number' && entry.startLine >= 1 ? entry.startLine : 1
    var end = typeof entry.endLine === 'number' && entry.endLine >= start ? entry.endLine : start
    return end > start ? ':' + start + '-' + end : ':' + start
  }

  function describe(entry) {
    if (entry === null || typeof entry !== 'object') return ''
    if (typeof entry.filePath !== 'string' || entry.filePath === '') return ''
    return '@' + entry.filePath + lineSuffix(entry)
  }

  function install(ctx) {
    var pulling = false

    function insertItems(items) {
      if (!Array.isArray(items) || items.length === 0) return
      var texts = []
      for (var i = 0; i < items.length; i++) {
        var text = describe(items[i])
        if (text !== '') texts.push(text)
      }
      if (texts.length === 0) return
      typeText(texts.join(' ') + ' ')
    }

    function pull() {
      if (pulling || globalThis.__DSH_VSCODE_SEL_DISABLED__ === true) return
      if (document.visibilityState === 'hidden') return
      pulling = true
      fetch(endpoint() + PENDING_PATH, { method: 'GET', cache: 'no-store' })
        .then(function (response) {
          if (!response.ok) throw new Error('pending failed: ' + response.status)
          return response.json()
        })
        .then(function (payload) {
          insertItems(payload === null ? null : payload.items)
        })
        .catch(function (reason) {
          console.warn('[dsh-vscode-sel] pull failed:', reason)
        })
        .then(function () {
          pulling = false
        })
    }

    function onFocus() {
      pull()
    }

    function onVisibility() {
      if (document.visibilityState === 'visible') pull()
    }

    window.addEventListener('focus', onFocus)
    document.addEventListener('visibilitychange', onVisibility)

    var timer = window.setInterval(pull, 5000)

    globalThis.__dshVscodeSel__ = {
      /** Pull now instead of waiting for the next focus or timer tick. */
      pull: pull,
      /** Turn the background timer off when something else drives insertion. */
      disable: function () {
        globalThis.__DSH_VSCODE_SEL_DISABLED__ = true
      },
      /** Re-enable the timer after a disable. */
      enable: function () {
        globalThis.__DSH_VSCODE_SEL_DISABLED__ = false
      }
    }

    return function () {
      window.removeEventListener('focus', onFocus)
      document.removeEventListener('visibilitychange', onVisibility)
      window.clearInterval(timer)
      delete globalThis.__dshVscodeSel__
    }
  }

  function apply(ctx) {
    ctx.effect(function () {
      return install(ctx)
    })
  }

  exports.apply = apply
  exports.inject = []
  return module.exports
} })
