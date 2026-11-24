'use strict'

const vscode = require('vscode')
const path = require('node:path')
const fs = require('node:fs')
const { execFileSync } = require('node:child_process')

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

function git(cwd, ...args) {
  return execFileSync('git', ['-C', cwd, ...args], {
    encoding: 'utf8',
    timeout: 10000,
    windowsHide: true,
    stdio: ['ignore', 'pipe', 'ignore']
  }).trim()
}

function firstExistingDirectory(candidates) {
  for (const candidate of candidates) {
    if (candidate && typeof candidate === 'string') {
      try {
        if (fs.statSync(candidate).isDirectory()) return candidate
      } catch {
        continue
      }
    }
  }
  return null
}

function fallbackBranch() {
  const folders = []
  if (vscode.workspace.workspaceFolders) {
    for (const folder of vscode.workspace.workspaceFolders) folders.push(folder.uri.fsPath)
  }
  let directory = firstExistingDirectory(folders)
  if (!directory) {
    const active = vscode.window.activeTextEditor
    directory = active ? path.dirname(active.document.uri.fsPath) : process.cwd()
  }
  let branch = ''
  try {
    branch = git(directory, 'rev-parse', '--abbrev-ref', 'HEAD')
  } catch {
    return ''
  }
  if (!branch || branch === 'HEAD') {
    try {
      return git(directory, 'rev-parse', '--short', 'HEAD')
    } catch {
      return ''
    }
  }
  return branch
}

let branchCache = { value: '', at: 0 }
let gitWatcher = null

async function currentBranch() {
  for (const repository of vscode.extensions.getExtension('vscode.git')?.exports?.getAPI(1)?.repositories ?? []) {
    const head = repository.state && repository.state.HEAD
    if (head && head.name) return head.name
    if (head && head.commit) {
      const short = String(head.commit).slice(0, 8)
      if (short !== '') return short
    }
  }
  const now = Date.now()
  if (branchCache.value !== '' && now - branchCache.at < 5000) return branchCache.value
  branchCache = { value: fallbackBranch(), at: now }
  return branchCache.value
}

function watchGit() {
  if (gitWatcher) return
  const api = vscode.extensions.getExtension('vscode.git')?.exports
  if (!api) return
  gitWatcher = api.onDidOpenRepository((repository) => {
    branchCache.at = 0
    repository.state.onDidChange(() => {
      branchCache.at = 0
    })
  })
}

async function getBranchName() {
  const branch = await currentBranch()
  if (branch === '') {
    vscode.window.showErrorMessage('无法获取当前 git 分支：请确认工作区是 git 仓库且 git 在 PATH 中。')
    return '${workspaceFolder}'
  }
  return branch
}

function activate(context) {
  context.subscriptions.push(vscode.commands.registerCommand('dshSel.sendSelection', sendSelection))
  context.subscriptions.push(vscode.commands.registerCommand('dshSel.checkEndpoint', checkEndpoint))
  context.subscriptions.push(vscode.commands.registerCommand('dshSel.getBranchName', getBranchName))
  context.subscriptions.push({ dispose: () => gitWatcher && gitWatcher.dispose() })
  watchGit()
}

function deactivate() {}

module.exports = { activate, deactivate }
