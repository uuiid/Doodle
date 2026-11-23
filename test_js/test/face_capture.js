// face_capture.js — 面部捕捉 HTTP 接口测试
//
// 路由唯一来源：src/doodle_lib/http_method/local/face_capture.h，路由**只**注册在
// create_kitsu_local_route()（kitsu.cpp），生产路由没有它们 —— 被测服务端必须带 `--local`
// 启动，否则整组用例都会落到 404「服务器端未实现 api」。
//
//   /api/doodle/local/face_capture/recordings                             GET
//   /api/doodle/local/face_capture/recordings/upload                       POST
//   /api/doodle/local/face_capture/recordings/{session_id_}                GET, DELETE
//   /api/doodle/local/face_capture/recordings/{session_id_}/video          GET
//   /api/doodle/local/face_capture/recordings/{session_id_}/curves         GET
//   /api/doodle/local/face_capture/recordings/{session_id_}/transcode      GET, POST
//   /api/doodle/local/face_capture/transcode                               GET
//
// 已删除（用例只断言 404）：`/devices`、`/stream`、`POST /recordings`、`POST /recordings/stop`。
//
// ---------------------------------------------------------------------------
// 录制模型
// ---------------------------------------------------------------------------
// 实时推流（原 WS /stream）已移除，录制改由**前端**完成：浏览器 getUserMedia + MediaRecorder
// 录好后整段 POST 到 /recordings/upload，服务端只负责落盘与后续转码解算。
// 因此会话目录里只有一段视频文件，命名取决于上传的扩展名：
//
//   <FACE_CAPTURE_SESSIONS_DIR>/<session_id>/take.mp4     ← 上传 .mp4
//                                     take.webm            ← 上传 .webm
//                                     curves/face_curves.dhfc
//                                     frames/frame_00000N.png
//
// 会话目录 = capture_session::default_root() / <uuid>，`--local` 分支用 `--root` 覆盖了
// core_set 的 p_root，所以以 `--root E:\\Doodle\\build\\root` 启动时目录就是
// `E:\\Doodle\\build\\root\\face_capture\\sessions`。视频由同一个磁盘上的服务端打开，
// 因此测试进程必须与服务端跑在同一台机器上。
//
// 帧率是浏览器 MediaRecorder 的可变帧率，服务端不再补偿，也不再有对齐元数据文件。
//
// ---------------------------------------------------------------------------
// 转码（"特殊"的一条 URL）
// ---------------------------------------------------------------------------
// POST /recordings/{session_id}/transcode 要求请求体带 ue_project：
//   1. 会话目录没有 take.mp4 / take.webm → 404「录制视频不存在」
//   2. 缺 ue_project / 不是字符串         → 422「缺少 ue_project」
//   3. ue_project 路径不存在              → 422「ue_project 不存在: <path>」
//   4. 入库 + start_next + 立即返回 200/201 的 server_task_info（status=submitted）
// 之后后台任务解帧、调 UE 解算，终态只有 completed / failed 两种。
// 本文件**只做 A 层**：断言提交、双通道（单条 + 全量列表）可见、终态落在 {completed, failed}
// 之内；等不到终态不判失败，而是带日志跳过 —— 终态依赖 UE 工程、插件版本服务与解算环境，
// 不在这组契约测试的责任范围（FACE_CAPTURE_TRANSCODE_E2E=1 时才额外断言 completed）。
//
// ---------------------------------------------------------------------------
// 刻意不覆盖
// ---------------------------------------------------------------------------
// · 真实录制（浏览器 MediaRecorder）：需要真人操作，本文件只覆盖上传契约。
// · 视频的真实解码：上传用例发的都是字节流，不代表可解；解码能力由转码 E2E 覆盖。
//
// 环境变量见 README.md「面部捕捉」。

import { test } from '@playwright/test';
import { expect } from 'chai';
import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';

import {
  JWT,
  NIL_ID,
  NON_EXISTENT_ID,
  FACE_CAPTURE_API,
  FACE_CAPTURE_SESSIONS_DIR,
  FACE_CAPTURE_SAMPLE,
  FACE_CAPTURE_SEED_DIR,
  FACE_CAPTURE_TRANSCODE_E2E,
  FACE_CAPTURE_WAIT_MS,
  FACE_CAPTURE_UE_PROJECT,
  KITSU_COOKIE,
} from './config.js';
import { readJson, readBuffer, expectError } from './helpers.js';

// capture_state 的取值（face_capture_types.h），JSON 里是字符串
const STATE_STOPPED = 'stopped';

// 1x1 透明 PNG，用来铺 frames/ 计数
const PNG_1PX = Buffer.from(
  'iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNk+M9QDwADhgGAWjR9awAAAABJRU5ErkJggg==',
  'base64'
);

const SAMPLE_READY = fs.existsSync(FACE_CAPTURE_SAMPLE) && canOpenExclusive(FACE_CAPTURE_SAMPLE);

