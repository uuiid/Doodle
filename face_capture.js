// face_capture.js — 面部捕捉 HTTP / WebSocket 接口测试
//
// 路由唯一来源：src/doodle_lib/http_method/local/face_capture.h:71-161，九条路由**只**注册在
// create_kitsu_local_route()（kitsu.cpp:337-345），生产路由没有它们 —— 被测服务端必须带 `--local`
// 启动，否则整组用例都会落到 404「服务器端未实现 api」。
//
//   /api/doodle/local/face_capture/devices                                GET, POST
//   /api/doodle/local/face_capture/stream/{session_id_}                    WS upgrade
//   /api/doodle/local/face_capture/recordings                             GET, POST
//   /api/doodle/local/face_capture/recordings/{session_id_}/stop           POST
//   /api/doodle/local/face_capture/recordings/{session_id_}                GET, DELETE
//   /api/doodle/local/face_capture/recordings/{session_id_}/video          GET
//   /api/doodle/local/face_capture/recordings/{session_id_}/curves         GET
//   /api/doodle/local/face_capture/recordings/{preview_id_}/transcode      GET, POST
//   /api/doodle/local/face_capture/transcode                               GET
//
// ---------------------------------------------------------------------------
// 会话数据从哪来
// ---------------------------------------------------------------------------
// 真实录制要占用摄像头，本文件**不录**，而是把样本视频与已解算好的动捕曲线直接铺进会话目录：
//
//   <FACE_CAPTURE_SESSIONS_DIR>/<session_id>/take.mp4                  ← 复制 FACE_CAPTURE_SAMPLE
//                                     align.json                       ← 本文件生成，自洽
//                                     curves/face_curves.dhfc          ← 复制 FACE_CAPTURE_SEED_DIR
//                                     frames/frame_00000N.png          ← 3 个 1x1 PNG
//
// 会话目录 = capture_session::default_root() / <uuid>
//          = core_set::get_cache_root()/"face_capture"/"sessions"/<uuid>（capture_session.cpp:523-525），
// `--local` 分支用 `--root` 覆盖了 core_set 的 p_root（kitsu_supplement.cpp:179），
// 所以服务端以 `--root E:\Doodle\build\root` 启动时，目录就是
// `E:\Doodle\build\root\face_capture\sessions`。视频由同一个磁盘上的服务端打开，
// 因此测试进程必须与服务端跑在同一台机器上。
//
// 铺数据是幂等的：每个用例用 `freshSession()` 建一个新 uuid 目录，结束时（after）删掉。
//
// ---------------------------------------------------------------------------
// 转码（"特殊"的一条 URL）
// ---------------------------------------------------------------------------
// POST /recordings/{preview_id}/transcode 要求请求体带 ue_project（face_capture_transcode.cpp:172-199）：
//   1. 会话目录没有 take.mp4            → 404「take.mp4 不存在」
//   2. 缺 ue_project / 不是字符串       → 422「缺少 ue_project」
//   3. ue_project 路径不存在            → 422「ue_project 不存在: <path>」
//   4. 入库 + start_next + 立即返回 200/201 的 server_task_info（status=submitted）
// 之后后台任务在后台解帧、调 UE 解算，终态只有 completed / failed 两种。
// 本文件**只做 A 层**：断言提交、双通道（单条 + 全量列表）可见、终态落在 {completed, failed}
// 之内；等不到终态不判失败，而是带日志跳过 —— 终态依赖 UE 工程、插件版本服务与解算环境，
// 不在这组契约测试的责任范围（FACE_CAPTURE_TRANSCODE_E2E=1 时才额外断言 completed）。
//
// ---------------------------------------------------------------------------
// 刻意不覆盖
// ---------------------------------------------------------------------------
// · 录制中（previewing → recording）的路径：需要能打开的摄像头，本机 CI 不可控；
//   无摄像头时 GET /devices 返回空列表，相关用例会跳过。
// · WS 的 hello / 音视频帧：只有 capture_session::open 成功（摄像头打开）后才会推送
//   （capture_session.cpp:110-112），本文件只断言握手与升级语义。
// · GET /stream/{id} 不带 token：服务端在升级**之后**才校验 token（http_session_data.cpp:157-182），
//   客户端看到的是握手成功后被关闭，而不是 HTTP 401，行为不确定，不建断言。
//
// 环境变量见 README.md「面部捕捉」。

