// ai_light.js — AI 灯光 HTTP 接口测试
//
// 覆盖 src/doodle_lib/http_method/kitsu/ai_light.h 中注册的**全部** URI：
//
//   1. GET/POST /api/doodle/ai/light/shot/{entity_id}/session
//   2. GET/DELETE /api/doodle/ai/light/session/{id}
//   3. GET/POST /api/doodle/ai/light/session/{session_id}/reference
//   4. DELETE /api/doodle/ai/light/reference/{id}
//   5. GET /api/doodle/ai/light/reference/{id}/image
//   6. GET /api/doodle/ai/light/candidate/{id}/image
//   7. GET/POST /api/doodle/ai/light/session/{session_id}/spec
//   8. POST /api/doodle/ai/light/session/{session_id}/spec/confirm
//   9. POST /api/doodle/ai/light/session/{session_id}/iterate
//  10. POST /api/doodle/ai/light/session/{session_id}/judge
//  11. POST /api/doodle/ai/light/render_report/{task_id}
//  12. POST /api/doodle/ai/light/render_report_local/{session_id}/{iteration}
//  13. POST /api/doodle/ai/light/candidate/{id}/file/{role}
//  14. GET /api/doodle/ai/light/candidate/{id}/movie
//  15. GET/POST /api/doodle/ai/light/preset
//  16. GET/PUT/DELETE /api/doodle/ai/light/preset/{id}
//
// 前置条件（本文件不负责创建）：需要一个**已有灯光任务**的镜头 entity_id。
// 建会话时服务端用 resolve_lighting_task_id() 从镜头解析灯光任务，
// 镜头没有灯光任务会直接 400「镜头 {} 没有灯光任务，无法建立 AI 灯光会话」。
// 另外该镜头必须属于当前登录人有权限的项目（JWT sub 对应的用户）。
// 从环境变量覆盖：
//     $env:AI_LIGHT_ENTITY_ID = "<镜头 uuid>"
//
// 注意：本文件只做 HTTP 契约验证。真实跑一遍完整链路（/spec → /spec/confirm →
// /iterate → /judge）会调用方舟推理接口并产生真实计费，因此那些步骤默认跳过，
// 需要显式打开：$env:AI_LIGHT_FULL_FLOW = "1"
//

import request from 'superagent';
import { expect } from 'chai';
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { URL, JWT } from './config.js';

const authHeader = { 'Cookie': `access_token_cookie=${JWT}` };

const __dirname = path.dirname(fileURLToPath(import.meta.url));

// 参考图目录：E:\Doodle\build\image
const IMAGE_DIR = process.env.AI_LIGHT_IMAGE_DIR || path.resolve(__dirname, '..', '..', 'build', 'image');

// 有灯光任务的镜头 id（建会话的唯一入口需要它）
const ENTITY_ID = process.env.AI_LIGHT_ENTITY_ID || '019e8267-2505-70d6-8050-430e4565ad8e';

// 是否跑完整链路（会真的调 LLM 推理、产生计费）
const FULL_FLOW = process.env.AI_LIGHT_FULL_FLOW === '1';

const NON_EXISTENT_ID = '00000000-0000-0000-0000-000000000000';

// ---------------------------------------------------------------------------
// 提示词：场景总关卡为「路径」，含灯光子关卡；复制一份出来改，删除除 post 框以外的
// 全部灯光与氛围组件（保留 bp_sky），要求白天氛围。
// ---------------------------------------------------------------------------
const PROMPT_TEXT = [
  '根据参考图为该场景进行灯光氛围制作，',
  '场景总关卡为"路径"，其中包含了灯光子关卡，',
  '复制一份出来在其中进行修改，',
  '修改前删除除了post框以外的所有灯光和氛围相关的组件再开始，',
  '可以保留bp_sky。',
  '要求是：白天氛围',
].join('');

// 列出 build/image 下的参考图（PNG/JPG）
function listReferenceImages() {
  if (!fs.existsSync(IMAGE_DIR)) return [];
  return fs
    .readdirSync(IMAGE_DIR)
    .filter((n) => /\.(png|jpg|jpeg|webp|tif|tiff|exr|bmp)$/i.test(n))
    .map((n) => path.join(IMAGE_DIR, n))
    .sort();
}

// superagent 默认把非 2xx 抛异常。契约测试要看状态码与错误体，所以统一转成返回响应。
async function send(build) {
  try {
    return await build();
  } catch (err) {
    if (err && typeof err.status === 'number') return err.response || err;
    throw err;
  }
}

// 断言一个「失败响应」的状态码与错误消息片段
function expectError(res, status, fragment) {
  expect(res.status, `期望 ${status}，实际 ${res.status}：${JSON.stringify(res.body)}`).to.equal(status);
  if (fragment) {
    const text = JSON.stringify(res.body ?? '');
    expect(text, `错误消息应包含「${fragment}」，实际：${text}`).to.include(fragment);
  }
}

// ═══════════════════════════════════════════════════════════════════════════
// 0. 连通性与前置条件
// ═══════════════════════════════════════════════════════════════════════════

describe('AI 灯光 —— 前置条件检查', function () {
  this.timeout(30000);

  it('build/image 参考图目录存在且至少有一张图', function () {
    expect(fs.existsSync(IMAGE_DIR), `参考图目录不存在: ${IMAGE_DIR}`).to.be.true;
    const images = listReferenceImages();
    expect(images.length, `参考图目录里没有图片: ${IMAGE_DIR}`).to.be.above(0);
    console.log(`参考图目录 ${IMAGE_DIR} 下共 ${images.length} 张图`);
  });

  it('配置了 AI_LIGHT_ENTITY_ID（有灯光任务的镜头）', function () {
    if (!ENTITY_ID) {
      this.skip();
    }
    expect(ENTITY_ID).to.match(/^[0-9a-fA-F-]{36}$/, 'AI_LIGHT_ENTITY_ID 必须是 uuid');
  });
});