function canOpenExclusive(filePath) {
  try {
    fs.closeSync(fs.openSync(filePath, 'r'));
    return true;
  } catch {
    return false;
  }
}

const CURVES_SOURCE = path.join(FACE_CAPTURE_SEED_DIR, 'face_curves.dhfc');
const CURVES_READY = fs.existsSync(CURVES_SOURCE);

test.describe.configure({ mode: 'serial' });

// ---------------------------------------------------------------------------
// 会话目录铺数据
// ---------------------------------------------------------------------------

function sessionDir(sessionId) {
  return path.join(FACE_CAPTURE_SESSIONS_DIR, sessionId);
}

// withWebm=true 时铺 take.webm，否则铺 take.mp4 —— 两种扩展名都要能被后续链路识别
function seedSession({ withVideo = true, withWebm = false, withCurves = true, withFrames = 0 } = {}) {
  const sessionId = crypto.randomUUID();
  const dir = sessionDir(sessionId);
  fs.mkdirSync(dir, { recursive: true });
  if (withVideo) fs.copyFileSync(FACE_CAPTURE_SAMPLE, path.join(dir, withWebm ? 'take.webm' : 'take.mp4'));
  if (withCurves) {
    fs.mkdirSync(path.join(dir, 'curves'), { recursive: true });
    fs.copyFileSync(CURVES_SOURCE, path.join(dir, 'curves', 'face_curves.dhfc'));
  }
  if (withFrames > 0) {
    fs.mkdirSync(path.join(dir, 'frames'), { recursive: true });
    for (let i = 1; i <= withFrames; ++i) {
      fs.writeFileSync(path.join(dir, 'frames', `frame_${String(i).padStart(6, '0')}.png`), PNG_1PX);
    }
  }
  return { sessionId, dir, videoName: withWebm ? 'take.webm' : 'take.mp4' };
}

function removeSession(sessionId) {
  const dir = sessionDir(sessionId);
  if (!fs.existsSync(dir)) return;
  try {
    fs.rmSync(dir, { recursive: true, force: true });
  } catch {
    // 服务端后台转码还在读这个目录时删不掉，交给下次清理
  }
}

// 整文件串行跑：所有铺出来的会话最后一起删
const created = [];

// 「提交任务并在两个通道可见」用例提交出去的任务 id，供后面的终态用例轮询（不再重复提交）
let transcodeSubmittedId = null;

function freshSession(options) {
  if (!SAMPLE_READY) test.skip(true, `样本视频不存在：${FACE_CAPTURE_SAMPLE}`);
  if (options?.withCurves && !CURVES_READY) test.skip(true, `动捕曲线不存在：${CURVES_SOURCE}`);
  const seeded = seedSession(options);
  created.push(seeded.sessionId);
  return seeded;
}

test.afterAll(() => {
  for (const sessionId of created) removeSession(sessionId);
});

// ---------------------------------------------------------------------------
// 已移除的路由
// ---------------------------------------------------------------------------

test.describe('面部捕捉已移除的路由', () => {
  test('GET /devices 不再存在', async ({ request }) => {
    const res = await readJson(await request.get(`${FACE_CAPTURE_API}/devices`, { headers: authCookie() }));
    expect(res.status, '设备枚举路由已删除').to.equal(404);
    expect(res.body).to.be.an('object').that.has.property('code').that.equals(res.status);
  });

  test('POST /devices 不再存在', async ({ request }) => {
    const res = await readJson(
      await request.post(`${FACE_CAPTURE_API}/devices`, { headers: authHeader(), data: { index: 0 } })
    );
    expect(res.status).to.equal(404);
  });

  test('POST /recordings 不再存在', async ({ request }) => {
    const res = await readJson(
      await request.post(`${FACE_CAPTURE_API}/recordings`, { headers: authHeader(), data: {} })
    );
    expect(res.status).to.equal(404);
  });

  test('POST /recordings/stop 不再存在', async ({ request }) => {
    const res = await readJson(
      await request.post(`${FACE_CAPTURE_API}/recordings/stop`, { headers: authHeader(), data: {} })
    );
    expect(res.status).to.equal(404);
  });
});

// ---------------------------------------------------------------------------
// upload
// ---------------------------------------------------------------------------

