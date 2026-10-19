// dsh-vscode-sel host half: a local HTTP mailbox between a VS Code extension
// and the browser page running DSH.
//
// VS Code cannot reach a browser page directly, so both sides meet here:
// the extension POSTs one selection to `/api/dsh-vscode-sel/push`, and the
// page's client half polls `/api/dsh-vscode-sel/pending` when the window gains
// focus. The mailbox is in-process memory: bounded, short-lived, and emptied by
// the first read.
//
// Routes are registered through `ctx.webServer.register`, which dispatches
// straight to the handler (no gateway authentication layer), so no credential
// exchange is involved.

/** Cordis plugin name — must match the row name in cordis.patch.yml. */
export const name = 'dsh-vscode-sel'

/** Services required by this plugin. */
export const inject = ['webServer']

const PREFIX = '/api/dsh-vscode-sel'
const PUSH_PATH = `${PREFIX}/push`
const PENDING_PATH = `${PREFIX}/pending`

const MAX_ENTRIES = 32
const MAX_BODY_BYTES = 64 * 1024
const TTL_MS = 60 * 1000

/** One queued selection: a file plus an optional inclusive line range. */
let entries = []
let nextSeq = 1

function writeJson(res, status, payload) {
  const body = JSON.stringify(payload)
  res.writeHead(status, { 'content-type': 'application/json; charset=utf-8' })
  res.end(body)
}

function prune() {
  const deadline = Date.now() - TTL_MS
  if (entries.length === 0) return
  entries = entries.filter((entry) => entry.ts >= deadline)
  if (entries.length > MAX_ENTRIES) entries = entries.slice(entries.length - MAX_ENTRIES)
}

function drain() {
  prune()
  const drained = entries
  entries = []
  return drained
}

function readBody(req) {
  return new Promise((resolve, reject) => {
    const declared = Number(req.headers['content-length'])
    if (Number.isFinite(declared) && declared > MAX_BODY_BYTES) {
      reject(new Error('payload too large'))
      return
    }
    let size = 0
    const chunks = []
    req.on('data', (chunk) => {
      size += chunk.length
      if (size > MAX_BODY_BYTES) {
        reject(new Error('payload too large'))
        req.destroy()
        return
      }
      chunks.push(chunk)
    })
    req.on('end', () => {
      resolve(Buffer.concat(chunks).toString('utf8'))
    })
    req.on('error', reject)
  })
}

function normalizeLine(value, fallback) {
  const parsed = Number(value)
  if (!Number.isFinite(parsed)) return fallback
  const floored = Math.floor(parsed)
  return floored >= 1 ? floored : fallback
}

function parseSelection(text) {
  let payload
  try {
    payload = JSON.parse(text)
  } catch {
    return null
  }
  if (payload === null || typeof payload !== 'object') return null
  const filePath = typeof payload.filePath === 'string' ? payload.filePath.trim() : ''
  if (filePath === '') return null
  const startLine = normalizeLine(payload.startLine, 1)
  const endLine = normalizeLine(payload.endLine, startLine)
  return { filePath, startLine, endLine: Math.max(startLine, endLine) }
}

function pathOf(req) {
  const url = req.url ?? '/'
  const question = url.indexOf('?')
  return question === -1 ? url : url.slice(0, question)
}

async function handlePush(req, res) {
  if (req.method !== 'POST') {
    res.writeHead(405, { allow: 'POST' })
    res.end()
    return
  }
  let text
  try {
    text = await readBody(req)
  } catch (error) {
    writeJson(res, 413, { error: error instanceof Error ? error.message : String(error) })
    return
  }
  const selection = parseSelection(text)
  if (selection === null) {
    writeJson(res, 400, { error: 'expected { filePath, startLine?, endLine? }' })
    return
  }
  prune()
  const entry = { seq: nextSeq, ts: Date.now(), ...selection }
  nextSeq += 1
  entries.push(entry)
  prune()
  writeJson(res, 200, { ok: true, seq: entry.seq })
}

function handlePending(req, res) {
  if (req.method === 'GET') {
    writeJson(res, 200, { items: drain() })
    return
  }
  res.writeHead(405, { allow: 'GET' })
  res.end()
}

export function apply(ctx) {
  ctx.effect(() =>
    ctx.webServer.register({
      kind: 'prefix',
      path: PREFIX,
      handler: async (req, res) => {
        try {
          const path = pathOf(req)
          if (path === PUSH_PATH) {
            await handlePush(req, res)
            return
          }
          if (path === PENDING_PATH) {
            handlePending(req, res)
            return
          }
          writeJson(res, 404, { error: 'unknown route' })
        } catch (error) {
          console.error('[dsh-vscode-sel] request failed:', error)
          if (!res.headersSent) writeJson(res, 500, { error: 'internal error' })
          else res.end()
        }
      }
    })
  )
}