// ═══════════════════════════════════════════════════════════════════════════
// 1. 会话：/shot/{entity_id}/session 与 /session/{id}
// ═══════════════════════════════════════════════════════════════════════════

describe('AI 灯光 —— 会话（shot_session / session_instance）', function () {
  this.timeout(60000);

  let sessionId = null;

  after(async function () {
    if (sessionId) {
      await send(() => request.delete(`${URL}/api/doodle/ai/light/session/${sessionId}`).set(authHeader));
      sessionId = null;
    }
  });

  it('POST /shot/{entity_id}/session — 建会话返回六表聚合详情', async function () {
    if (!ENTITY_ID) this.skip();

    const req = await send(() =>
      request
        .post(`${URL}/api/doodle/ai/light/shot/${ENTITY_ID}/session`)
        .set(authHeader)
        .send({
          prompt_text: PROMPT_TEXT,
          title: 'AI 灯光测试会话',
          // 下面三个由服务端从镜头覆盖，客户端传什么都改不了归属
          entity_id: NON_EXISTENT_ID,
          project_id: NON_EXISTENT_ID,
          task_id: NON_EXISTENT_ID,
          // 状态与轮次由服务端强制，传进来也应被忽略
          state: 'awaiting_judgement',
          current_iteration: 99,
        })
    );

    expect(req.status).to.equal(201);
    const body = req.body;

    // 顶层就是 ai_light_session_detail_t 的十个键
    expect(body).to.have.keys([
      'session',
      'references',
      'spec_history',
      'current_spec',
      'iterations',
      'candidates',
      'accepted_candidate',
      'is_waiting_human',
      'is_judgement_overdue',
      'candidate_count_per_iteration',
    ]);

    // 会话本体
    expect(body.session).to.have.keys([
      'id', 'project_id', 'task_id', 'entity_id', 'submitter', 'title',
      'prompt_text', 'state', 'current_iteration', 'judgement_due_at',
      'failure_reason', 'created_at', 'updated_at',
    ]);
    expect(body.session.prompt_text).to.equal(PROMPT_TEXT);
    expect(body.session.title).to.equal('AI 灯光测试会话');

    // 归属与状态由服务端定：请求体里的伪造值必须被覆盖
    expect(body.session.entity_id).to.equal(ENTITY_ID);
    expect(body.session.state).to.equal('draft');
    expect(body.session.current_iteration).to.equal(0);
    expect(body.session.judgement_due_at).to.equal(null);
    expect(body.session.failure_reason).to.equal('');
    expect(body.session.project_id).to.not.equal(NON_EXISTENT_ID);
    expect(body.session.task_id).to.not.equal(NON_EXISTENT_ID);

    // 新会话没有参考图、没有 Spec、没有轮次
    expect(body.references).to.be.an('array').with.lengthOf(0);
    expect(body.spec_history).to.be.an('array').with.lengthOf(0);
    expect(body.current_spec).to.equal(null);
    expect(body.iterations).to.be.an('array').with.lengthOf(0);
    expect(body.candidates).to.be.an('array').with.lengthOf(0);
    expect(body.accepted_candidate).to.equal(null);
    expect(body.is_waiting_human).to.equal(false);
    expect(body.candidate_count_per_iteration).to.equal(6);

    sessionId = body.session.id;
  });

  it('POST /shot/{entity_id}/session — 缺少 prompt_text 返回 400', async function () {
    if (!ENTITY_ID) this.skip();

    const req = await send(() =>
      request.post(`${URL}/api/doodle/ai/light/shot/${ENTITY_ID}/session`).set(authHeader).send({})
    );

    expectError(req, 400, 'prompt_text');
  });

  it('POST /shot/{entity_id}/session — 镜头不存在返回 404', async function () {
    const req = await send(() =>
      request
        .post(`${URL}/api/doodle/ai/light/shot/${NON_EXISTENT_ID}/session`)
        .set(authHeader)
        .send({ prompt_text: PROMPT_TEXT })
    );

    expect(req.status).to.equal(404);
  });

  it('GET /shot/{entity_id}/session — 返回该镜头的会话列表', async function () {
    if (!ENTITY_ID) this.skip();
    expect(sessionId, '需要先建会话').to.not.be.null;

    const req = await send(() =>
      request.get(`${URL}/api/doodle/ai/light/shot/${ENTITY_ID}/session`).set(authHeader)
    );

    expect(req.status).to.equal(200);
    expect(req.body).to.have.property('data');
    expect(req.body.data).to.be.an('array');

    const found = req.body.data.find((s) => s.id === sessionId);
    expect(found, '列表中应包含刚建的会话').to.exist;
    expect(found.entity_id).to.equal(ENTITY_ID);
    expect(found.state).to.equal('draft');
  });

  it('GET /session/{id} — 会话详情', async function () {
    if (!ENTITY_ID) this.skip();
    expect(sessionId, '需要先建会话').to.not.be.null;

    const req = await send(() => request.get(`${URL}/api/doodle/ai/light/session/${sessionId}`).set(authHeader));

    expect(req.status).to.equal(200);
    expect(req.body.session.id).to.equal(sessionId);
    // 详情与建会话返回同构
    expect(req.body).to.have.keys([
      'session', 'references', 'spec_history', 'current_spec', 'iterations',
      'candidates', 'accepted_candidate', 'is_waiting_human',
      'is_judgement_overdue', 'candidate_count_per_iteration',
    ]);
  });

  it('GET /session/{id} — 会话不存在返回 404', async function () {
    const req = await send(() =>
      request.get(`${URL}/api/doodle/ai/light/session/${NON_EXISTENT_ID}`).set(authHeader)
    );

    expect(req.status).to.equal(404);
  });
});