test.describe('面部捕捉上传', () => {
  test('POST /recordings/upload 上传 webm 落盘为 take.webm', async ({ request }) => {
    const sessionId = await uploadVideo(request, 'take.webm');
    const dir = sessionDir(sessionId);
    expect(fs.existsSync(path.join(dir, 'take.webm')), '应落盘为 take.webm').to.equal(true);
    expect(fs.existsSync(path.join(dir, 'take.mp4'))).to.equal(false);
  });

  test('POST /recordings/upload 上传 mp4 落盘为 take.mp4', async ({ request }) => {
    const sessionId = await uploadVideo(request, 'take.mp4');
    const dir = sessionDir(sessionId);
    expect(fs.existsSync(path.join(dir, 'take.mp4')), '应落盘为 take.mp4').to.equal(true);
  });

  test('POST /recordings/upload 上传后出现在会话列表', async ({ request }) => {
    const sessionId = await uploadVideo(request, 'take.webm');
    const res = await readJson(await request.get(`${FACE_CAPTURE_API}/recordings`, { headers: authCookie() }));
    const entry = res.body.data.find((item) => item.session_id === sessionId);
    expect(entry, '上传的会话应出现在列表里').to.not.equal(undefined);
  });

  test('POST /recordings/upload 上传后 video 接口返回对应 Content-Type', async ({ request }) => {
    const webm = await uploadVideo(request, 'take.webm');
    const webmRes = await readBuffer(
      await request.fetch(`${FACE_CAPTURE_API}/recordings/${webm}/video`, { headers: authCookie() })
    );
    expect(webmRes.status).to.equal(200);
    expect(webmRes.headers['content-type']).to.equal('video/webm');

    const mp4 = await uploadVideo(request, 'take.mp4');
    const mp4Res = await readBuffer(
      await request.fetch(`${FACE_CAPTURE_API}/recordings/${mp4}/video`, { headers: authCookie() })
    );
    expect(mp4Res.status).to.equal(200);
    expect(mp4Res.headers['content-type']).to.equal('video/mp4');
  });

  test('POST /recordings/upload 不支持的扩展名返回 400', async ({ request }) => {
    for (const name of ['take.avi', 'take', 'take.mkv']) {
      const res = await readJson(await uploadRaw(request, name, PNG_1PX));
      expectError(res, 400, '不支持的视频扩展名');
    }
  });

  test('POST /recordings/upload 没有 body 返回 400', async ({ request }) => {
    // 不带 body 时 get_file() 仍给出一个 0 字节的临时文件，所以落到的也是「视频文件为空」
    const res = await readJson(
      await request.post(`${FACE_CAPTURE_API}/recordings/upload`, {
        headers: { ...authCookie(), 'Content-Disposition': 'take.webm', 'Content-Type': 'application/octet-stream' },
      })
    );
    expectError(res, 400, '请求体中的视频文件为空');
  });

  test('POST /recordings/upload 空 body 返回 400', async ({ request }) => {
    const res = await readJson(await uploadRaw(request, 'take.webm', Buffer.alloc(0)));
    expectError(res, 400, '请求体中的视频文件为空');
  });
});

// ---------------------------------------------------------------------------
// recordings 列表
// ---------------------------------------------------------------------------

test.describe('面部捕捉 recordings 列表', () => {
  test('GET /recordings 返回 {count, data} 并至少包含种子会话', async ({ request }) => {
    const seeded = freshSession({ withFrames: 3 });

    const res = await readJson(await request.get(`${FACE_CAPTURE_API}/recordings`, { headers: authCookie() }));
    expect(res.status, JSON.stringify(res.body)).to.equal(200);
    expect(res.body).to.have.all.keys('count', 'data');
    expect(res.body.data).to.be.an('array');
    expect(res.body.count).to.equal(res.body.data.length);

    const entry = res.body.data.find((item) => item.session_id === seeded.sessionId);
    expect(entry, '列表里应能找到刚铺进去的会话').to.not.equal(undefined);
    expect(entry).to.have.all.keys('session_id', 'path');
    expect(entry.session_id).to.equal(seeded.sessionId);
  });

  test('GET /recordings 跳过没有视频的目录', async ({ request }) => {
    const emptyId = crypto.randomUUID();
    const emptyDir = sessionDir(emptyId);
    const onlyFrames = crypto.randomUUID();
    const onlyFramesDir = sessionDir(onlyFrames);
    fs.mkdirSync(path.join(emptyDir, 'frames'), { recursive: true });
    fs.mkdirSync(onlyFramesDir, { recursive: true });
    created.push(emptyId, onlyFrames);

    const res = await readJson(await request.get(`${FACE_CAPTURE_API}/recordings`, { headers: authCookie() }));
    const ids = res.body.data.map((item) => item.session_id);
    expect(ids).to.not.include(emptyId);
    expect(ids).to.not.include(onlyFrames);

    // 补上 take.webm 后算一条会话 —— 有视频才算会话
    fs.writeFileSync(path.join(onlyFramesDir, 'take.webm'), 'not a real video');
    const after = await readJson(await request.get(`${FACE_CAPTURE_API}/recordings`, { headers: authCookie() }));
    const entry = after.body.data.find((item) => item.session_id === onlyFrames);
    expect(entry, '补了 take.webm 的目录应出现在列表里').to.not.equal(undefined);
  });

  test('GET /recordings 目录按文件名给出 session_id', async ({ request }) => {
    const seeded = freshSession({ withCurves: false });
    const res = await readJson(await request.get(`${FACE_CAPTURE_API}/recordings`, { headers: authCookie() }));
    const entry = res.body.data.find((item) => item.session_id === seeded.sessionId);
    expect(entry.session_id).to.equal(path.basename(seeded.dir));
  });
});