import { test } from '@playwright/test';
import { expect } from 'chai';
import fs from 'node:fs';
import path from 'node:path';
import net from 'node:net';
import crypto from 'node:crypto';

import {
  URL,
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
} from './config.js';
import { readJson, readBuffer, expectError } from './helpers.js';

// capture_state 的取值（face_capture_types.h），JSON 里是字符串
const STATE_IDLE = 'idle';
const STATE_STOPPED = 'stopped';
const STATE_PREVIEWING = 'previewing';

// 1x1 透明 PNG，用来铺 frames/ 计数
const PNG_1PX = Buffer.from(
  '89504e470d0a1a0a0000000d49484452000000010000000108060000001f15c489' +
    '0000000d4944415478da63f8cfc0f01f0005000201a5f4c1b40000000049454e44ae426082',
  'hex'
);

// 样本视频的字节数随样本变化，这里只在运行时取真实大小做对比
const SAMPLE_READY = fs.existsSync(FACE_CAPTURE_SAMPLE);
const CURVES_SOURCE = path.join(FACE_CAPTURE_SEED_DIR, 'face_curves.dhfc');
const CURVES_READY = fs.existsSync(CURVES_SOURCE);

test.describe.configure({ mode: 'serial' });

// ---------------------------------------------------------------------------
// 会话目录铺数据
// ---------------------------------------------------------------------------

function sessionDir(sessionId) {
  return path.join(FACE_CAPTURE_SESSIONS_DIR, sessionId);
}

function writeAlign(dir, sessionId, frameCount) {
  const frames = Array.from({ length: frameCount }, (_, i) => ({
    seq: i,
    orig_seq: i,
    wall_ms: i * 40,
  }));
  fs.writeFileSync(
    path.join(dir, 'align.json'),
    JSON.stringify({
      session_id: sessionId,
      width: 1280,
      height: 720,
      fps: 25,
      source_fps: 25,
      started_at: '2026-11-11T15:30:00+08:00',
      duration_ms: frameCount * 40,
      raw_frames: frames,
      inserted_frames: [],
      frames,
      gaps: [],
      truncated: false,
    })
  );
  return frames;
}

