import net from 'node:net';
import crypto from 'node:crypto';
import { execSync } from 'node:child_process';

const API = 'http://127.0.0.1:50024/api/doodle/local/face_capture';
const WS = '/api/doodle/local/face_capture/stream/00000000-0000-0000-0000-000000000000';

function connectWs() {
  return new Promise((resolve, reject) => {
    const sock = net.connect({ host: '127.0.0.1', port: 50024 });
    const state = { sock, statusLine: '', framed: [], closed: false, buf: Buffer.alloc(0) };
    sock.on('connect', () => sock.write(
      `GET ${WS} HTTP/1.1\r\nHost: 127.0.0.1:50024\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n` +
      `Sec-WebSocket-Key: ${crypto.randomBytes(16).toString('base64')}\r\nSec-WebSocket-Version: 13\r\n\r\n`));
    sock.on('data', (chunk) => {
      state.buf = Buffer.concat([state.buf, chunk]);
      if (!state.statusLine) {
        const i = state.buf.indexOf('\r\n\r\n');
        if (i < 0) return;
        state.statusLine = state.buf.subarray(0, i).toString('latin1').split('\r\n')[0];
        state.buf = state.buf.subarray(i + 4);
        resolve(state);
      }
      let b = state.buf, frames = [], off = 0;
      while (b.length - off >= 2) {
        const b0 = b[off], b1 = b[off + 1];
        const opcode = b0 & 0x0f, masked = (b1 & 0x80) !== 0;
        let len = b1 & 0x7f, p = off + 2;
        if (len === 126) { if (b.length - p < 2) break; len = b.readUInt16BE(p); p += 2; }
        else if (len === 127) { if (b.length - p < 8) break; len = Number(b.readBigUInt64BE(p)); p += 8; }
        const mask = masked ? b.subarray(p, p + 4) : null;
        if (masked) p += 4;
        if (b.length - p < len) break;
        const payload = Buffer.from(b.subarray(p, p + len));
        if (mask) for (let k = 0; k < payload.length; k++) payload[k] ^= mask[k & 3];
        frames.push({ opcode, len, payload });
        off = p + len;
      }
      if (frames.length) { state.buf = b.subarray(off); state.framed.push(...frames); }
    });
    sock.on('close', () => { state.closed = true; if (!state.statusLine) reject(new Error('closed without response')); });
    sock.on('error', (e) => { if (!state.statusLine) reject(e); });
    sock.setTimeout(15000, () => { if (!state.statusLine) reject(new Error('timeout')); });
  });
}

for (let attempt = 0; attempt < 12; attempt++) {
  let ws;
  try { ws = await connectWs(); } catch (e) { console.log(`#${attempt} 连接失败: ${e.message}`); await new Promise(r => setTimeout(r, 500)); continue; }
  ws.sock.setTimeout(0);
  await new Promise(r => setTimeout(r, 2500));
  const text = ws.framed.filter(f => f.opcode === 1).map(f => f.payload.toString('utf8'));
  const bin = ws.framed.filter(f => f.opcode === 2);
  console.log(`#${attempt} ${ws.statusLine} | text=${text.length} binary=${bin.length} closed=${ws.closed}`);
  for (const t of text.slice(0, 3)) console.log('   TEXT', t.slice(0, 200));
  for (const b of bin.slice(0, 3)) console.log('   BIN len=' + b.len + ' head=' + b.payload.subarray(0, 16).toString('hex'));
  try { ws.sock.destroy(); } catch {}
  if (ws.framed.length) break;
}