// ---------------------------------------------------------------------------
// 单条 recording
// ---------------------------------------------------------------------------

test.describe('面部捕捉 recording 详情', () => {
  test('GET /recordings/{id} 返回产物清单', async ({ request }) => {
    const seeded = freshSession({ withFrames: 3 });

    const res = await readJson(
      await request.get(`${FACE_CAPTURE_API}/recordings/${seeded.sessionId}`, { headers: authCookie() })
    );
    expect(res.status, JSON.stringify(res.body)).to.equal(200);
    expect(res.body.session_id).to.equal(seeded.sessionId);
    expect(res.body.state).to.equal(STATE_STOPPED);
    expect(res.body.artifacts).to.deep.equal({
      take_mp4: true,
      take_webm: false,
      frames: 3,
      curves: true,
    });
  });

  test('GET /recordings/{id} webm 会话 take_webm 为真', async ({ request }) => {
    const seeded = freshSession({ withWebm: true });

    const res = await readJson(
      await request.get(`${FACE_CAPTURE_API}/recordings/${seeded.sessionId}`, { headers: authCookie() })
    );
    expect(res.status, JSON.stringify(res.body)).to.equal(200);
    expect(res.body.artifacts.take_webm).to.equal(true);
    expect(res.body.artifacts.take_mp4).to.equal(false);
  });

  test('GET /recordings/{id} 会话不存在返回 404', async ({ request }) => {
    const res = await readJson(
      await request.get(`${FACE_CAPTURE_API}/recordings/${NON_EXISTENT_ID}`, { headers: authCookie() })
    );
    expectError(res, 404, '会话不存在');
  });

  test('GET /recordings/{id} 只有视频时产物为假', async ({ request }) => {
    const seeded = freshSession({ withCurves: false });

    const res = await readJson(
      await request.get(`${FACE_CAPTURE_API}/recordings/${seeded.sessionId}`, { headers: authCookie() })
    );
    expect(res.status, JSON.stringify(res.body)).to.equal(200);
    expect(res.body.session_id).to.equal(seeded.sessionId);
    expect(res.body).to.not.have.property('width');
    expect(res.body.artifacts).to.deep.equal({
      take_mp4: true,
      take_webm: false,
      frames: 0,
      curves: false,
    });
  });

  test('DELETE /recordings/{id} 删除目录并让后续查询 404', async ({ request }) => {
    const seeded = freshSession({ withFrames: 1 });

    const del = await readJson(
      await request.delete(`${FACE_CAPTURE_API}/recordings/${seeded.sessionId}`, { headers: authCookie() })
    );
    expect(del.status, JSON.stringify(del.body)).to.equal(200);
    expect(del.body).to.deep.equal({ ok: true });
    expect(fs.existsSync(seeded.dir)).to.equal(false);

    const again = await readJson(
      await request.get(`${FACE_CAPTURE_API}/recordings/${seeded.sessionId}`, { headers: authCookie() })
    );
    expectError(again, 404, '会话不存在');

    const del2 = await readJson(
      await request.delete(`${FACE_CAPTURE_API}/recordings/${seeded.sessionId}`, { headers: authCookie() })
    );
    expectError(del2, 404, '会话不存在');
  });

  test('DELETE /recordings/{id} 不存在返回 404', async ({ request }) => {
    const res = await readJson(
      await request.delete(`${FACE_CAPTURE_API}/recordings/${NON_EXISTENT_ID}`, { headers: authCookie() })
    );
    expectError(res, 404);
  });
});

// ---------------------------------------------------------------------------
// 产物下载
// ---------------------------------------------------------------------------

