'use strict'

const vscode = require('vscode')
const path = require('node:path')

const PUSH_PATH = '/api/dsh-vscode-sel/push'
const PENDING_PATH = '/api/dsh-vscode-sel/pending'

function config() {
  return vscode.workspace.getConfiguration('dshSel')
}

function endpoint() {
  const raw = String(config().get('endpoint') || '').trim()
  const base = raw === '' ? 'http://127.0.0.1:3080' : raw
  return base.replace(/\/+$/, '')
}

function pathFor(document) {
  const fsPath = document.uri.fsPath
  const format = config().get('format')
  if (format === 'basename') return path.basename(fsPath)
  return path.resolve(fsPath)
}

function lineSuffix(selection) {
  if (config().get('includeLineNumbers') !== true) return ''
  const start = selection.start.line + 1
  const end = selection.end.line + 1
  if (end > start) return `:${start}-${end}`
  return `:${start}`
}

async function push(payload) {
  const url = endpoint() + PUSH_PATH
  const controller = new AbortController()
  const timer = setTimeout(() => controller.abort(), 5000)
  try {
    const response = await fetch(url, {
      method: 'POST',
      headers: { 'content-type': 'application/json' },
      body: JSON.stringify(payload),
      signal: controller.signal
    })
    if (!response.ok) {
      const text = await response.text().catch(() => '')
      throw new Error(`HTTP ${response.status}${text === '' ? '' : ': ' + text}`)
    }
    return await response.json().catch(() => ({}))
  } finally {
    clearTimeout(timer)
  }
}

function failureHint(error) {
  const reason = error && error.name === 'AbortError' ? '请求超时' : error && error.message ? error.message : String(error)
  return `无法连接 DSH (${endpoint()})：${reason}。请确认 dsh web 正在运行、端口与 dshSel.endpoint 一致，且 dsh-vscode-sel 插件已安装。`
}

async function sendSelection() {
  const editor = vscode.window.activeTextEditor
  if (!editor) {
    vscode.window.showWarningMessage('请先打开一个编辑器并选中代码。')
    return
  }
  const selection = editor.selection
  if (selection.isEmpty) {
    vscode.window.showWarningMessage('请先选中代码片段。')
    return
  }
  const document = editor.document
  const payload = {
    filePath: pathFor(document),
    startLine: selection.start.line + 1,
    endLine: selection.end.line + 1
  }
  try {
    await push(payload)
    vscode.window.showInformationMessage(`已发送 ${payload.filePath}${lineSuffix(selection)}，切换到 DSH 窗口即可插入。`)
  } catch (error) {
    vscode.window.showErrorMessage(failureHint(error))
  }
}

async function checkEndpoint() {
  const url = endpoint() + PENDING_PATH
  try {
    const response = await fetch(url, { method: 'GET' })
    if (!response.ok) {
      vscode.window.showErrorMessage(`DSH 端点可达但返回 HTTP ${response.status} (${url})。`)
      return
    }
    const payload = await response.json().catch(() => ({}))
    const count = Array.isArray(payload.items) ? payload.items.length : 0
    vscode.window.showInformationMessage(`DSH 端点正常 (${url})，当前有 ${count} 条待取。`)
  } catch (error) {
    vscode.window.showErrorMessage(failureHint(error))
  }
}

function activate(context) {
  context.subscriptions.push(vscode.commands.registerCommand('dshSel.sendSelection', sendSelection))
  context.subscriptions.push(vscode.commands.registerCommand('dshSel.checkEndpoint', checkEndpoint))
}

function deactivate() {}

module.exports = { activate, deactivate }