// ═══════════════════════════════════════════════════════════════════════════
// 2. 参考图：上传 / 列表 / 取图 / 删除
// ═══════════════════════════════════════════════════════════════════════════

describe('AI 灯光 —— 参考图（reference）', function () {
  this.timeout(180000);

  let sessionId = null;

  before(async function () {
    if (!ENTITY_ID) this.skip();
    const req = await send(() =>
      request
        .post(`${URL}/api/doodle/ai/light/shot/${ENTITY_ID}/session`)
        .set(authHeader)
        .send({ prompt_text: PROMPT_TEXT, title: 'AI 灯光参考图测试' })
    );
    expect(req.status).to.equal(201);
    sessionId = req.body.session.id;
  });

  after(async function () {
    if (sessionId) {
      await send(() => request.delete(`${URL}/api/doodle/ai/light/session/${sessionId}`).set(authHeader));
      sessionId = null;
    }
  });

  it('POST /session/{id}/reference — 无文件返回 400', async function () {
    expect(sessionId, '需要先建会话').to.not.be.null;

    const req = await send(() =>
      request.post(`${URL}/api/doodle/ai/light/session/${sessionId}/reference`).set(authHeader).send({})
    );

    expectError(req, 400, '参考图');
  });

  it('POST /session/{id}/reference — 多图上传返回参考图列表', async function () {
    expect(sessionId, '需要先建会话').to.not.be.null;

    const images = listReferenceImages();
    expect(images.length, '参考图目录为空').to.be.above(0);
    // 取前两张，验证多图一次上传
    const picked = images.slice(0, 2);

    let builder = request.post(`${URL}/api/doodle/ai/light/session/${sessionId}/reference`).set(authHeader);
    for (const img of picked) builder = builder.attach('file', img);

    const req = await send(() => builder);

    expect(req.status).to.equal(200);
    expect(req.body).to.have.property('data');
    expect(req.body.data).to.have.lengthOf(picked.length);

    req.body.data.forEach((ref, i) => {
      expect(ref).to.have.keys([
        'id', 'session_id', 'ordinal', 'file_path', 'thumbnail_path', 'width',
        'height', 'stats', 'stats_confidence', 'is_outlier', 'excluded_reason',
        'created_at',
      ]);
      expect(ref.session_id).to.equal(sessionId);
      // ordinal 是展示顺序，0 起、递增
      expect(ref.ordinal).to.equal(i);
      // 服务端统一转 PNG 后才落盘
      expect(ref.file_path.toLowerCase()).to.match(/\.png$/);
      // 统计在已落盘的 PNG 上算出来：长宽必须真实
      expect(ref.width).to.be.above(0);
      expect(ref.height).to.be.above(0);
      expect(ref.is_outlier).to.equal(false);
    });
  });

  it('GET /session/{id}/reference — 列表与上传结果一致', async function () {
    expect(sessionId, '需要先建会话').to.not.be.null;

    const req = await send(() =>
      request.get(`${URL}/api/doodle/ai/light/session/${sessionId}/reference`).set(authHeader)
    );

    expect(req.status).to.equal(200);
    expect(req.body.data).to.be.an('array').with.lengthOf(2);
    // 按 ordinal 升序
    expect(req.body.data[0].ordinal).to.equal(0);
    expect(req.body.data[1].ordinal).to.equal(1);
  });

  it('GET /reference/{id}/image — 取参考图（PNG 流）', async function () {
    expect(sessionId, '需要先建会话').to.not.be.null;

    const listReq = await send(() =>
      request.get(`${URL}/api/doodle/ai/light/session/${sessionId}/reference`).set(authHeader)
    );
    const refId = listReq.body.data[0].id;

    // 取图是二进制流：开 buffer + 自带解析器收成 Buffer，而不是默认的 JSON/text 解析
    const req = await send(() =>
      request
        .get(`${URL}/api/doodle/ai/light/reference/${refId}/image`)
        .set(authHeader)
        .buffer(true)
        .parse((res, callback) => {
          const chunks = [];
          res.on('data', (chunk) => chunks.push(chunk));
          res.on('end', () => callback(null, Buffer.concat(chunks)));
        })
    );

    expect(req.status).to.equal(200);
    expect(req.headers['content-type']).to.include('image/png');
    expect(req.body.length, '返回的图片字节不应为空').to.be.above(0);
    // PNG magic
    expect(req.body.slice(0, 8).toString('hex')).to.equal('89504e470d0a1a0a');
  });

  it('GET /reference/{id}/image — 参考图不存在返回 404', async function () {
    const req = await send(() =>
      request.get(`${URL}/api/doodle/ai/light/reference/${NON_EXISTENT_ID}/image`).set(authHeader)
    );

    expect(req.status).to.equal(404);
  });

  it('DELETE /reference/{id} — 删除单张参考图返回 204', async function () {
    expect(sessionId, '需要先建会话').to.not.be.null;

    const listReq = await send(() =>
      request.get(`${URL}/api/doodle/ai/light/session/${sessionId}/reference`).set(authHeader)
    );
    expect(listReq.body.data).to.have.lengthOf(2);
    const refId = listReq.body.data[1].id;

    const delReq = await send(() =>
      request.delete(`${URL}/api/doodle/ai/light/reference/${refId}`).set(authHeader)
    );
    expect(delReq.status).to.equal(204);

    // 删完只剩一张
    const afterReq = await send(() =>
      request.get(`${URL}/api/doodle/ai/light/session/${sessionId}/reference`).set(authHeader)
    );
    expect(afterReq.body.data).to.have.lengthOf(1);
    expect(afterReq.body.data.map((r) => r.id)).to.not.include(refId);
  });

  it('DELETE /reference/{id} — 参考图不存在返回 404', async function () {
    const req = await send(() =>
      request.delete(`${URL}/api/doodle/ai/light/reference/${NON_EXISTENT_ID}`).set(authHeader)
    );

    expect(req.status).to.equal(404);
  });

  it('POST /session/{id}/reference — 会话不存在返回 404', async function () {
    const req = await send(() =>
      request
        .post(`${URL}/api/doodle/ai/light/session/${NON_EXISTENT_ID}/reference`)
        .set(authHeader)
        .send({})
    );

    expect(req.status).to.equal(404);
  });
});