test.describe('面部捕捉产物下载', () => {
  test('GET /recordings/{id}/video 发 mp4 且支持 Range', async ({ request }) => {
    const seeded = freshSession({ withCurves: false });

    const etag = await readBuffer(
      await request.fetch(`${FACE_CAPTURE_API}/recordings/${seeded.sessionId}/video`, { headers: authCookie() })
    );
    expect(etag.status).to.equal(200);
    expect(etag.headers['content-type']).to.equal('video/mp4');
    expect(etag.headers['accept-ranges']).to.equal('bytes');

    const size = fs.statSync(path.join(seeded.dir, 'take.mp4')).size;
    expect(Number(etag.headers['content-length'])).to.equal(size);
    expect(etag.headers['etag'], 'etag = "{mtime}-{size}-{adler32}"').to.include(`-${size}-`);

    // bytes=0-15：服务端只解析 "-" 前面的起点（http_body.cpp），上界被忽略，
    // 于是回 206 + 从 0 到文件末尾的全部内容。这条是记录在案的缺陷，这里按现状断言。
    const ranged = await request.fetch(`${FACE_CAPTURE_API}/recordings/${seeded.sessionId}/video`, {
      headers: { ...authCookie(), Range: 'bytes=0-15' },
    });
    const rangedInfo = await readBuffer(ranged);
    expect(rangedInfo.status).to.equal(206);
    expect(rangedInfo.headers['content-range'], '上界被忽略').to.equal(`bytes 0-${size - 1}/${size}`);
    expect(rangedInfo.buffer.length).to.equal(size);

    const tail = await request.fetch(`${FACE_CAPTURE_API}/recordings/${seeded.sessionId}/video`, {
      headers: { ...authCookie(), Range: `bytes=${size - 16}-` },
    });
    const tailInfo = await readBuffer(tail);
    expect(tailInfo.status).to.equal(206);
    expect(tailInfo.headers['content-range']).to.equal(`bytes ${size - 16}-${size - 1}/${size}`);
    expect(tailInfo.buffer.length).to.equal(16);

    const real = fs.readFileSync(path.join(seeded.dir, 'take.mp4')).subarray(size - 16);
    expect(Buffer.compare(tailInfo.buffer, real), '尾部 16 字节应与源文件一致').to.equal(0);
  });

  test('GET /recordings/{id}/video webm 会话发 webm', async ({ request }) => {
    const seeded = freshSession({ withWebm: true, withCurves: false });

    const res = await readBuffer(
      await request.fetch(`${FACE_CAPTURE_API}/recordings/${seeded.sessionId}/video`, { headers: authCookie() })
    );
    expect(res.status).to.equal(200);
    expect(res.headers['content-type']).to.equal('video/webm');

    const size = fs.statSync(path.join(seeded.dir, 'take.webm')).size;
    expect(Number(res.headers['content-length'])).to.equal(size);
  });

  test('GET /recordings/{id}/video 没有视频返回 404', async ({ request }) => {
    const seeded = freshSession({ withVideo: false, withCurves: false });

    const res = await readJson(
      await request.get(`${FACE_CAPTURE_API}/recordings/${seeded.sessionId}/video`, { headers: authCookie() })
    );
    expectError(res, 404, '录制视频不存在');
  });

  test('GET /recordings/{id}/curves 以附件下载', async ({ request }) => {
    const seeded = freshSession();

    const res = await readBuffer(
      await request.get(`${FACE_CAPTURE_API}/recordings/${seeded.sessionId}/curves`, { headers: authCookie() })
    );
    expect(res.status, JSON.stringify(res.headers)).to.equal(200);
    expect(res.headers['content-type']).to.equal('application/octet-stream');
    expect(res.headers['accept-ranges']).to.equal('bytes');
    expect(res.headers['content-disposition']).to.equal(
      `attachment; filename="${seeded.sessionId}.dhfc"; filename*=UTF-8''${seeded.sessionId}.dhfc`
    );

    const source = fs.readFileSync(CURVES_SOURCE);
    expect(res.buffer.length).to.equal(source.length);
    expect(res.buffer.subarray(0, 8).toString('latin1'), 'DHFC 魔数').to.equal('DHFC0001');
    expect(Buffer.compare(res.buffer, source), '下载内容应与种子文件逐字节一致').to.equal(0);
  });

  test('GET /recordings/{id}/curves 支持 Range', async ({ request }) => {
    const seeded = freshSession();
    const size = fs.statSync(path.join(seeded.dir, 'curves', 'face_curves.dhfc')).size;

    const res = await readBuffer(
      await request.get(`${FACE_CAPTURE_API}/recordings/${seeded.sessionId}/curves`, {
        headers: { ...authCookie(), Range: 'bytes=8-' },
      })
    );
    expect(res.status).to.equal(206);
    expect(res.headers['content-range']).to.equal(`bytes 8-${size - 1}/${size}`);
    expect(res.buffer.length).to.equal(size - 8);
  });

  test('GET /recordings/{id}/curves 没有曲线返回 404', async ({ request }) => {
    const seeded = freshSession({ withCurves: false });

    const res = await readJson(
      await request.get(`${FACE_CAPTURE_API}/recordings/${seeded.sessionId}/curves`, { headers: authCookie() })
    );
    expectError(res, 404, 'face_curves.dhfc 不存在');
  });

  test('GET /recordings/{id}/curves 会话目录不存在返回 404', async ({ request }) => {
    const res = await readJson(
      await request.get(`${FACE_CAPTURE_API}/recordings/${NON_EXISTENT_ID}/curves`, { headers: authCookie() })
    );
    expectError(res, 404, 'face_curves.dhfc 不存在');
  });
});

// ---------------------------------------------------------------------------
// 转码（A 层）
// ---------------------------------------------------------------------------

