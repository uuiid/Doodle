import { expect } from 'chai';

export async function readJson(response) {
  const status = response.status();
  const text = await response.text();
  let body = null;
  if (text) {
    try {
      body = JSON.parse(text);
    } catch {
      body = text;
    }
  }
  return { status, headers: response.headers(), body, text };
}

export async function readBuffer(response) {
  const status = response.status();
  const headers = response.headers();
  const buffer = await response.body();
  return { status, headers, body: buffer, buffer };
}

export function expectError(res, status, fragment) {
  expect(res.status, `期望 ${status}，实际 ${res.status}：${JSON.stringify(res.body)}`).to.equal(status);
  if (fragment) {
    const text = JSON.stringify(res.body ?? '');
    expect(text, `错误消息应包含「${fragment}」，实际：${text}`).to.include(fragment);
  }
}

export async function waitFor(fn, predicate, timeoutMs = 40000, intervalMs = 1500) {
  const deadline = Date.now() + timeoutMs;
  let last = null;
  while (Date.now() < deadline) {
    last = await fn();
    if (predicate(last)) return last;
    await new Promise((resolve) => setTimeout(resolve, intervalMs));
  }
  return last;
}

export function pngSignatureHex(buffer) {
  return Buffer.from(buffer).subarray(0, 8).toString('hex');
}