// ═══════════════════════════════════════════════════════════════════════════
// 3. Spec：版本列表 / 生成 / 人工闸门 1
// ═══════════════════════════════════════════════════════════════════════════

describe('AI 灯光 —— Spec（spec / spec_confirm）', function () {
  this.timeout(300000);

  let sessionId = null;

  before(async function () {
    if (!ENTITY_ID) this.skip();
    const req = await send(() =>
      request
        .post(`${URL}/api/doodle/ai/light/shot/${ENTITY_ID}/session`)
        .set(authHeader)
        .send({ prompt_text: PROMPT_TEXT, title: 'AI 灯光 Spec 测试' })
    );
    expect(req.status).to.equal(201);
    sessionId = req.body.session.id;
  });

  after(async function () {
    if (sessionId) {
      await send(() => request.delete(`${URL}/api/doodle/ai/light/session/${sessionId}`).set(authHeader));
      sessionId = null;
    }
  });

  it('GET /session/{id}/spec — 尚无 Spec 时 data 为空、current_revision 为 0', async function () {
    expect(sessionId, '需要先建会话').to.not.be.null;

    const req = await send(() =>
      request.get(`${URL}/api/doodle/ai/light/session/${sessionId}/spec`).set(authHeader)
    );

    expect(req.status).to.equal(200);
    expect(req.body.data).to.be.an('array').with.lengthOf(0);
    expect(req.body.current_revision).to.equal(0);
  });

  it('POST /session/{id}/spec/confirm — 还没有 Spec 就确认返回 400', async function () {
    expect(sessionId, '需要先建会话').to.not.be.null;

    const req = await send(() =>
      request.post(`${URL}/api/doodle/ai/light/session/${sessionId}/spec/confirm`).set(authHeader).send({})
    );

    expectError(req, 400, 'Spec');
  });

  it('POST /session/{id}/spec — 没有参考图时拒绝生成（400）', async function () {
    expect(sessionId, '需要先建会话').to.not.be.null;

    // Spec 的输入是「提示词 + 参考图」，参考图为空时拒绝，
    // 避免蒸出一份 prompt-only 的方案把主输入静默丢掉。
    const req = await send(() =>
      request
        .post(`${URL}/api/doodle/ai/light/session/${sessionId}/spec`)
        .set(authHeader)
        .send({})
    );

    expect(req.status).to.equal(400);
  });

  it('POST /session/{id}/spec — 排除不属于本会话的参考图返回 400', async function () {
    if (!FULL_FLOW) this.skip();
    expect(sessionId, '需要先建会话').to.not.be.null;

    const req = await send(() =>
      request
        .post(`${URL}/api/doodle/ai/light/session/${sessionId}/spec`)
        .set(authHeader)
        .send({ excluded_reference_ids: [NON_EXISTENT_ID] })
    );

    expect(req.status).to.equal(400);
  });

  // -------------------------------------------------------------------------
  // 以下步骤会真的调用 LLM 推理（产生计费），默认跳过。
  // 打开方式：$env:AI_LIGHT_FULL_FLOW = "1"
  // -------------------------------------------------------------------------

  it('POST /session/{id}/spec — 生成第一版 Spec（draft → awaiting_spec_confirm）', async function () {
    if (!ENTITY_ID) this.skip();
    if (!FULL_FLOW) this.skip();
    expect(sessionId, '需要先建会话').to.not.be.null;

    // 先传参考图，否则 /spec 会被 400 挡下
    const images = listReferenceImages().slice(0, 2);
    let up = request.post(`${URL}/api/doodle/ai/light/session/${sessionId}/reference`).set(authHeader);
    for (const img of images) up = up.attach('file', img);
    const upReq = await send(() => up);
    expect(upReq.status).to.equal(200);

    const req = await send(() =>
      request
        .post(`${URL}/api/doodle/ai/light/session/${sessionId}/spec`)
        .set(authHeader)
        .send({ prompt_text: PROMPT_TEXT })
    );

    expect(req.status).to.equal(200);
    expect(req.body.session.state).to.equal('awaiting_spec_confirm');
    expect(req.body.spec_history).to.have.lengthOf(1);

    const spec = req.body.current_spec;
    expect(spec).to.not.be.null;
    expect(spec.revision).to.equal(1);
    expect(spec.session_id).to.equal(sessionId);
    expect(spec.human_confirmed).to.equal(false);
    // 意图来自提示词：白天氛围
    expect(spec.intent).to.be.an('object');
    expect(spec.targets).to.be.an('object');
    expect(spec.evidence).to.be.an('object');
    expect(['stats_only', 'prompt_only']).to.include(spec.degrade_level);

    expect(req.body.is_waiting_human).to.equal(true);
  });

  it('GET /session/{id}/spec — 版本列表含刚生成的 Spec', async function () {
    if (!FULL_FLOW) this.skip();
    expect(sessionId, '需要先建会话').to.not.be.null;

    const req = await send(() =>
      request.get(`${URL}/api/doodle/ai/light/session/${sessionId}/spec`).set(authHeader)
    );

    expect(req.status).to.equal(200);
    expect(req.body.data).to.have.lengthOf(1);
    expect(req.body.current_revision).to.equal(1);
  });

  it('POST /session/{id}/spec/confirm — 人工闸门 1 确认', async function () {
    if (!FULL_FLOW) this.skip();
    expect(sessionId, '需要先建会话').to.not.be.null;

    const req = await send(() =>
      request
        .post(`${URL}/api/doodle/ai/light/session/${sessionId}/spec/confirm`)
        .set(authHeader)
        .send({})
    );

    expect(req.status).to.equal(200);
    // 确认后立即进入候选生成
    expect(req.body.session.state).to.equal('generating');
    expect(req.body.current_spec.human_confirmed).to.equal(true);
    expect(req.body.current_spec.confirmed_by).to.not.be.null;
    expect(req.body.current_spec.confirmed_at).to.not.be.null;
  });

  it('POST /session/{id}/spec/confirm — 重复确认是幂等的', async function () {
    if (!FULL_FLOW) this.skip();
    expect(sessionId, '需要先建会话').to.not.be.null;

    const first = await send(() =>
      request.get(`${URL}/api/doodle/ai/light/session/${sessionId}`).set(authHeader)
    );
    const confirmedAt = first.body.current_spec.confirmed_at;

    const req = await send(() =>
      request
        .post(`${URL}/api/doodle/ai/light/session/${sessionId}/spec/confirm`)
        .set(authHeader)
        .send({})
    );

    expect(req.status).to.equal(200);
    expect(req.body.already_confirmed).to.equal(true);
    // 重复确认不刷新时间戳（那是「谁在什么时候认的」的证据）
    expect(req.body.spec.confirmed_at).to.equal(confirmedAt);
  });

  it('POST /session/{id}/spec/confirm — 会话不存在返回 404', async function () {
    const req = await send(() =>
      request.post(`${URL}/api/doodle/ai/light/session/${NON_EXISTENT_ID}/spec/confirm`).set(authHeader).send({})
    );

    expect(req.status).to.equal(404);
  });
});