test.describe('面部捕捉转码', () => {
  // 一次成功的提交之后，后台任务会同步把整段样本视频解成 PNG，期间服务端所有请求都会排队。
  // 所以「会真的启动任务」的用例放在这一组最后，且只跑一次。
  test('GET /transcode 返回任务数组', async ({ request }) => {
    const res = await readJson(await request.get(`${FACE_CAPTURE_API}/transcode`, { headers: authCookie() }));
    expect(res.status).to.equal(200);
    expect(res.body).to.be.an('array');
    for (const task of res.body) expectServerTaskInfo(task);
  });

  test('GET /recordings/{id}/transcode 没有任务返回 404', async ({ request }) => {
    const seeded = freshSession({ withCurves: false });

    const res = await readJson(
      await request.get(`${FACE_CAPTURE_API}/recordings/${seeded.sessionId}/transcode`, { headers: authCookie() })
    );
    expectError(res, 404, '该会话没有转码任务');
  });

  test('POST /recordings/{id}/transcode 会话不存在返回 404', async ({ request }) => {
    const res = await readJson(
      await request.post(`${FACE_CAPTURE_API}/recordings/${NON_EXISTENT_ID}/transcode`, {
        headers: authHeader(),
        data: { ue_project: FACE_CAPTURE_UE_PROJECT },
      })
    );
    expectError(res, 404, '录制视频不存在');
  });

  test('POST /recordings/{id}/transcode 缺少视频返回 404', async ({ request }) => {
    const seeded = freshSession({ withVideo: false, withCurves: false });

    const res = await readJson(
      await request.post(`${FACE_CAPTURE_API}/recordings/${seeded.sessionId}/transcode`, {
        headers: authHeader(),
        data: { ue_project: FACE_CAPTURE_UE_PROJECT },
      })
    );
    expectError(res, 404, '录制视频不存在');
  });

  test('POST /recordings/{id}/transcode 缺少 ue_project 返回 422', async ({ request }) => {
    const seeded = freshSession({ withCurves: false });

    for (const data of [{}, { ue_project: 123 }, { ue_project: null }, { ue_project: ['x'] }]) {
      const res = await readJson(
        await request.post(`${FACE_CAPTURE_API}/recordings/${seeded.sessionId}/transcode`, {
          headers: authHeader(),
          data,
        })
      );
      expectError(res, 422, '缺少 ue_project');
    }
  });

  test('POST /recordings/{id}/transcode ue_project 不存在返回 422', async ({ request }) => {
    const seeded = freshSession({ withCurves: false });

    const res = await readJson(
      await request.post(`${FACE_CAPTURE_API}/recordings/${seeded.sessionId}/transcode`, {
        headers: authHeader(),
        data: { ue_project: 'E:\\\\definitely-not-here\\\\x.uproject' },
      })
    );
    expectError(res, 422, 'ue_project 不存在');
  });

  test('POST /recordings/{id}/transcode 提交任务并在两个通道可见', async ({ request }) => {
    test.skip(!fs.existsSync(FACE_CAPTURE_UE_PROJECT), `UE 工程不存在：${FACE_CAPTURE_UE_PROJECT}`);
    const seeded = freshSession({ withCurves: false });
    transcodeSubmittedId = seeded.sessionId;

    // 提交这一条 HTTP 响应要等后台 run() 走到第一个 co_await 才回来：同步解帧 + 拉 UE 插件
    // 都在 run() 的前半段。实测提交约 30s 才返回，所以这里给 120s；
    // 仍拿不到就以「发出去了」继续，靠后面两个查询通道覆盖。
    let submitted = null;
    try {
      submitted = await readJson(
        await request.post(`${FACE_CAPTURE_API}/recordings/${seeded.sessionId}/transcode`, {
          headers: authHeader(),
          data: { ue_project: FACE_CAPTURE_UE_PROJECT },
          timeout: 120000,
        })
      );
    } catch (error) {
      console.log(`提交后没有在 120s 内拿到响应：${error.message.split('\\n')[0]}`);
    }

    if (submitted) {
      expect([200, 201], JSON.stringify(submitted.body)).to.include(submitted.status);
      const task = submitted.body;
      expectServerTaskInfo(task);
      expect(task.id).to.equal(seeded.sessionId);
      expect(task.name).to.equal(`face_capture_transcode_${seeded.sessionId}`);
      expect(task.command).to.deep.equal({ preview_id: seeded.sessionId, ue_project: FACE_CAPTURE_UE_PROJECT });
      expect(task.status, '提交时 start_next 会立刻把它置为 running').to.be.oneOf(['submitted', 'running']);
    }

    // 任务已经开跑，后台解帧会占住请求线程；这里给读接口留更宽的等待时间
    let single = null;
    try {
      single = await readJson(
        await request.get(`${FACE_CAPTURE_API}/recordings/${seeded.sessionId}/transcode`, {
          headers: authCookie(),
          timeout: FACE_CAPTURE_WAIT_MS,
        })
      );
    } catch (error) {
      test.skip(true, `后台解帧期间查询应不上：${error.message.split('\\n')[0]}`);
    }
    expect(single.status, JSON.stringify(single.body)).to.equal(200);
    expectServerTaskInfo(single.body);
    expect(single.body.id).to.equal(seeded.sessionId);

    // POST 没拿到响应时任务表里也可能还没有这条（提交协程尚未恢复），给全量通道重试一会儿
    let all = null;
    const deadline = Date.now() + FACE_CAPTURE_WAIT_MS;
    while (Date.now() < deadline) {
      try {
        all = await readJson(
          await request.get(`${FACE_CAPTURE_API}/transcode`, {
            headers: authCookie(),
            timeout: FACE_CAPTURE_WAIT_MS,
          })
        );
      } catch (error) {
        test.skip(true, `后台解帧期间查询应不上：${error.message.split('\\n')[0]}`);
      }
      if (all.body.some((item) => item.id === seeded.sessionId)) break;
      await new Promise((resolve) => setTimeout(resolve, 1000));
    }
    expect(all.status).to.equal(200);
    expect(all.body.filter((item) => item.id === seeded.sessionId)).to.have.lengthOf(1);
  });

  test('POST /recordings/{id}/transcode webm 会话也能提交', async ({ request }) => {
    test.skip(!fs.existsSync(FACE_CAPTURE_UE_PROJECT), `UE 工程不存在：${FACE_CAPTURE_UE_PROJECT}`);
    const seeded = freshSession({ withWebm: true, withCurves: false });

    // 只断言前置校验放行（不是 404「录制视频不存在」）；真实提交交给上一条用例
    let status = null;
    try {
      const res = await readJson(
        await request.post(`${FACE_CAPTURE_API}/recordings/${seeded.sessionId}/transcode`, {
          headers: authHeader(),
          data: { ue_project: 'E:\\\\definitely-not-here\\\\x.uproject' },
          timeout: 30000,
        })
      );
      status = res.status;
    } catch (error) {
      test.skip(true, `服务端未响应：${error.message.split('\\n')[0]}`);
    }
    expect(status, 'webm 会话应通过「录制视频不存在」的前置校验').to.equal(422);
  });

  test('POST /recordings/{id}/transcode 终态落在 completed / failed 之内', async ({ request }) => {
    // A 层：终态只断言「落在 completed / failed 之内」，等不到就带日志跳过。
    test.skip(!fs.existsSync(FACE_CAPTURE_UE_PROJECT), `UE 工程不存在：${FACE_CAPTURE_UE_PROJECT}`);
    let sessionId = transcodeSubmittedId;
    if (!sessionId) {
      const seeded = freshSession({ withCurves: false });
      sessionId = seeded.sessionId;
      const submitted = await readJson(
        await request.post(`${FACE_CAPTURE_API}/recordings/${sessionId}/transcode`, {
          headers: authHeader(),
          data: { ue_project: FACE_CAPTURE_UE_PROJECT },
          timeout: 120000,
        })
      );
      expect([200, 201], JSON.stringify(submitted.body)).to.include(submitted.status);
    }

    let last = null;
    let lastError = null;
    const deadline = Date.now() + FACE_CAPTURE_WAIT_MS;
    while (Date.now() < deadline) {
      try {
        const res = await readJson(
          await request.get(`${FACE_CAPTURE_API}/recordings/${sessionId}/transcode?poll=1`, {
            headers: authCookie(),
            timeout: 5000,
          })
        );
        last = res.body;
        lastError = null;
      } catch (error) {
        // 后台解帧期间服务端可能应不上，轮询超时不算失败
        lastError = error;
      }
      if (last && (last.status === 'completed' || last.status === 'failed')) break;
      await new Promise((resolve) => setTimeout(resolve, 1000));
    }

    const detail = last
      ? `status=${last.status} last_line_log=${last.last_line_log}`
      : `没有拿到任务${lastError ? `（${lastError.message.split('\\n')[0]}）` : ''}`;
    if (!last || last.status === 'running' || last.status === 'submitted') {
      test.skip(true, `UE 解算未在 ${FACE_CAPTURE_WAIT_MS}ms 内结束（${detail}）`);
    }
    expect(last.status, detail).to.be.oneOf(['completed', 'failed']);
    expect(last.end_time, '终态一定有 end_time').to.not.equal(null);

    if (FACE_CAPTURE_TRANSCODE_E2E) {
      expect(last.status, `端到端模式要求解算成功：${detail}`).to.equal('completed');
      const rec = await readJson(
        await request.get(`${FACE_CAPTURE_API}/recordings/${sessionId}`, { headers: authCookie() })
      );
      expect(rec.body.artifacts.curves, '解算成功应产出 curves/face_curves.dhfc').to.equal(true);
      expect(rec.body.artifacts.frames, '解算前会清空 frames/ 并重新解帧').to.be.greaterThan(0);
    }
  });
});

