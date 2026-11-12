import net from 'node:net';
import crypto from 'node:crypto';

const WS = '/api/doodle/local/face_capture/stream/00000000-0000-0000-0000-000000000000';
const origin = { host: '127.0.0.1', port: 50024 };

const sock = net.connect({ host: origin.host, port: origin.port });
let buf = Buffer.alloc(0);
let statusLine = '';
let frames = [];

sock.on('connect', () => {
  console.log(`[${new Date().toLocaleTimeString()}] 已连接，发送 Upgrade`);
  sock.write(
    `GET ${WS} HTTP/1.1\r\nHost: ${origin.host}:${origin.port}\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n` +
      `Sec-WebSocket-Key: ${crypto.randomBytes(16).toString('base64')}\r\nSec-WebSocket-Version: 13\r\n\r\n`
  );
});

sock.on('data', (chunk) => {
  buf = Buffer.concat([buf, chunk]);
  if (!statusLine) {
    const i = buf.indexOf('\r\n\r\n');
    if (i < 0) return;
    statusLine = buf.subarray(0, i).toString('latin1').split('\r\n')[0];
    console.log(`[${new Date().toLocaleTimeString()}] 响应: ${statusLine}`);
    buf = buf.subarray(i + 4);
  }
  let off = 0;
  while (buf.length - off >= 2) {
    const b0 = buf[off], b1 = buf[off + 1];
    const opcode = b0 & 0x0f, masked = (b1 & 0x80) !== 0;
    let len = b1 & 0x7f, p = off + 2;
    if (len === 126) { if (buf.length - p < 2) break; len = buf.readUInt16BE(p); p += 2; }
    else if (len === 127) { if (buf.length - p < 8) break; len = Number(buf.readBigUInt64BE(p)); p += 8; }
    const mask = masked ? buf.subarray(p, p + 4) : null;
    if (masked) p += 4;
    if (buf.length - p < len) break;
    const payload = Buffer.from(buf.subarray(p, p + len));
    if (mask) for (let k = 0; k < payload.length; k++) payload[k] ^= mask[k & 3];
    frames.push({ opcode, len, payload });
    off = p + len;
    const tag = opcode === 1 ? 'TEXT' : opcode === 2 ? 'BIN ' : `OP${opcode}`;
    const extra = opcode === 1 ? payload.toString('utf8') : payload.subarray(0, 12).toString('hex');
    console.log(`[${new Date().toLocaleTimeString()}] 帧 #${frames.length} ${tag} len=${len} ${extra}`);
  }
  buf = buf.subarray(off);
});

sock.on('close', () => {
  console.log(`[${new Date().toLocaleTimeString()}] 连接关闭，共收到 ${frames.length} 帧`);
  process.exit(0);
});
sock.on('error', (e) => { console.log('ERR', e.message); process.exit(1); });

const holdMs = Number(process.argv[2] || 60000);
console.log(`保持 ${holdMs} ms ...`);
setTimeout(() => {
  console.log(`[${new Date().toLocaleTimeString()}] 超时结束，共收到 ${frames.length} 帧`);
  sock.destroy();
  process.exit(0);
}, holdMs);