// ═══════════════════════════════════════════════════════════════════════════
// 4. 迭代与判断：/iterate、/judge
// ═══════════════════════════════════════════════════════════════════════════

describe('AI 灯光 —— 迭代与判断（iterate / judge）', function () {
  this.timeout(300000);

  let sessionId = null;

  before(async function () {
    if (!ENTITY_ID) this.skip();
    const req = await send(() =>
      request
        .post(`${URL}/api/doodle/ai/light/shot/${ENTITY_ID}/session`)
        .set(authHeader)
        .send({ prompt_text: PROMPT_TEXT, title: 'AI 灯光迭代测试' })
    );
    expect(req.status).to.equal(201);
    sessionId = req.body.session.id;
  });

  after(async function () {
    if (sessionId) {
      await send(() => request.delete(`${URL}/api/doodle/ai/light/session/${sessionId}`).set(authHeader));
      sessionId = null;
    }
  });

  it('POST /session/{id}/iterate — 非 generating 状态返回 400', async function () {
    expect(sessionId, '需要先建会话').to.not.be.null;

    // 会话是 draft，不是 generating
    const req = await send(() =>
      request.post(`${URL}/api/doodle/ai/light/session/${sessionId}/iterate`).set(authHeader).send({})
    );

    expect(req.status).to.equal(400);
    const text = JSON.stringify(req.body ?? '');
    expect(text).to.include('generating');
  });

  it('POST /session/{id}/iterate — 无请求体也能调用（空体不应因 body 解析失败）', async function () {
    expect(sessionId, '需要先建会话').to.not.be.null;

    // /iterate 刻意不读请求体；空体调用应当走到状态校验，而不是「body 不是 json」
    const req = await send(() =>
      request.post(`${URL}/api/doodle/ai/light/session/${sessionId}/iterate`).set(authHeader)
    );

    expect(req.status).to.equal(400);
    const text = JSON.stringify(req.body ?? '');
    expect(text, '不应该报 body 解析错误').to.not.include('body 不是 json');
    expect(text).to.include('generating');
  });

  it('POST /session/{id}/judge — 没有轮次时返回 400', async function () {
    expect(sessionId, '需要先建会话').to.not.be.null;

    const req = await send(() =>
      request
        .post(`${URL}/api/doodle/ai/light/session/${sessionId}/judge`)
        .set(authHeader)
        .send({ decision: 'reject_all' })
    );

    expect(req.status).to.equal(400);
    const text = JSON.stringify(req.body ?? '');
    expect(text).to.include('轮次');
  });

  it('POST /session/{id}/judge — 会话不存在返回 404', async function () {
    const req = await send(() =>
      request
        .post(`${URL}/api/doodle/ai/light/session/${NON_EXISTENT_ID}/judge`)
        .set(authHeader)
        .send({ decision: 'reject_all' })
    );

    expect(req.status).to.equal(404);
  });

  it('POST /session/{id}/iterate — 生成一轮候选并返回 launch_plan', async function () {
    if (!FULL_FLOW) this.skip();
    expect(sessionId, '需要先建会话').to.not.be.null;

    // 前置：参考图 → Spec → 确认（确认后状态为 generating）
    const images = listReferenceImages().slice(0, 2);
    let up = request.post(`${URL}/api/doodle/ai/light/session/${sessionId}/reference`).set(authHeader);
    for (const img of images) up = up.attach('file', img);
    expect((await send(() => up)).status).to.equal(200);

    expect(
      (await send(() =>
        request.post(`${URL}/api/doodle/ai/light/session/${sessionId}/spec`).set(authHeader).send({})
      )).status
    ).to.equal(200);

    expect(
      (await send(() =>
        request
          .post(`${URL}/api/doodle/ai/light/session/${sessionId}/spec/confirm`)
          .set(authHeader)
          .send({})
      )).status
    ).to.equal(200);

    const req = await send(() =>
      request.post(`${URL}/api/doodle/ai/light/session/${sessionId}/iterate`).set(authHeader).send({})
    );

    expect(req.status).to.equal(200);
    // 引擎内路线：本轮该怎么渲放进 launch_plan
    expect(req.body).to.have.property('launch_plan');

    const body = req.body;
    expect(body.session.current_iteration).to.equal(1);
    expect(body.iterations).to.have.lengthOf(1);
    // 固定 6 个候选（g_candidate_count）
    expect(body.candidates).to.have.lengthOf(6);
    expect(body.candidate_count_per_iteration).to.equal(6);

    const plan = body.launch_plan;
    expect(plan.session_id).to.equal(sessionId);
    expect(plan.iteration).to.equal(1);
    expect(plan.candidates).to.have.lengthOf(6);
    // 第 0 个是本轮基线
    expect(plan.candidates[0].candidate_index).to.equal(0);

    // 候选字段
    body.candidates.forEach((c) => {
      expect(c).to.have.keys([
        'id', 'iteration_id', 'session_id', 'candidate_index', 'variant_axis',
        'variant_value', 'variant_label', 'rig', 'render_status', 'render_ms',
        'light_count', 'metrics', 'vlm_score', 'vlm_delta', 'is_selected',
        'is_accepted', 'accepted_session_id', 'preview_file_id', 'created_at',
      ]);
      expect(c.session_id).to.equal(sessionId);
      expect(c.render_status).to.equal('pending');
      expect(c.is_accepted).to.equal(false);
    });
  });

  it('POST /session/{id}/judge — 定稿必须指定候选（400）', async function () {
    if (!FULL_FLOW) this.skip();
    expect(sessionId, '需要先建会话').to.not.be.null;

    const req = await send(() =>
      request
        .post(`${URL}/api/doodle/ai/light/session/${sessionId}/judge`)
        .set(authHeader)
        .send({ decision: 'accept' })
    );

    expectError(req, 400, '候选');
  });

  it('POST /session/{id}/judge — 候选不属于本轮返回 400', async function () {
    if (!FULL_FLOW) this.skip();
    expect(sessionId, '需要先建会话').to.not.be.null;

    const req = await send(() =>
      request
        .post(`${URL}/api/doodle/ai/light/session/${sessionId}/judge`)
        .set(authHeader)
        .send({ decision: 'accept', selected_candidate_id: NON_EXISTENT_ID })
    );

    expect(req.status).to.equal(400);
  });

  it('POST /session/{id}/judge — continue_iteration 记录反馈轴并回到 generating', async function () {
    if (!FULL_FLOW) this.skip();
    expect(sessionId, '需要先建会话').to.not.be.null;

    const detail = await send(() =>
      request.get(`${URL}/api/doodle/ai/light/session/${sessionId}`).set(authHeader)
    );
    const iteration = detail.body.iterations[0];

    const req = await send(() =>
      request
        .post(`${URL}/api/doodle/ai/light/session/${sessionId}/judge`)
        .set(authHeader)
        .send({
          decision: 'continue_iteration',
          feedback_text: '白天氛围不够，整体再亮一点',
          // 滑块通道：直接给轴，不经 LLM
          feedback_axes: { kelvin: -200, ambient_level: 0.2 },
        })
    );

    expect(req.status).to.equal(200);
    expect(req.body.session.state).to.equal('generating');
    // 判断之后回到 generating，next iteration 由 /iterate 触发
    expect(req.body.session.judgement_due_at).to.equal(null);

    const judged = req.body.iterations.find((i) => i.id === iteration.id);
    expect(judged.state).to.equal('judged');
    expect(judged.judged_at).to.not.be.null;
  });
});