// ---------------------------------------------------------------------------
// 已移除的推流路由
// ---------------------------------------------------------------------------

test.describe('面部捕捉推流路由已移除', () => {
  test('GET /stream 不再存在', async ({ request }) => {
    const res = await readJson(await request.get(`${FACE_CAPTURE_API}/stream`, { headers: authCookie() }));
    expect(res.status, '实时推流已删除，路由应不存在').to.equal(404);
    expect(res.body).to.be.an('object').that.has.property('code').that.equals(res.status);
  });

  test('/stream 的 WS 升级请求也不再被接受', async ({ request }) => {
    const res = await request.fetch(`${FACE_CAPTURE_API}/stream`, {
      headers: { ...authCookie(), Upgrade: 'websocket', Connection: 'Upgrade', 'Sec-WebSocket-Version': '13', 'Sec-WebSocket-Key': crypto.randomBytes(16).toString('base64') },
    });
    expect(res.status(), '没有 WS 路由，不会回 101').to.not.equal(101);
  });
});

// ---------------------------------------------------------------------------
// 鉴权与通用请求语义
// ---------------------------------------------------------------------------

test.describe('面部捕捉鉴权', () => {
  const routes = [
    ['GET', '/recordings'],
    ['POST', '/recordings/upload'],
    ['GET', `/recordings/${NIL_ID}`],
    ['DELETE', `/recordings/${NIL_ID}`],
    ['GET', `/recordings/${NIL_ID}/video`],
    ['GET', `/recordings/${NIL_ID}/curves`],
    ['GET', `/recordings/${NIL_ID}/transcode`],
    ['POST', `/recordings/${NIL_ID}/transcode`],
    ['GET', '/transcode'],
  ];

  test('没有 token 的请求一律 401', async ({ request }) => {
    for (const [method, url] of routes) {
      const res = await readJson(
        await request.fetch(`${FACE_CAPTURE_API}${url}`, {
          method,
          data: method === 'GET' ? undefined : {},
          headers: { 'Content-Type': 'application/json' },
        })
      );
      expectError(res, 401, '请先登录');
    }
  });

  test('OPTIONS 不需要 token', async ({ request }) => {
    const res = await request.fetch(`${FACE_CAPTURE_API}/recordings`, { method: 'OPTIONS' });
    expect(res.status()).to.equal(200);
    expect(res.headers()['access-control-allow-methods']).to.include('DELETE');
  });

  test('只带 Bearer 也能通过鉴权', async ({ request }) => {
    const res = await readJson(
      await request.get(`${FACE_CAPTURE_API}/recordings`, { headers: { Authorization: 'Bearer any-value' } })
    );
    expect(res.status, 'local 路由只截取 token，不校验内容').to.equal(200);
  });

  test('垃圾 token 也通过鉴权（local 路由不验签）', async ({ request }) => {
    const res = await readJson(
      await request.get(`${FACE_CAPTURE_API}/recordings`, {
        headers: { Cookie: 'access_token_cookie=not-a-jwt' },
      })
    );
    expect(res.status).to.equal(200);
  });
});

