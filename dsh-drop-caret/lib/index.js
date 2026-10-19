// dsh-drop-caret host half: an HTTP route that persists uploaded files into
// the requesting session's workspace under `.dsh-drop/<sessionId>/`, so the
// agent's filesystem backend can always resolve the returned path.
//
// Also provides /api/dsh-drop-list-dir to recursively list all files in a
// directory, used when dropping a folder from VS Code Explorer.

import { mkdir, writeFile, stat } from 'node:fs/promises'
import { join, sep } from 'node:path'
import { createHash } from 'node:crypto'

/** Cordis plugin name — must match the row name in cordis.patch.yml. */
export const name = 'dsh-drop-caret'

/** Services required by this plugin. */
export const inject = ['webServer', 'sessions']

const MAX_BYTES = 24 * 1024 * 1024

/** 消毒文件名：剥控制字符/路径分隔/点段，按 UTF-8 字节截断并保留扩展名。 */
function sanitizeFileName(raw) {
  const cleaned = String(raw || '').replace(/[\u0000-\u001f\u007f]/g, '')
  const segments = cleaned.split(/[\\/]/).filter((s) => s !== '' && s !== '.' && s !== '..')
  const joined = segments.join('_').replace(/^\.+/, '').trim()
  const dot = joined.lastIndexOf('.')
  const ext = dot > 0 && dot < joined.length - 1 ? joined.slice(dot) : ''
  const stem = dot > 0 ? joined.slice(0, dot) : joined
  if (/^\.+$/.test(stem)) return 'upload.bin'
  const extBytes = Buffer.byteLength(ext)
  let bytes = 0
  let cut = stem.length
  for (let i = 0; i < stem.length; i++) {
    const code = stem.codePointAt(i) ?? 0
    const width = code > 0xffff ? 4 : code > 0x7ff ? 3 : code > 0x7f ? 2 : 1
    if (bytes + width > 120 - extBytes) {
      cut = i
      break
    }
    bytes += width
  }
  const out = stem.slice(0, cut) + ext
  return out === '' ? 'upload.bin' : out
}

/** Session id 是透明 token，仍约束到安全字母表。 */
function sanitizeSessionId(id) {
  const cleaned = String(id || '').replace(/[^A-Za-z0-9_-]+/g, '_').slice(0, 80)
  return cleaned === '' ? 'anonymous' : cleaned
}

function writeJson(res, status, body) {
  const payload = JSON.stringify(body)
  res.writeHead(status, { 'content-type': 'application/json' })
  res.end(payload)
}

export function apply(ctx) {
  const storageDirFor = (sessionId) => {
    const session = ctx.sessions.get(sessionId)
    const cwd = session === undefined ? undefined : session.header.cwd
    const base = cwd || process.cwd()
    return join(base, '.dsh-drop', sanitizeSessionId(sessionId))
  }

  // 端点 1: 上传文件（OS 文件管理器拖拽）
  ctx.effect(() =>
    ctx.webServer.register({
      kind: 'prefix',
      path: '/api/dsh-drop',
      handler: async (req, res) => {
        if (req.method !== 'POST') {
          res.writeHead(405, { allow: 'POST' })
          res.end('method not allowed')
          return
        }
        const sessionId = sanitizeSessionId(String(req.headers['x-session-id'] || ''))
        const dir = storageDirFor(sessionId)

        const declared = Number(req.headers['content-length'])
        if (Number.isFinite(declared) && declared > MAX_BYTES) {
          writeJson(res, 413, { error: 'payload too large' })
          return
        }

        try {
          const chunks = []
          let total = 0
          for await (const chunk of req) {
            const buf = Buffer.isBuffer(chunk) ? chunk : Buffer.from(chunk)
            total += buf.length
            if (total > MAX_BYTES) {
              writeJson(res, 413, { error: 'payload too large' })
              return
            }
            chunks.push(buf)
          }
          if (total === 0) {
            writeJson(res, 400, { error: 'empty upload' })
            return
          }

          let rawName = 'upload.bin'
          try {
            const header = String(req.headers['x-file-name'] || '')
            if (header !== '') rawName = decodeURIComponent(header)
          } catch {
            /* fall through to default name */
          }
          const name = sanitizeFileName(rawName)
          const data = Buffer.concat(chunks)

          await mkdir(dir, { recursive: true })
          const digest = createHash('sha256').update(data).digest('hex').slice(0, 12)
          const dest = join(dir, `${digest}-${name}`)
          await writeFile(dest, data)

          writeJson(res, 200, { path: dest, name, bytes: data.length })
        } catch (err) {
          console.error('[dsh-drop-caret] upload persist failed:', err)
          writeJson(res, 500, { error: 'write failed' })
        }
      }
    })
  )

  // 端点 2: 递归列出文件夹中的文件（VS Code Explorer 文件夹拖拽）
  ctx.effect(() =>
    ctx.webServer.register({
      kind: 'prefix',
      path: '/api/dsh-drop-list-dir',
      handler: async (req, res) => {
        if (req.method !== 'POST') {
          res.writeHead(405, { allow: 'POST' })
          res.end('method not allowed')
          return
        }

        try {
          const chunks = []
          for await (const chunk of req) {
            chunks.push(Buffer.isBuffer(chunk) ? chunk : Buffer.from(chunk))
          }
          const body = JSON.parse(Buffer.concat(chunks).toString('utf-8'))
          const dirPath = body.path

          if (typeof dirPath !== 'string' || dirPath === '') {
            writeJson(res, 400, { error: 'missing path' })
            return
          }

          // 检查是否是目录
          let stats
          try {
            stats = await stat(dirPath)
          } catch (err) {
            writeJson(res, 404, { error: 'path not found', details: err.message })
            return
          }

          if (!stats.isDirectory()) {
            // 单个文件直接返回
            writeJson(res, 200, { isDirectory: false, files: [dirPath] })
            return
          }

          // 文件夹：不再递归展开，仅返回目录类型，由 client 直接插入「@文件夹名」
          writeJson(res, 200, { isDirectory: true })
        } catch (err) {
          console.error('[dsh-drop-caret] list-dir failed:', err)
          writeJson(res, 500, { error: 'list failed', details: err.message })
        }
      }
    })
  )
}