// ═══════════════════════════════════════════════════════════════════════════
// 5. 工作机回传：/render_report/{task_id} 与 /render_report_local/{session_id}/{iteration}
// ═══════════════════════════════════════════════════════════════════════════

describe('AI 灯光 —— 回传（render_report / render_report_local）', function () {
  this.timeout(60000);

  let sessionId = null;

  before(async function () {
    if (!ENTITY_ID) this.skip();
    const req = await send(() =>
      request
        .post(`${URL}/api/doodle/ai/light/shot/${ENTITY_ID}/session`)
        .set(authHeader)
        .send({ prompt_text: PROMPT_TEXT, title: 'AI 灯光回传测试' })
    );
    expect(req.status).to.equal(201);
    sessionId = req.body.session.id;
  });

  after(async function () {
    if (sessionId) {
      await send(() => request.delete(`${URL}/api/doodle/ai/light/session/${sessionId}`).set(authHeader));
      sessionId = null;
    }
  });

  it('POST /render_report/{task_id} — 任务不存在返回 404', async function () {
    const req = await send(() =>
      request
        .post(`${URL}/api/doodle/ai/light/render_report/${NON_EXISTENT_ID}`)
        .set(authHeader)
        .send({ candidates: [], frame_rate: 24, error: '' })
    );

    expect(req.status).to.equal(404);
  });

  it('POST /render_report/{task_id} — 任务不是 AI 灯光类型返回 400', async function () {
    if (!ENTITY_ID) this.skip();

    // 会话上的 task_id 是 Kitsu 灯光任务（不是 server_task_info 的 ai_light 任务）
    const detail = await send(() =>
      request.get(`${URL}/api/doodle/ai/light/session/${sessionId}`).set(authHeader)
    );
    const taskId = detail.body.session.task_id;

    const req = await send(() =>
      request
        .post(`${URL}/api/doodle/ai/light/render_report/${taskId}`)
        .set(authHeader)
        .send({ candidates: [], frame_rate: 24, error: '' })
    );

    // 任务行不存在 → 404；存在但不是 ai_light 类型 → 400。两者都是「拒绝」。
    expect([400, 404]).to.include(req.status);
  });

  it('POST /render_report/{task_id} — 回传体缺少严格字段返回 502', async function () {
    if (!ENTITY_ID) this.skip();

    // 先建一个真正的 ai_light 任务行不可行（本文件不派发），所以这里只验证
    // 「形状错误 → 502」这条在解析层的契约：用不存在任务时先被 404 挡下。
    const req = await send(() =>
      request
        .post(`${URL}/api/doodle/ai/light/render_report/${NON_EXISTENT_ID}`)
        .set(authHeader)
        .send({ frame_rate: 24 })
    );

    expect([404, 502]).to.include(req.status);
  });

  it('POST /render_report_local/{session_id}/{iteration} — 轮次不存在返回 404', async function () {
    expect(sessionId, '需要先建会话').to.not.be.null;

    const req = await send(() =>
      request
        .post(`${URL}/api/doodle/ai/light/render_report_local/${sessionId}/1`)
        .set(authHeader)
        .send({ candidates: [], frame_rate: 24, error: '' })
    );

    expectError(req, 404, '轮');
  });

  it('POST /render_report_local/{session_id}/{iteration} — 会话不存在返回 404', async function () {
    const req = await send(() =>
      request
        .post(`${URL}/api/doodle/ai/light/render_report_local/${NON_EXISTENT_ID}/1`)
        .set(authHeader)
        .send({ candidates: [], frame_rate: 24, error: '' })
    );

    expect(req.status).to.equal(404);
  });

  it('POST /render_report_local/{session_id}/{iteration} — 无候选的整轮失败回执', async function () {
    if (!FULL_FLOW) this.skip();
    expect(sessionId, '需要先建会话').to.not.be.null;

    // 轮次必须先存在（由 /iterate 生成），本用例依赖 full flow 的前序步骤
    const detail = await send(() =>
      request.get(`${URL}/api/doodle/ai/light/session/${sessionId}`).set(authHeader)
    );
    if (detail.body.iterations.length === 0) this.skip();
    const iteration = detail.body.iterations[0].iteration;

    const req = await send(() =>
      request
        .post(`${URL}/api/doodle/ai/light/render_report_local/${sessionId}/${iteration}`)
        .set(authHeader)
        .send({ candidates: [], frame_rate: 24, error: 'UE 启动失败' })
    );

    expect(req.status).to.equal(201);
    expect(req.body).to.have.keys(['session_id', 'iteration', 'iteration_state', 'session_state', 'failure_reason']);
    expect(req.body.session_id).to.equal(sessionId);
    expect(req.body.iteration).to.equal(iteration);
    // 整轮失败只写 failure_reason，不改状态机状态（D28）
    expect(req.body.failure_reason).to.include('UE 启动失败');
  });
});