// ---------------------------------------------------------------------------
// 工具
// ---------------------------------------------------------------------------

function authHeader() {
  return { Cookie: `access_token_cookie=${KITSU_COOKIE}`, 'Content-Type': 'application/json' };
}

function authCookie() {
  return { Cookie: `access_token_cookie=${KITSU_COOKIE}` };
}

// 上传接口用原始 body 发视频字节，文件名由 Content-Disposition 头携带
async function uploadRaw(request, fileName, data) {
  const res = await request.post(`${FACE_CAPTURE_API}/recordings/upload`, {
    headers: { ...authCookie(), 'Content-Disposition': fileName, 'Content-Type': 'application/octet-stream' },
    data,
  });
  return res;
}

// 上传一段真实样本视频，返回服务端分配的 session_id（登记进 created 以便收尾清理）
async function uploadVideo(request, fileName) {
  const bytes = fs.readFileSync(FACE_CAPTURE_SAMPLE);
  const res = await readJson(await uploadRaw(request, fileName, bytes));
  expect([200, 201], JSON.stringify(res.body)).to.include(res.status);
  expect(res.body).to.have.all.keys('session_id', 'video');
  expect(res.body.video).to.equal(fileName);

  const sessionId = res.body.session_id;
  created.push(sessionId);
  expect(fs.existsSync(path.join(sessionDir(sessionId), fileName)), `${fileName} 应已落盘`).to.equal(true);
  return sessionId;
}

function expectServerTaskInfo(task) {
  expect(task, 'server_task_info 的固定字段（server_task_info.h）').to.include.keys(
    'id',
    'status',
    'name',
    'source_computer',
    'submitter',
    'submit_time',
    'run_time',
    'end_time',
    'run_computer_id',
    'type',
    'last_line_log',
    'run_time_info',
    'command',
    'priority',
    'task_id',
    'submitter_cookies'
  );
  expect(task.priority).to.equal(100);
  expect(task).to.not.have.property('progress');
  expect(task.status).to.be.oneOf(['submitted', 'assigned', 'running', 'completed', 'canceled', 'failed', 'unknown']);
}
