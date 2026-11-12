import { test } from '@playwright/test';
import net from 'node:net';
import crypto from 'node:crypto';
import { FACE_CAPTURE_API, JWT } from './config.js';

const TRACE = [];

function rawHandshake(urlPath, withCookie, label) {
  const origin = new globalThis.URL(FACE_CAPTURE_API);
  const port = Number(origin.port || 80);
  const key = crypto.randomBytes(16).toString('base64');

  return new Promise((resolve, reject) => {
    const t0 = Date.now();
    const sock = net.connect({ host: origin.hostname, port });
    let buffer = Buffer.alloc(0);
    let headEnded = false;
    let statusLine = '';
    let closed = false;
    let settled = false;
    let timer = null;

    const settle = (reason) => {
      if (settled) return;
      settled = true;
      if (timer) clearTimeout(timer);
      const idx = buffer.indexOf('\r\n\r\n');
      TRACE.push(
        `${label} settle=${reason}@${Date.now() - t0}ms status=${JSON.stringify(statusLine)} bytes=${buffer.length} closed=${closed}\n` +
          (idx >= 0 ? buffer.subarray(0, idx).toString('latin1') : '(no header)')
      );
      try {
        sock.destroy();
      } catch {}
      resolve({ statusLine, closed, buffer });
    };

    timer = setTimeout(() => settle('hold'), 1500);
    sock.setTimeout(4500, () => settle('timeout'));
    sock.on('connect', () => {
      TRACE.push(`${label} connect@${Date.now() - t0}ms`);
      sock.write(
        `GET ${urlPath} HTTP/1.1\r\nHost: ${origin.host}\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n` +
          `Sec-WebSocket-Key: ${key}\r\nSec-WebSocket-Version: 13\r\n` +
          (withCookie ? `Cookie: access_token_cookie=${JWT}\r\n` : '') +
          '\r\n'
      );
    });
    sock.on('data', (chunk) => {
      TRACE.push(`${label} data(${chunk.length})`);
      buffer = Buffer.concat([buffer, chunk]);
      if (headEnded) return;
      const idx = buffer.indexOf('\r\n\r\n');
      if (idx < 0) return;
      headEnded = true;
      statusLine = buffer.subarray(0, idx).toString('latin1').split('\r\n')[0];
    });
    sock.on('close', () => {
      closed = true;
      settle('close');
    });
    sock.on('error', (err) => {
      TRACE.push(`${label} error(${err.message})`);
      if (!settled) {
        if (timer) clearTimeout(timer);
        settle('error');
        if (!statusLine) reject(err);
      }
    });
  });
}

test('ws detail probe', async () => {
  const base = `${FACE_CAPTURE_API}/stream`;
  for (const [label, withCookie] of [
    ['no-cookie', false],
    ['cookie', true],
    ['no-cookie-2', false],
    ['cookie-2', true],
  ]) {
    try {
      await rawHandshake(`${base}/00000000-0000-0000-0000-000000000003`, withCookie, label);
    } catch (e) {
      TRACE.push(`${label} rejected ${e.message}`);
    }
  }
  console.log(TRACE.join('\n'));
});