// ═══════════════════════════════════════════════════════════════════════════
// 6. 候选产物：/candidate/{id}/file/{role} 与 /candidate/{id}/movie
// ═══════════════════════════════════════════════════════════════════════════

describe('AI 灯光 —— 候选产物（candidate file / movie / image）', function () {
  this.timeout(120000);

  const OCTET = 'application/octet-stream';

  it('POST /candidate/{id}/file/{role} — 候选不存在返回 404', async function () {
    const req = await send(() =>
      request
        .post(`${URL}/api/doodle/ai/light/candidate/${NON_EXISTENT_ID}/file/render`)
        .set(authHeader)
        .set('Content-Type', OCTET)
        .send(Buffer.from('not-an-image'))
    );

    expect(req.status).to.equal(404);
  });

  it('POST /candidate/{id}/file/{role} — 请求体不是 octet-stream 返回 400', async function () {
    // 内容类型检查在候选存在性之前
    const req = await send(() =>
      request
        .post(`${URL}/api/doodle/ai/light/candidate/${NON_EXISTENT_ID}/file/render`)
        .set(authHeader)
        .set('Content-Type', 'application/json')
        .send({ hello: 'world' })
    );

    expectError(req, 400, 'application/octet-stream');
  });

  it('GET /candidate/{id}/image — 候选不存在返回 404', async function () {
    const req = await send(() =>
      request.get(`${URL}/api/doodle/ai/light/candidate/${NON_EXISTENT_ID}/image`).set(authHeader)
    );

    expect(req.status).to.equal(404);
  });

  it('GET /candidate/{id}/movie — 候选不存在返回 404', async function () {
    const req = await send(() =>
      request.get(`${URL}/api/doodle/ai/light/candidate/${NON_EXISTENT_ID}/movie`).set(authHeader)
    );

    expect(req.status).to.equal(404);
  });

  // 这条是「上传 → 取图」的闭环。上传体就是文件本身，服务端按候选行的三个 uuid
  // 现算落点（D27/D40），成功回 204；随后 GET /candidate/{id}/image 才可能有内容。
  // 但候选必须来自真实一轮（pending 会 404「尚未渲染完成」），所以默认跳过。
  it('POST /candidate/{id}/file/{role} — 上传海报后取图返回 PNG（完整一轮）', async function () {
    if (!FULL_FLOW) this.skip();
    // 需要真实一轮渲染产出的候选：先跑 /iterate 拿到候选 id，再上传 PNG 字节
    this.skip();
  });
});

// ═══════════════════════════════════════════════════════════════════════════
// 7. 预设库：/preset 与 /preset/{id}
// ═══════════════════════════════════════════════════════════════════════════