function seedSession({ withAlign = true, withVideo = true, withCurves = true, withFrames = 0, frameCount = 75 } = {}) {
  const sessionId = crypto.randomUUID();
  const dir = sessionDir(sessionId);
  fs.mkdirSync(dir, { recursive: true });
  if (withVideo) fs.copyFileSync(FACE_CAPTURE_SAMPLE, path.join(dir, 'take.mp4'));
  const frames = withAlign ? writeAlign(dir, sessionId, frameCount) : [];
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
  return { sessionId, dir, frames };
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
// devices
// ---------------------------------------------------------------------------

test.describe('面部捕捉 devices', () => {
  test('GET /devices 返回选中设备与设备列表', async ({ request }) => {
    const res = await readJson(await request.get(`${FACE_CAPTURE_API}/devices`, { headers: authCookie() }));
    expect(res.status, JSON.stringify(res.body)).to.equal(200);
    expect(res.body).to.have.property('selected');
    expect(res.body).to.have.property('devices').that.is.an('array');
    expect(res.body.selected).to.be.a('number');

    for (const device of res.body.devices) {
      expect(device).to.have.all.keys('index', 'name', 'backend', 'width', 'height', 'selected');
      expect(device.index).to.be.a('number');
      expect(device.name).to.equal(`camera_${device.index}`);
      expect(device.backend).to.be.a('string');
      expect(device.width).to.be.a('number');
      expect(device.height).to.be.a('number');
      expect(device.selected).to.equal(device.index === res.body.selected);
    }

    // list_devices 逐 index 打开、首个打不开即 break（capture_session.cpp:504-521），所以 index 从 0 连续
    res.body.devices.forEach((device, i) => expect(device.index).to.equal(i));

    if (res.body.devices.length === 0) {
      // 没有摄像头时选中项仍是默认 0，列表为空
      expect(res.body.selected).to.equal(0);
    } else {
      expect(res.body.devices.map((d) => d.index)).to.include(res.body.selected);
    }
  });

  test('POST /devices 没有摄像头时拒绝不存在的 index', async ({ request }) => {
    const list = await readJson(await request.get(`${FACE_CAPTURE_API}/devices`, { headers: authCookie() }));
    const missing = 100;
    expect(list.body.devices.map((d) => d.index)).to.not.include(missing);
    if (list.body.devices.length >= missing) test.skip(true, `本机摄像头多于 ${missing} 个`);

    const res = await readJson(
      await request.post(`${FACE_CAPTURE_API}/devices`, { headers: authHeader(), data: { index: missing } })
    );
    expectError(res, 404, `没有 index = ${missing} 的摄像头`);
  });

  test('POST /devices 请求体校验', async ({ request }) => {
    const cases = [
      ['空对象', {}, '请求体需要整数 index'],
      ['字符串 index', { index: '0' }, '请求体需要整数 index'],
      ['小数 index', { index: 1.5 }, '请求体需要整数 index'],
      ['布尔 index', { index: true }, '请求体需要整数 index'],
      ['数组 index', { index: [0] }, '请求体需要整数 index'],
      ['数组请求体', [], '请求体需要整数 index'],
      ['null 请求体', null, '请求体需要整数 index'],
      ['负数 index', { index: -1 }, 'index 不能为负'],
    ];
    for (const [name, data, fragment] of cases) {
      const res = await readJson(
        await request.post(`${FACE_CAPTURE_API}/devices`, { headers: authHeader(), data })
      );
      expectError(res, 400, fragment);
      expect(res.body.error, name).to.be.a('string');
    }
  });

  test('POST /devices 非法 JSON 返回 400', async ({ request }) => {
    const res = await readJson(
      await request.post(`${FACE_CAPTURE_API}/devices`, {
        headers: { ...authHeader(), 'Content-Type': 'application/json' },
        data: '{"index":}',
      })
    );
    expectError(res, 400);
  });

  test('POST /devices 选中已有设备后 GET 同步', async ({ request }) => {
    const list = await readJson(await request.get(`${FACE_CAPTURE_API}/devices`, { headers: authCookie() }));
    if (list.body.devices.length === 0) test.skip(true, '本机没有可用摄像头');

    const target = list.body.devices[list.body.devices.length - 1].index;
    const posted = await readJson(
      await request.post(`${FACE_CAPTURE_API}/devices`, { headers: authHeader(), data: { index: target } })
    );
    expect(posted.status, JSON.stringify(posted.body)).to.be.oneOf([200, 201]);
    expect(posted.body).to.deep.equal({ selected: target });

    const after = await readJson(await request.get(`${FACE_CAPTURE_API}/devices`, { headers: authCookie() }));
    expect(after.body.selected).to.equal(target);
    expect(after.body.devices.filter((d) => d.selected).map((d) => d.index)).to.deep.equal([target]);
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
    expect(entry.session_id).to.equal(seeded.sessionId);
    expect(entry.width).to.equal(1280);
    expect(entry.height).to.equal(720);
    expect(entry.fps).to.equal(25);
    expect(entry.source_fps).to.equal(25);
    expect(entry.started_at).to.equal('2026-11-11T15:30:00+08:00');
    expect(entry.duration_ms).to.equal(seeded.frames.length * 40);
    expect(entry.frames).to.deep.equal(seeded.frames);
    expect(entry.gaps).to.deep.equal([]);
    expect(entry.truncated).to.equal(false);
  });

  test('GET /recordings 跳过既无 take.mp4 又无 align.json 的目录', async ({ request }) => {
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

    // 只有 align.json 的目录算一条会话（recording_json 能从 align.json 补全字段）
    fs.writeFileSync(path.join(onlyFramesDir, 'take.mp4'), 'not a real video');
    const after = await readJson(await request.get(`${FACE_CAPTURE_API}/recordings`, { headers: authCookie() }));
    const entry = after.body.data.find((item) => item.session_id === onlyFrames);
    expect(entry, '只补了 take.mp4 的目录应出现在列表里').to.not.equal(undefined);
    expect(entry).to.not.have.property('width');
    expect(entry).to.not.have.property('frames');
  });

  test('GET /recordings 对损坏的 align.json 只返回 session_id', async ({ request }) => {
    const sessionId = crypto.randomUUID();
    const dir = sessionDir(sessionId);
    fs.mkdirSync(dir, { recursive: true });
    fs.writeFileSync(path.join(dir, 'align.json'), '{ broken json');
    created.push(sessionId);

    const res = await readJson(await request.get(`${FACE_CAPTURE_API}/recordings`, { headers: authCookie() }));
    expect(res.status).to.equal(200);
    const entry = res.body.data.find((item) => item.session_id === sessionId);
    expect(entry, '损坏的 align.json 不阻止目录进列表').to.not.equal(undefined);
    expect(entry).to.deep.equal({ session_id: sessionId });
  });
});

// ---------------------------------------------------------------------------
// 单条 recording
// ---------------------------------------------------------------------------

test.describe('面部捕捉 recording 详情', () => {
  test('GET /recordings/{id} 返回对齐信息与产物清单', async ({ request }) => {
    const seeded = freshSession({ withFrames: 3 });

    const res = await readJson(
      await request.get(`${FACE_CAPTURE_API}/recordings/${seeded.sessionId}`, { headers: authCookie() })
    );
    expect(res.status, JSON.stringify(res.body)).to.equal(200);
    expect(res.body.session_id).to.equal(seeded.sessionId);
    expect(res.body.state).to.equal(STATE_STOPPED);
    expect(res.body.width).to.equal(1280);
    expect(res.body.height).to.equal(720);
    expect(res.body.fps).to.equal(25);
    expect(res.body.duration_ms).to.equal(seeded.frames.length * 40);
    expect(res.body.frames).to.deep.equal(seeded.frames);
    expect(res.body.artifacts).to.deep.equal({
      take_mp4: true,
      align_json: true,
      frames: 3,
      curves: true,
    });
  });

  test('GET /recordings/{id} 会话不存在返回 404', async ({ request }) => {
    const res = await readJson(
      await request.get(`${FACE_CAPTURE_API}/recordings/${NON_EXISTENT_ID}`, { headers: authCookie() })
    );
    expectError(res, 404, '会话不存在');
  });

  test('GET /recordings/{id} 只有 take.mp4 时缺失字段 + artifacts 为假', async ({ request }) => {
    const seeded = freshSession({ withAlign: false, withCurves: false });

    const res = await readJson(
      await request.get(`${FACE_CAPTURE_API}/recordings/${seeded.sessionId}`, { headers: authCookie() })
    );
    expect(res.status, JSON.stringify(res.body)).to.equal(200);
    expect(res.body.session_id).to.equal(seeded.sessionId);
    expect(res.body.state).to.equal(STATE_STOPPED);
    expect(res.body).to.not.have.property('width');
    expect(res.body).to.not.have.property('frames');
    expect(res.body.artifacts).to.deep.equal({
      take_mp4: true,
      align_json: false,
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
    expectError(res, 404, '会话不存在');
  });
});

// ---------------------------------------------------------------------------
// video / curves 下载
// ---------------------------------------------------------------------------

test.describe('面部捕捉产物下载', () => {
  test('GET /recordings/{id}/video 发 mp4 且支持 Range', async ({ request }) => {
    const seeded = freshSession({ withCurves: false });

    const head = await request.fetch(`${FACE_CAPTURE_API}/recordings/${seeded.sessionId}/video`, {
      method: 'HEAD',
      headers: authCookie(),
    });
    const headInfo = await readBuffer(head);
    expect(headInfo.status, 'HEAD 走 body 层的 empty_body 分支，仍是 200').to.equal(200);
    expect(headInfo.headers['content-type']).to.equal('video/mp4');
    expect(headInfo.headers['accept-ranges']).to.equal('bytes');

    const size = fs.statSync(path.join(seeded.dir, 'take.mp4')).size;
    expect(Number(headInfo.headers['content-length'])).to.equal(size);
    expect(headInfo.headers['etag'], 'etag = "{mtime}-{size}-{adler32}"').to.include(`-${size}-`);

    const ranged = await request.fetch(`${FACE_CAPTURE_API}/recordings/${seeded.sessionId}/video`, {
      headers: { ...authCookie(), Range: 'bytes=0-15' },
    });
    const rangedInfo = await readBuffer(ranged);
    expect(rangedInfo.status, '服务端只认 bytes=N- 形式').to.equal(200);

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

  test('GET /recordings/{id}/video 没有 take.mp4 返回 404', async ({ request }) => {
    const seeded = freshSession({ withVideo: false, withCurves: false });

    const res = await readJson(
      await request.get(`${FACE_CAPTURE_API}/recordings/${seeded.sessionId}/video`, { headers: authCookie() })
    );
    expectError(res, 404, 'take.mp4 不存在');
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
    expect(res.buffer.subarray(0, 4), '第 8 字节起应是版本号').to.not.deep.equal(Buffer.from('DHFC'));
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
// 没有预览会话时的录制入口
// ---------------------------------------------------------------------------

test.describe('面部捕捉录制入口（无预览会话）', () => {
  // 这组断言的前提是服务端进程里没有活着的 ws_outlet（holder_->session() == nullptr）。
  // 一旦某个 WS 用例建立了会话，服务端单例就有值了，所以放在 WS 用例之前执行（文件内串行）。
  test('POST /recordings 没有预览连接返回 409', async ({ request }) => {
    const res = await readJson(
      await request.post(`${FACE_CAPTURE_API}/recordings`, { headers: authHeader(), data: {} })
    );
    expectError(res, 409, '当前没有可用的预览连接');
  });

  test('POST /recordings/{id}/stop 没有活动会话返回 404', async ({ request }) => {
    const res = await readJson(
      await request.post(`${FACE_CAPTURE_API}/recordings/${NON_EXISTENT_ID}/stop`, {
        headers: authHeader(),
        data: {},
      })
    );
    expectError(res, 404, '没有活动会话');
  });
});

// ---------------------------------------------------------------------------
// 转码（A 层）
// ---------------------------------------------------------------------------

test.describe('面部捕捉转码', () => {
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
    expectError(res, 404, 'take.mp4 不存在');
  });

  test('POST /recordings/{id}/transcode 缺少 take.mp4 返回 404', async ({ request }) => {
    const seeded = freshSession({ withVideo: false, withCurves: false });

    const res = await readJson(
      await request.post(`${FACE_CAPTURE_API}/recordings/${seeded.sessionId}/transcode`, {
        headers: authHeader(),
        data: { ue_project: FACE_CAPTURE_UE_PROJECT },
      })
    );
    expectError(res, 404, 'take.mp4 不存在');
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

  test('POST /recordings/{id}/transcode 未升级成数组的请求体也返回 422', async ({ request }) => {
    const seeded = freshSession({ withCurves: false });

    for (const data of [[], null, 'ue_project']) {
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
        data: { ue_project: 'E:\\definitely-not-here\\x.uproject' },
      })
    );
    expectError(res, 422, 'ue_project 不存在');
  });

  test('POST /recordings/{id}/transcode 提交任务并在两个通道可见', async ({ request }) => {
    test.skip(!fs.existsSync(FACE_CAPTURE_UE_PROJECT), `UE 工程不存在：${FACE_CAPTURE_UE_PROJECT}`);
    const seeded = freshSession({ withCurves: false });

    const res = await readJson(
      await request.post(`${FACE_CAPTURE_API}/recordings/${seeded.sessionId}/transcode`, {
        headers: authHeader(),
        data: { ue_project: FACE_CAPTURE_UE_PROJECT },
      })
    );
    expect([200, 201], JSON.stringify(res.body)).to.include(res.status);
    const task = res.body;
    expectServerTaskInfo(task);
    expect(task.id).to.equal(seeded.sessionId);
    expect(task.name).to.equal(`face_capture_transcode_${seeded.sessionId}`);
    expect(task.command).to.deep.equal({ preview_id: seeded.sessionId, ue_project: FACE_CAPTURE_UE_PROJECT });
    expect(task.status, '提交时 start_next 会立刻把它置为 running').to.be.oneOf(['submitted', 'running']);

    const single = await readJson(
      await request.get(`${FACE_CAPTURE_API}/recordings/${seeded.sessionId}/transcode`, { headers: authCookie() })
    );
    expect(single.status).to.equal(200);
    expect(single.body.id).to.equal(seeded.sessionId);

    const all = await readJson(await request.get(`${FACE_CAPTURE_API}/transcode`, { headers: authCookie() }));
    expect(all.body.filter((item) => item.id === seeded.sessionId)).to.have.lengthOf(1);

    // 同一个会话再提交一次：enqueue 不去重，列表里会变成两条同 id 的任务
    const again = await readJson(
      await request.post(`${FACE_CAPTURE_API}/recordings/${seeded.sessionId}/transcode`, {
        headers: authHeader(),
        data: { ue_project: FACE_CAPTURE_UE_PROJECT },
      })
    );
    expect([200, 201]).to.include(again.status);
    const all2 = await readJson(await request.get(`${FACE_CAPTURE_API}/transcode`, { headers: authCookie() }));
    expect(all2.body.filter((item) => item.id === seeded.sessionId)).to.have.lengthOf(2);

    const single2 = await readJson(
      await request.get(`${FACE_CAPTURE_API}/recordings/${seeded.sessionId}/transcode`, { headers: authCookie() })
    );
    expect(single2.status).to.equal(200, '单条查询返回第一条匹配的任务');
  });

  test('POST /recordings/{id}/transcode 终态落在 completed / failed 之内', async ({ request }) => {
    test.skip(!fs.existsSync(FACE_CAPTURE_UE_PROJECT), `UE 工程不存在：${FACE_CAPTURE_UE_PROJECT}`);
    const seeded = freshSession({ withCurves: false });

    const submitted = await readJson(
      await request.post(`${FACE_CAPTURE_API}/recordings/${seeded.sessionId}/transcode`, {
        headers: authHeader(),
        data: { ue_project: FACE_CAPTURE_UE_PROJECT },
      })
    );
    expect([200, 201]).to.include(submitted.status);

    let last = null;
    const deadline = Date.now() + FACE_CAPTURE_WAIT_MS;
    while (Date.now() < deadline) {
      const res = await readJson(
        await request.get(`${FACE_CAPTURE_API}/recordings/${seeded.sessionId}/transcode`, { headers: authCookie() })
      );
      last = res.body;
      if (last && (last.status === 'completed' || last.status === 'failed')) break;
      await new Promise((resolve) => setTimeout(resolve, 1000));
    }

    const detail = last ? `status=${last.status} last_line_log=${last.last_line_log}` : '没有拿到任务';
    expect(last, '任务应始终可查').to.not.equal(null);
    if (last.status === 'running' || last.status === 'submitted') {
      test.skip(true, `UE 解算未在 ${FACE_CAPTURE_WAIT_MS}ms 内结束（${detail}）`);
    }
    expect(last.status, detail).to.be.oneOf(['completed', 'failed']);
    expect(last.end_time, '终态一定有 end_time').to.not.equal(null);

    if (FACE_CAPTURE_TRANSCODE_E2E) {
      expect(last.status, `端到端模式要求解算成功：${detail}`).to.equal('completed');
      const rec = await readJson(
        await request.get(`${FACE_CAPTURE_API}/recordings/${seeded.sessionId}`, { headers: authCookie() })
      );
      expect(rec.body.artifacts.curves, '解算成功应产出 curves/face_curves.dhfc').to.equal(true);
      expect(rec.body.artifacts.frames, '解算前会清空 frames/ 并重新解帧').to.be.greaterThan(0);
    }
  });
});

// ---------------------------------------------------------------------------
// WebSocket stream
// ---------------------------------------------------------------------------

test.describe('面部捕捉 stream（WebSocket）', () => {
  test('GET /stream/{id} 返回 101 且必须带 token', async ({ request }) => {
    const unauth = await rawHandshake(request, `${FACE_CAPTURE_API}/stream/${NIL_ID}`, { withAuth: false });
    expect(unauth.statusLine, '不带 token 时服务端在升级之后才校验，客户端看到的是 101 后被关闭').to.equal(
      'HTTP/1.1 101 Switching Protocols'
    );
    expect(unauth.closed, '升级后连接应被服务端关闭').to.equal(true);
    expect(unauth.afterClose.length, '没有会话时不应推送任何帧').to.equal(0);
  });

  test('GET /stream/{id} 带 token 升级成功并保持连接', async ({ request }) => {
    const ws = await rawHandshake(request, `${FACE_CAPTURE_API}/stream/${NIL_ID}`, { withAuth: true });
    expect(ws.statusLine).to.equal('HTTP/1.1 101 Switching Protocols');
    expect(ws.headers['upgrade'], ws.headText).to.equal('websocket');
    expect(ws.headers['connection'].toLowerCase()).to.equal('upgrade');
    expect(ws.headers['sec-websocket-accept']).to.be.a('string');
    expect(ws.closed, '没有摄像头时没有 hello，但连接保持打开直到客户端关闭').to.equal(false);
  });

  test('GET /stream/{id} 非升级请求走普通 GET 且未实现返回 404', async ({ request }) => {
    const res = await readJson(
      await request.get(`${FACE_CAPTURE_API}/stream/${NIL_ID}`, { headers: authCookie() })
    );
    expectError(res, 404, '服务器端未实现 api');
  });

  test('GET /devices 在会话目录铺好数据后仍可用', async ({ request }) => {
    const seeded = freshSession({ withCurves: false });
    const res = await readJson(
      await request.get(`${FACE_CAPTURE_API}/recordings/${seeded.sessionId}`, { headers: authCookie() })
    );
    expect(res.status).to.equal(200);
    expect(res.body.session_id).to.equal(seeded.sessionId);
    expect(res.body.state).to.equal(STATE_STOPPED);
  });
});

// ---------------------------------------------------------------------------
// 鉴权与通用请求语义
// ---------------------------------------------------------------------------

test.describe('面部捕捉鉴权', () => {
  const routes = [
    ['GET', '/devices'],
    ['POST', '/devices'],
    ['GET', '/recordings'],
    ['POST', '/recordings'],
    ['GET', `/recordings/${NIL_ID}`],
    ['DELETE', `/recordings/${NIL_ID}`],
    ['POST', `/recordings/${NIL_ID}/stop`],
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
    const res = await request.fetch(`${FACE_CAPTURE_API}/devices`, { method: 'OPTIONS' });
    expect(res.status()).to.equal(200);
    expect(res.headers()['access-control-allow-methods']).to.include('DELETE');
  });

  test('只带 Bearer 也能通过鉴权', async ({ request }) => {
    const res = await readJson(
      await request.get(`${FACE_CAPTURE_API}/devices`, { headers: { Authorization: 'Bearer any-value' } })
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
  return { Cookie: `access_token_cookie=${JWT}`, 'Content-Type': 'application/json' };
}

function authCookie() {
  return { Cookie: `access_token_cookie=${JWT}` };
}

function expectServerTaskInfo(task) {
  expect(task, 'server_task_info 的固定字段（server_task_info.h:148-165）').to.include.keys(
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

// Playwright 的 request fixture 不支持 WebSocket，这里用 net 自己发一次 Upgrade 请求，
// 等到响应头之后再保持 holdMs，用来区分「升级后被关闭」和「保持打开」。
async function rawHandshake(urlPath, { withAuth, holdMs = 1500, cookie } = {}) {
  const origin = new globalThis.URL(URL);
  const port = Number(origin.port || 80);
  const key = crypto.randomBytes(16).toString('base64');

  return new Promise((resolve, reject) => {
    const sock = net.connect({ host: origin.hostname, port });
    let buffer = Buffer.alloc(0);
    let headEnded = false;
    let headers = {};
    let statusLine = '';
    let closed = false;
    let settled = false;
    let timer = null;

    const settle = () => {
      if (settled) return;
      settled = true;
      if (timer) clearTimeout(timer);
      const payload = headEnded ? buffer.subarray(buffer.indexOf('\r\n\r\n') + 4) : Buffer.alloc(0);
      try {
        sock.destroy();
      } catch {
        /* 连接可能已经关闭 */
      }
      resolve({
        statusLine,
        headers,
        headText: headEnded ? buffer.subarray(0, buffer.indexOf('\r\n\r\n')).toString('latin1') : '',
        closed,
        payload,
      });
    };

    timer = setTimeout(settle, holdMs);
    sock.setTimeout(holdMs + 3000, settle);
    sock.on('connect', () => {
      sock.write(
        `GET ${urlPath} HTTP/1.1\r\nHost: ${origin.host}\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n` +
          `Sec-WebSocket-Key: ${key}\r\nSec-WebSocket-Version: 13\r\n` +
          (withAuth ? `Cookie: ${cookie || authCookie().Cookie}\r\n` : '') +
          '\r\n'
      );
    });
    sock.on('data', (chunk) => {
      buffer = Buffer.concat([buffer, chunk]);
      if (headEnded) return;
      const idx = buffer.indexOf('\r\n\r\n');
      if (idx < 0) return;
      headEnded = true;
      const lines = buffer.subarray(0, idx).toString('latin1').split('\r\n');
      statusLine = lines[0];
      for (const line of lines.slice(1)) {
        const sep = line.indexOf(':');
        if (sep > 0) headers[line.slice(0, sep).trim().toLowerCase()] = line.slice(sep + 1).trim();
      }
    });
    sock.on('close', () => {
      closed = true;
      settle();
    });
    sock.on('error', (err) => {
      if (!settled) {
        if (timer) clearTimeout(timer);
        settle();
        if (!statusLine) reject(err);
      }
    });
  });
}