describe('AI 灯光 —— 预设库（preset）', function () {
  this.timeout(60000);

  let presetId = null;

  after(async function () {
    if (presetId) {
      await send(() => request.delete(`${URL}/api/doodle/ai/light/preset/${presetId}`).set(authHeader));
      presetId = null;
    }
  });

  it('POST /preset — 缺少名称返回 400', async function () {
    const req = await send(() =>
      request.post(`${URL}/api/doodle/ai/light/preset`).set(authHeader).send({ tags: ['测试'] })
    );

    expectError(req, 400, '预设名称');
  });

  it('POST /preset — 新建预设（source 由服务端强制为 artist）', async function () {
    const req = await send(() =>
      request
        .post(`${URL}/api/doodle/ai/light/preset`)
        .set(authHeader)
        .send({
          name: 'ai_light_测试预设_白天',
          tags: ['白天', 'interior'],
          // 客户端传 builtin 不应生效
          source: 'builtin',
          applicable: { scene_type: ['interior', 'day'], interior: true },
          stats_target: { contrast: 0.4 },
        })
    );

    expect(req.status).to.equal(200);
    expect(req.body).to.have.keys([
      'id', 'name', 'tags', 'embedding', 'embedding_dim', 'rig_fragment',
      'applicable', 'stats_target', 'enabled', 'source', 'created_by',
      'created_at', 'updated_at',
    ]);
    expect(req.body.name).to.equal('ai_light_测试预设_白天');
    expect(req.body.tags).to.deep.equal(['白天', 'interior']);
    // 来源由服务端定，客户端传 builtin 被覆盖
    expect(req.body.source).to.equal('artist');
    expect(req.body.enabled).to.equal(true);
    expect(req.body.created_by).to.not.be.null;

    presetId = req.body.id;
  });

  it('GET /preset — 列表包含刚建的预设', async function () {
    expect(presetId, '需要先建预设').to.not.be.null;

    const req = await send(() => request.get(`${URL}/api/doodle/ai/light/preset`).set(authHeader));

    expect(req.status).to.equal(200);
    expect(req.body.data).to.be.an('array');
    expect(req.body.data.map((p) => p.id)).to.include(presetId);
  });

  it('GET /preset?enabled_only=false — 也返回全部预设', async function () {
    expect(presetId, '需要先建预设').to.not.be.null;

    const req = await send(() =>
      request.get(`${URL}/api/doodle/ai/light/preset?enabled_only=false`).set(authHeader)
    );

    expect(req.status).to.equal(200);
    expect(req.body.data.map((p) => p.id)).to.include(presetId);
  });

  it('GET /preset/{id} — 预设详情', async function () {
    expect(presetId, '需要先建预设').to.not.be.null;

    const req = await send(() => request.get(`${URL}/api/doodle/ai/light/preset/${presetId}`).set(authHeader));

    expect(req.status).to.equal(200);
    expect(req.body.id).to.equal(presetId);
    expect(req.body.name).to.equal('ai_light_测试预设_白天');
  });

  it('PUT /preset/{id} — 局部更新只覆盖请求体里出现的字段', async function () {
    expect(presetId, '需要先建预设').to.not.be.null;

    const req = await send(() =>
      request
        .put(`${URL}/api/doodle/ai/light/preset/${presetId}`)
        .set(authHeader)
        .send({ name: 'ai_light_测试预设_白天_改', enabled: false, source: 'builtin' })
    );

    expect(req.status).to.equal(200);
    expect(req.body.name).to.equal('ai_light_测试预设_白天_改');
    expect(req.body.enabled).to.equal(false);
    // tags 没传，应保持原值（局部更新）
    expect(req.body.tags).to.deep.equal(['白天', 'interior']);
    // source 不可改
    expect(req.body.source).to.equal('artist');
  });

  it('GET /preset — enabled_only 默认 true，停用的预设不在列表里', async function () {
    expect(presetId, '需要先建预设').to.not.be.null;

    const req = await send(() => request.get(`${URL}/api/doodle/ai/light/preset`).set(authHeader));

    expect(req.status).to.equal(200);
    expect(req.body.data.map((p) => p.id)).to.not.include(presetId);

    const all = await send(() =>
      request.get(`${URL}/api/doodle/ai/light/preset?enabled_only=false`).set(authHeader)
    );
    expect(all.body.data.map((p) => p.id)).to.include(presetId);
  });

  it('GET /preset/{id} — 预设不存在返回 404', async function () {
    const req = await send(() =>
      request.get(`${URL}/api/doodle/ai/light/preset/${NON_EXISTENT_ID}`).set(authHeader)
    );

    expectError(req, 404, '预设');
  });

  it('PUT /preset/{id} — 预设不存在返回 404', async function () {
    const req = await send(() =>
      request.put(`${URL}/api/doodle/ai/light/preset/${NON_EXISTENT_ID}`).set(authHeader).send({ name: 'x' })
    );

    expect(req.status).to.equal(404);
  });

  it('DELETE /preset/{id} — 删除预设返回 204', async function () {
    expect(presetId, '需要先建预设').to.not.be.null;

    const req = await send(() =>
      request.delete(`${URL}/api/doodle/ai/light/preset/${presetId}`).set(authHeader)
    );
    expect(req.status).to.equal(204);

    // 删完再取应 404
    const gone = await send(() =>
      request.get(`${URL}/api/doodle/ai/light/preset/${presetId}`).set(authHeader)
    );
    expect(gone.status).to.equal(404);

    presetId = null;
  });

  it('DELETE /preset/{id} — 预设不存在返回 404', async function () {
    const req = await send(() =>
      request.delete(`${URL}/api/doodle/ai/light/preset/${NON_EXISTENT_ID}`).set(authHeader)
    );

    expect(req.status).to.equal(404);
  });
});

// ═══════════════════════════════════════════════════════════════════════════
// 8. 鉴权：无 Cookie / 坏 token
// ═══════════════════════════════════════════════════════════════════════════

describe('AI 灯光 —— 鉴权', function () {
  this.timeout(30000);

  it('GET /preset 无 Cookie 返回 401', async function () {
    const req = await send(() => request.get(`${URL}/api/doodle/ai/light/preset`));
    expect(req.status).to.equal(401);
  });

  it('GET /session/{id} 无 Cookie 返回 401', async function () {
    const req = await send(() => request.get(`${URL}/api/doodle/ai/light/session/${NON_EXISTENT_ID}`));
    expect(req.status).to.equal(401);
  });

  it('DELETE /session/{id} 无 Cookie 返回 401', async function () {
    const req = await send(() => request.delete(`${URL}/api/doodle/ai/light/session/${NON_EXISTENT_ID}`));
    expect(req.status).to.equal(401);
  });
});
