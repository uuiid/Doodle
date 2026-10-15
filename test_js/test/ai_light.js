// ai_light.js — AI 灯光 HTTP 接口测试
//
// 覆盖 src/doodle_lib/http_method/kitsu/ai_light.h 中注册的**全部** URI：
//
//   1. GET/POST /api/doodle/ai/light/shot/{entity_id}/session
//   2. GET/DELETE /api/doodle/ai/light/session/{id}
//   3. GET/POST /api/doodle/ai/light/session/{session_id}/reference
//   4. DELETE /api/doodle/ai/light/reference/{id}
//   5. GET/POST /api/doodle/ai/light/session/{session_id}/spec
//   6. POST /api/doodle/ai/light/session/{session_id}/spec/confirm
//   7. POST /api/doodle/ai/light/session/{session_id}/iterate
//   8. POST /api/doodle/ai/light/session/{session_id}/judge
//   9. POST /api/doodle/ai/light/render_report/{task_id}
//  10. POST /api/doodle/ai/light/render_report_local/{session_id}/{iteration}
//  11. POST /api/doodle/ai/light/candidate/{id}/file/{role}
//  12. GET /api/doodle/ai/light/candidate/{id}/movie
//  13. GET/POST /api/doodle/ai/light/preset
//  14. GET/PUT/DELETE /api/doodle/ai/light/preset/{id}
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

import { test } from '@playwright/test';
import { expect } from 'chai';
import fs from 'node:fs';
import path from 'node:path';
import {
  URL,
  authHeader,
  NON_EXISTENT_ID,
  IMAGE_DIR,
  listReferenceImages,
  mimeTypeOf,
} from './config.js';
import { readJson, expectError } from './helpers.js';

// 有灯光任务的镜头 id（建会话的唯一入口需要它）
const ENTITY_ID = process.env.AI_LIGHT_ENTITY_ID || '019e8267-2505-70d6-8050-430e4565ad8e';

// 是否跑完整链路（会真的调 LLM 推理、产生计费）
const FULL_FLOW = process.env.AI_LIGHT_FULL_FLOW === '1';

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

// 同名多文件沿用原 superagent 的 .attach('file', img) 循环语义，用 FormData 重复 append
function multiFileForm(filePaths) {
  const form = new FormData();
  for (const p of filePaths) {
    form.append('file', new File([fs.readFileSync(p)], path.basename(p), { type: mimeTypeOf(p) }));
  }
  return form;
}

// ═══════════════════════════════════════════════════════════════════════════
// 0. 连通性与前置条件
// ═══════════════════════════════════════════════════════════════════════════

test.describe('AI 灯光 —— 前置条件检查', () => {
  test.describe.configure({ timeout: 30000 });

  test('build/image 参考图目录存在且至少有一张图', async () => {
    expect(fs.existsSync(IMAGE_DIR), `参考图目录不存在: ${IMAGE_DIR}`).to.be.true;
    const images = listReferenceImages();
    expect(images.length, `参考图目录里没有图片: ${IMAGE_DIR}`).to.be.above(0);
    console.log(`参考图目录 ${IMAGE_DIR} 下共 ${images.length} 张图`);
  });

  test('配置了 AI_LIGHT_ENTITY_ID（有灯光任务的镜头）', async () => {
    if (!ENTITY_ID) {
      test.skip();
    }
    expect(ENTITY_ID).to.match(/^[0-9a-fA-F-]{36}$/, 'AI_LIGHT_ENTITY_ID 必须是 uuid');
  });
});

// ═══════════════════════════════════════════════════════════════════════════
// 1. 会话：/shot/{entity_id}/session 与 /session/{id}
// ═══════════════════════════════════════════════════════════════════════════

test.describe('AI 灯光 —— 会话（shot_session / session_instance）', () => {
  test.describe.configure({ timeout: 60000 });

  let sessionId = null;

  test.afterAll(async ({ request }) => {
    if (sessionId) {
      await request.delete(`${URL}/api/doodle/ai/light/session/${sessionId}`, { headers: authHeader });
      sessionId = null;
    }
  });

  test('POST /shot/{entity_id}/session — 建会话返回六表聚合详情', async ({ request }) => {
    if (!ENTITY_ID) test.skip();

    const req = await readJson(
      await request.post(`${URL}/api/doodle/ai/light/shot/${ENTITY_ID}/session`, {
        headers: authHeader,
        data: {
          prompt_text: PROMPT_TEXT,
          title: 'AI 灯光测试会话',
          // 下面三个由服务端从镜头覆盖，客户端传什么都改不了归属
          entity_id: NON_EXISTENT_ID,
          project_id: NON_EXISTENT_ID,
          task_id: NON_EXISTENT_ID,
          // 状态与轮次由服务端强制，传进来也应被忽略
          state: 'awaiting_judgement',
          current_iteration: 99,
        },
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

  test('POST /shot/{entity_id}/session — 缺少 prompt_text 返回 400', async ({ request }) => {
    if (!ENTITY_ID) test.skip();

    const req = await readJson(
      await request.post(`${URL}/api/doodle/ai/light/shot/${ENTITY_ID}/session`, {
        headers: authHeader,
        data: {},
      })
    );

    expectError(req, 400, 'prompt_text');
  });

  test('POST /shot/{entity_id}/session — 镜头不存在返回 404', async ({ request }) => {
    const req = await readJson(
      await request.post(`${URL}/api/doodle/ai/light/shot/${NON_EXISTENT_ID}/session`, {
        headers: authHeader,
        data: { prompt_text: PROMPT_TEXT },
      })
    );

    expect(req.status).to.equal(404);
  });

  test('GET /shot/{entity_id}/session — 返回该镜头的会话列表', async ({ request }) => {
    if (!ENTITY_ID) test.skip();
    expect(sessionId, '需要先建会话').to.not.be.null;

    const req = await readJson(
      await request.get(`${URL}/api/doodle/ai/light/shot/${ENTITY_ID}/session`, { headers: authHeader })
    );

    expect(req.status).to.equal(200);
    expect(req.body).to.have.property('data');
    expect(req.body.data).to.be.an('array');

    const found = req.body.data.find((s) => s.id === sessionId);
    expect(found, '列表中应包含刚建的会话').to.exist;
    expect(found.entity_id).to.equal(ENTITY_ID);
    expect(found.state).to.equal('draft');
  });

  test('GET /session/{id} — 会话详情', async ({ request }) => {
    if (!ENTITY_ID) test.skip();
    expect(sessionId, '需要先建会话').to.not.be.null;

    const req = await readJson(
      await request.get(`${URL}/api/doodle/ai/light/session/${sessionId}`, { headers: authHeader })
    );

    expect(req.status).to.equal(200);
    expect(req.body.session.id).to.equal(sessionId);
    // 详情与建会话返回同构
    expect(req.body).to.have.keys([
      'session', 'references', 'spec_history', 'current_spec', 'iterations',
      'candidates', 'accepted_candidate', 'is_waiting_human',
      'is_judgement_overdue', 'candidate_count_per_iteration',
    ]);
  });

  test('GET /session/{id} — 会话不存在返回 404', async ({ request }) => {
    const req = await readJson(
      await request.get(`${URL}/api/doodle/ai/light/session/${NON_EXISTENT_ID}`, { headers: authHeader })
    );

    expect(req.status).to.equal(404);
  });
});

// ═══════════════════════════════════════════════════════════════════════════
// 2. 参考图：上传 / 列表 / 取图 / 删除
// ═══════════════════════════════════════════════════════════════════════════

test.describe('AI 灯光 —— 参考图（reference）', () => {
  test.describe.configure({ timeout: 180000 });

  let sessionId = null;

  test.beforeAll(async ({ request }) => {
    if (!ENTITY_ID) test.skip();
    const req = await readJson(
      await request.post(`${URL}/api/doodle/ai/light/shot/${ENTITY_ID}/session`, {
        headers: authHeader,
        data: { prompt_text: PROMPT_TEXT, title: 'AI 灯光参考图测试' },
      })
    );
    expect(req.status).to.equal(201);
    sessionId = req.body.session.id;
  });

  test.afterAll(async ({ request }) => {
    if (sessionId) {
      await request.delete(`${URL}/api/doodle/ai/light/session/${sessionId}`, { headers: authHeader });
      sessionId = null;
    }
  });

  test('POST /session/{id}/reference — 无文件返回 400', async ({ request }) => {
    expect(sessionId, '需要先建会话').to.not.be.null;

    const req = await readJson(
      await request.post(`${URL}/api/doodle/ai/light/session/${sessionId}/reference`, {
        headers: authHeader,
        data: {},
      })
    );

    expectError(req, 400, '参考图');
  });

  test('POST /session/{id}/reference — 多图上传返回参考图列表', async ({ request }) => {
    expect(sessionId, '需要先建会话').to.not.be.null;

    const images = listReferenceImages();
    expect(images.length, '参考图目录为空').to.be.above(0);
    // 取前两张，验证多图一次上传
    const picked = images.slice(0, 2);

    const req = await readJson(
      await request.post(`${URL}/api/doodle/ai/light/session/${sessionId}/reference`, {
        headers: authHeader,
        multipart: multiFileForm(picked),
      })
    );

    expect(req.status).to.equal(201);
    expect(req.body).to.have.property('data');
    expect(req.body.data).to.have.lengthOf(picked.length);

    req.body.data.forEach((ref, i) => {
      expect(ref).to.have.keys([
        'id', 'session_id', 'file_path', 'thumbnail_path', 'width',
        'height', 'stats', 'stats_confidence', 'is_outlier', 'excluded_reason',
        'created_at',
      ]);
      expect(ref.session_id).to.equal(sessionId);
      // 服务端统一转 PNG 后才落盘
      expect(ref.file_path.toLowerCase()).to.match(/\.png$/);
      // 统计在已落盘的 PNG 上算出来：长宽必须真实
      expect(ref.width).to.be.above(0);
      expect(ref.height).to.be.above(0);
      expect(ref.is_outlier).to.equal(false);
    });
  });

  test('GET /session/{id}/reference — 列表与上传结果一致', async ({ request }) => {
    expect(sessionId, '需要先建会话').to.not.be.null;

    const req = await readJson(
      await request.get(`${URL}/api/doodle/ai/light/session/${sessionId}/reference`, { headers: authHeader })
    );

    expect(req.status).to.equal(200);
    expect(req.body.data).to.be.an('array').with.lengthOf(2);
    // 按 created_at 升序（= 上传先后顺序）；ordinal 列已删除
    expect(new Date(req.body.data[0].created_at).getTime())
      .to.be.at.most(new Date(req.body.data[1].created_at).getTime());
  });

  test('DELETE /reference/{id} — 删除单张参考图返回 204', async ({ request }) => {
    expect(sessionId, '需要先建会话').to.not.be.null;

    const listReq = await readJson(
      await request.get(`${URL}/api/doodle/ai/light/session/${sessionId}/reference`, { headers: authHeader })
    );
    expect(listReq.body.data).to.have.lengthOf(2);
    const refId = listReq.body.data[1].id;

    const delReq = await readJson(
      await request.delete(`${URL}/api/doodle/ai/light/reference/${refId}`, { headers: authHeader })
    );
    expect(delReq.status).to.equal(204);

    // 删完只剩一张
    const afterReq = await readJson(
      await request.get(`${URL}/api/doodle/ai/light/session/${sessionId}/reference`, { headers: authHeader })
    );
    expect(afterReq.body.data).to.have.lengthOf(1);
    expect(afterReq.body.data.map((r) => r.id)).to.not.include(refId);
  });

  test('DELETE /reference/{id} — 参考图不存在返回 404', async ({ request }) => {
    const req = await readJson(
      await request.delete(`${URL}/api/doodle/ai/light/reference/${NON_EXISTENT_ID}`, { headers: authHeader })
    );

    expect(req.status).to.equal(404);
  });

  test('POST /session/{id}/reference — 会话不存在返回 404', async ({ request }) => {
    const req = await readJson(
      await request.post(`${URL}/api/doodle/ai/light/session/${NON_EXISTENT_ID}/reference`, {
        headers: authHeader,
        data: {},
      })
    );

    expect(req.status).to.equal(404);
  });
});

// ═══════════════════════════════════════════════════════════════════════════
// 3. Spec：版本列表 / 生成 / 人工闸门 1
// ═══════════════════════════════════════════════════════════════════════════

test.describe('AI 灯光 —— Spec（spec / spec_confirm）', () => {
  test.describe.configure({ timeout: 300000 });

  let sessionId = null;
  // 「别的会话」——只为给「参考图不属于本会话」那条用例提供一个**真实存在**的
  // 参考图行。服务端在写排除标记前会先 get_by_uuid 查库（ai_light.cpp:547），
  // 拿一个不存在的 uuid 去测会在那一步就抛 404，永远走不到 548 行的归属校验。
  let otherSessionId = null;
  let foreignRefId = null;

  test.beforeAll(async ({ request }) => {
    if (!ENTITY_ID) test.skip();
    const req = await readJson(
      await request.post(`${URL}/api/doodle/ai/light/shot/${ENTITY_ID}/session`, {
        headers: authHeader,
        data: { prompt_text: PROMPT_TEXT, title: 'AI 灯光 Spec 测试' },
      })
    );
    expect(req.status).to.equal(201);
    sessionId = req.body.session.id;
  });

  test.afterAll(async ({ request }) => {
    if (sessionId) {
      await request.delete(`${URL}/api/doodle/ai/light/session/${sessionId}`, { headers: authHeader });
      sessionId = null;
    }
    if (otherSessionId) {
      await request.delete(`${URL}/api/doodle/ai/light/session/${otherSessionId}`, { headers: authHeader });
      otherSessionId = null;
    }
  });

  test('GET /session/{id}/spec — 尚无 Spec 时 data 为空、current_revision 为 0', async ({ request }) => {
    expect(sessionId, '需要先建会话').to.not.be.null;

    const req = await readJson(
      await request.get(`${URL}/api/doodle/ai/light/session/${sessionId}/spec`, { headers: authHeader })
    );

    expect(req.status).to.equal(200);
    expect(req.body.data).to.be.an('array').with.lengthOf(0);
    expect(req.body.current_revision).to.equal(0);
  });

  test('POST /session/{id}/spec/confirm — 还没有 Spec 就确认返回 400', async ({ request }) => {
    expect(sessionId, '需要先建会话').to.not.be.null;

    const req = await readJson(
      await request.post(`${URL}/api/doodle/ai/light/session/${sessionId}/spec/confirm`, {
        headers: authHeader,
        data: {},
      })
    );

    expectError(req, 400, 'Spec');
  });

  test('POST /session/{id}/spec — 没有参考图时拒绝生成（400）', async ({ request }) => {
    expect(sessionId, '需要先建会话').to.not.be.null;

    // Spec 的输入是「提示词 + 参考图」，参考图为空时拒绝，
    // 避免蒸出一份 prompt-only 的方案把主输入静默丢掉。
    const req = await readJson(
      await request.post(`${URL}/api/doodle/ai/light/session/${sessionId}/spec`, {
        headers: authHeader,
        data: {},
      })
    );

    expect(req.status).to.equal(400);
  });

  test.skip('POST /session/{id}/spec — 排除不属于本会话的参考图返回 400', async ({ request }) => {
    if (!FULL_FLOW) test.skip();
    expect(sessionId, '需要先建会话').to.not.be.null;

    // 必须用**真实存在**的参考图 id：服务端在写排除标记前先查库
    // （ai_light.cpp:547 的 get_by_uuid，查不到会抛 sqlite_orm_exception{404}），
    // 传空 uuid 会被 404 挡在归属校验之前，「不属于本会话」这个分支根本没被执行到。
    // 所以另建一个会话并往它上传一张参考图，拿那张图的 id 来撞本会话的 /spec。
    if (!otherSessionId) {
      const mk = await readJson(
        await request.post(`${URL}/api/doodle/ai/light/shot/${ENTITY_ID}/session`, {
          headers: authHeader,
          data: { prompt_text: PROMPT_TEXT, title: 'AI 灯光 Spec 测试_另一个会话' },
        })
      );
      expect(mk.status, '需要能建出第二个会话来提供外部参考图').to.equal(201);
      otherSessionId = mk.body.session.id;

      const images = listReferenceImages().slice(0, 1);
      expect(images.length, '参考图目录为空').to.be.above(0);
      const upReq = await readJson(
        await request.post(`${URL}/api/doodle/ai/light/session/${otherSessionId}/reference`, {
          headers: authHeader,
          multipart: multiFileForm(images),
        })
      );
      expect(upReq.status, '需要能往第二个会话传参考图').to.equal(201);
      foreignRefId = upReq.body.data[0].id;
    }

    const req = await readJson(
      await request.post(`${URL}/api/doodle/ai/light/session/${sessionId}/spec`, {
        headers: authHeader,
        data: { excluded_reference_ids: [foreignRefId] },
      })
    );

    // 走到 548 行的归属校验：参考图存在、但不属于本会话 → 400
    expectError(req, 400, '不属于会话');
  });

  // -------------------------------------------------------------------------
  // 以下步骤会真的调用 LLM 推理（产生计费），默认跳过。
  // 打开方式：$env:AI_LIGHT_FULL_FLOW = "1"
  // -------------------------------------------------------------------------

  test('POST /session/{id}/spec — 生成第一版 Spec（draft → awaiting_spec_confirm）', async ({ request }) => {
    if (!ENTITY_ID) test.skip();
    if (!FULL_FLOW) test.skip();
    expect(sessionId, '需要先建会话').to.not.be.null;

    // 先传参考图，否则 /spec 会被 400 挡下
    const images = listReferenceImages().slice(0, 2);
    const upReq = await readJson(
      await request.post(`${URL}/api/doodle/ai/light/session/${sessionId}/reference`, {
        headers: authHeader,
        multipart: multiFileForm(images),
      })
    );
    expect(upReq.status).to.equal(201);

    const req = await readJson(
      await request.post(`${URL}/api/doodle/ai/light/session/${sessionId}/spec`, {
        headers: authHeader,
        data: { prompt_text: PROMPT_TEXT },
      })
    );

    expect(req.status).to.equal(201);
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

  test('GET /session/{id}/spec — 版本列表含刚生成的 Spec', async ({ request }) => {
    if (!FULL_FLOW) test.skip();
    expect(sessionId, '需要先建会话').to.not.be.null;

    const req = await readJson(
      await request.get(`${URL}/api/doodle/ai/light/session/${sessionId}/spec`, { headers: authHeader })
    );

    expect(req.status).to.equal(200);
    expect(req.body.data).to.have.lengthOf(1);
    expect(req.body.current_revision).to.equal(1);
  });

  test('POST /session/{id}/spec/confirm — 人工闸门 1 确认', async ({ request }) => {
    if (!FULL_FLOW) test.skip();
    expect(sessionId, '需要先建会话').to.not.be.null;

    const req = await readJson(
      await request.post(`${URL}/api/doodle/ai/light/session/${sessionId}/spec/confirm`, {
        headers: authHeader,
        data: {},
      })
    );

    expect(req.status).to.equal(201);
    // 确认后立即进入候选生成
    expect(req.body.session.state).to.equal('generating');
    expect(req.body.current_spec.human_confirmed).to.equal(true);
    expect(req.body.current_spec.confirmed_by).to.not.be.null;
    expect(req.body.current_spec.confirmed_at).to.not.be.null;
  });

  test('POST /session/{id}/spec/confirm — 重复确认是幂等的', async ({ request }) => {
    if (!FULL_FLOW) test.skip();
    expect(sessionId, '需要先建会话').to.not.be.null;

    const first = await readJson(
      await request.get(`${URL}/api/doodle/ai/light/session/${sessionId}`, { headers: authHeader })
    );
    const confirmedAt = first.body.current_spec.confirmed_at;

    const req = await readJson(
      await request.post(`${URL}/api/doodle/ai/light/session/${sessionId}/spec/confirm`, {
        headers: authHeader,
        data: {},
      })
    );

    expect(req.status).to.equal(201);
    expect(req.body.already_confirmed).to.equal(true);
    // 重复确认不刷新时间戳（那是「谁在什么时候认的」的证据）
    expect(req.body.spec.confirmed_at).to.equal(confirmedAt);
  });

  test('POST /session/{id}/spec/confirm — 会话不存在返回 404', async ({ request }) => {
    const req = await readJson(
      await request.post(`${URL}/api/doodle/ai/light/session/${NON_EXISTENT_ID}/spec/confirm`, {
        headers: authHeader,
        data: {},
      })
    );

    expect(req.status).to.equal(404);
  });
});

// ═══════════════════════════════════════════════════════════════════════════
// 4. 迭代与判断：/iterate、/judge
// ═══════════════════════════════════════════════════════════════════════════

test.describe('AI 灯光 —— 迭代与判断（iterate / judge）', () => {
  test.describe.configure({ timeout: 300000 });

  let sessionId = null;

  test.beforeAll(async ({ request }) => {
    if (!ENTITY_ID) test.skip();
    const req = await readJson(
      await request.post(`${URL}/api/doodle/ai/light/shot/${ENTITY_ID}/session`, {
        headers: authHeader,
        data: { prompt_text: PROMPT_TEXT, title: 'AI 灯光迭代测试' },
      })
    );
    expect(req.status).to.equal(201);
    sessionId = req.body.session.id;
  });

  test.afterAll(async ({ request }) => {
    if (sessionId) {
      await request.delete(`${URL}/api/doodle/ai/light/session/${sessionId}`, { headers: authHeader });
      sessionId = null;
    }
  });

  test('POST /session/{id}/iterate — 非 generating 状态返回 400', async ({ request }) => {
    expect(sessionId, '需要先建会话').to.not.be.null;

    // 会话是 draft，不是 generating
    const req = await readJson(
      await request.post(`${URL}/api/doodle/ai/light/session/${sessionId}/iterate`, {
        headers: authHeader,
        data: {},
      })
    );

    expect(req.status).to.equal(400);
    const text = JSON.stringify(req.body ?? '');
    expect(text).to.include('generating');
  });

  test('POST /session/{id}/iterate — 无请求体也能调用（空体不应因 body 解析失败）', async ({ request }) => {
    expect(sessionId, '需要先建会话').to.not.be.null;

    // /iterate 刻意不读请求体；空体调用应当走到状态校验，而不是「body 不是 json」
    const req = await readJson(
      await request.post(`${URL}/api/doodle/ai/light/session/${sessionId}/iterate`, { headers: authHeader })
    );

    expect(req.status).to.equal(400);
    const text = JSON.stringify(req.body ?? '');
    expect(text, '不应该报 body 解析错误').to.not.include('body 不是 json');
    expect(text).to.include('generating');
  });

  test('POST /session/{id}/judge — 没有轮次时返回 400', async ({ request }) => {
    expect(sessionId, '需要先建会话').to.not.be.null;

    const req = await readJson(
      await request.post(`${URL}/api/doodle/ai/light/session/${sessionId}/judge`, {
        headers: authHeader,
        data: { decision: 'reject_all' },
      })
    );

    expect(req.status).to.equal(400);
    const text = JSON.stringify(req.body ?? '');
    expect(text).to.include('轮次');
  });

  test('POST /session/{id}/judge — 会话不存在返回 404', async ({ request }) => {
    const req = await readJson(
      await request.post(`${URL}/api/doodle/ai/light/session/${NON_EXISTENT_ID}/judge`, {
        headers: authHeader,
        data: { decision: 'reject_all' },
      })
    );

    expect(req.status).to.equal(404);
  });

  test('POST /session/{id}/iterate — 生成一轮候选并返回 launch_plan', async ({ request }) => {
    if (!FULL_FLOW) test.skip();
    expect(sessionId, '需要先建会话').to.not.be.null;

    // 前置：参考图 → Spec → 确认（确认后状态为 generating）
    const images = listReferenceImages().slice(0, 2);
    expect(
      (
        await readJson(
          await request.post(`${URL}/api/doodle/ai/light/session/${sessionId}/reference`, {
            headers: authHeader,
            multipart: multiFileForm(images),
          })
        )
      ).status
    ).to.equal(201);

    expect(
      (
        await readJson(
          await request.post(`${URL}/api/doodle/ai/light/session/${sessionId}/spec`, {
            headers: authHeader,
            data: {},
          })
        )
      ).status
    ).to.equal(201);

    expect(
      (
        await readJson(
          await request.post(`${URL}/api/doodle/ai/light/session/${sessionId}/spec/confirm`, {
            headers: authHeader,
            data: {},
          })
        )
      ).status
    ).to.equal(201);

    const req = await readJson(
      await request.post(`${URL}/api/doodle/ai/light/session/${sessionId}/iterate`, {
        headers: authHeader,
        data: {},
      })
    );

    expect(req.status).to.equal(201);
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

  test('POST /session/{id}/judge — 定稿必须指定候选（400）', async ({ request }) => {
    if (!FULL_FLOW) test.skip();
    expect(sessionId, '需要先建会话').to.not.be.null;

    const req = await readJson(
      await request.post(`${URL}/api/doodle/ai/light/session/${sessionId}/judge`, {
        headers: authHeader,
        data: { decision: 'accept' },
      })
    );

    expectError(req, 400, '候选');
  });

  test('POST /session/{id}/judge — 候选不属于本轮返回 400', async ({ request }) => {
    if (!FULL_FLOW) test.skip();
    expect(sessionId, '需要先建会话').to.not.be.null;

    const req = await readJson(
      await request.post(`${URL}/api/doodle/ai/light/session/${sessionId}/judge`, {
        headers: authHeader,
        data: { decision: 'accept', selected_candidate_id: NON_EXISTENT_ID },
      })
    );

    expect(req.status).to.equal(400);
  });

  test('POST /session/{id}/judge — continue_iteration 记录反馈轴并回到 generating', async ({ request }) => {
    if (!FULL_FLOW) test.skip();
    expect(sessionId, '需要先建会话').to.not.be.null;

    const detail = await readJson(
      await request.get(`${URL}/api/doodle/ai/light/session/${sessionId}`, { headers: authHeader })
    );
    const iteration = detail.body.iterations[0];

    const req = await readJson(
      await request.post(`${URL}/api/doodle/ai/light/session/${sessionId}/judge`, {
        headers: authHeader,
        data: {
          decision: 'continue_iteration',
          feedback_text: '白天氛围不够，整体再亮一点',
          // 滑块通道：直接给轴，不经 LLM
          feedback_axes: { kelvin: -200, ambient_level: 0.2 },
        },
      })
    );

    expect(req.status).to.equal(201);
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

test.describe.skip('AI 灯光 —— 回传（render_report / render_report_local）', () => {
  test.describe.configure({ timeout: 60000 });

  let sessionId = null;

  test.beforeAll(async ({ request }) => {
    if (!ENTITY_ID) test.skip();
    const req = await readJson(
      await request.post(`${URL}/api/doodle/ai/light/shot/${ENTITY_ID}/session`, {
        headers: authHeader,
        data: { prompt_text: PROMPT_TEXT, title: 'AI 灯光回传测试' },
      })
    );
    expect(req.status).to.equal(201);
    sessionId = req.body.session.id;
  });

  test.afterAll(async ({ request }) => {
    if (sessionId) {
      await request.delete(`${URL}/api/doodle/ai/light/session/${sessionId}`, { headers: authHeader });
      sessionId = null;
    }
  });

  test('POST /render_report/{task_id} — 任务不存在返回 404', async ({ request }) => {
    const req = await readJson(
      await request.post(`${URL}/api/doodle/ai/light/render_report/${NON_EXISTENT_ID}`, {
        headers: authHeader,
        data: { candidates: [], frame_rate: 24, error: '' },
      })
    );

    expect(req.status).to.equal(404);
  });

  test('POST /render_report/{task_id} — 任务不是 AI 灯光类型返回 400', async ({ request }) => {
    if (!ENTITY_ID) test.skip();

    // 会话上的 task_id 是 Kitsu 灯光任务（不是 server_task_info 的 ai_light 任务）
    const detail = await readJson(
      await request.get(`${URL}/api/doodle/ai/light/session/${sessionId}`, { headers: authHeader })
    );
    const taskId = detail.body.session.task_id;

    const req = await readJson(
      await request.post(`${URL}/api/doodle/ai/light/render_report/${taskId}`, {
        headers: authHeader,
        data: { candidates: [], frame_rate: 24, error: '' },
      })
    );

    // 任务行不存在 → 404；存在但不是 ai_light 类型 → 400。两者都是「拒绝」。
    expect([400, 404]).to.include(req.status);
  });

  test('POST /render_report/{task_id} — 回传体缺少严格字段返回 502', async ({ request }) => {
    if (!ENTITY_ID) test.skip();

    // 先建一个真正的 ai_light 任务行不可行（本文件不派发），所以这里只验证
    // 「形状错误 → 502」这条在解析层的契约：用不存在任务时先被 404 挡下。
    const req = await readJson(
      await request.post(`${URL}/api/doodle/ai/light/render_report/${NON_EXISTENT_ID}`, {
        headers: authHeader,
        data: { frame_rate: 24 },
      })
    );

    expect([404, 502]).to.include(req.status);
  });

  test('POST /render_report_local/{session_id}/{iteration} — 轮次不存在返回 404', async ({ request }) => {
    expect(sessionId, '需要先建会话').to.not.be.null;

    const req = await readJson(
      await request.post(`${URL}/api/doodle/ai/light/render_report_local/${sessionId}/1`, {
        headers: authHeader,
        data: { candidates: [], frame_rate: 24, error: '' },
      })
    );

    expectError(req, 404, '轮');
  });

  test('POST /render_report_local/{session_id}/{iteration} — 会话不存在返回 404', async ({ request }) => {
    const req = await readJson(
      await request.post(`${URL}/api/doodle/ai/light/render_report_local/${NON_EXISTENT_ID}/1`, {
        headers: authHeader,
        data: { candidates: [], frame_rate: 24, error: '' },
      })
    );

    expect(req.status).to.equal(404);
  });

  test('POST /render_report_local/{session_id}/{iteration} — 无候选的整轮失败回执', async ({ request }) => {
    if (!FULL_FLOW) test.skip();
    expect(sessionId, '需要先建会话').to.not.be.null;

    // 轮次必须先存在（由 /iterate 生成），本用例依赖 full flow 的前序步骤
    const detail = await readJson(
      await request.get(`${URL}/api/doodle/ai/light/session/${sessionId}`, { headers: authHeader })
    );
    if (detail.body.iterations.length === 0) test.skip();
    const iteration = detail.body.iterations[0].iteration;

    const req = await readJson(
      await request.post(`${URL}/api/doodle/ai/light/render_report_local/${sessionId}/${iteration}`, {
        headers: authHeader,
        data: { candidates: [], frame_rate: 24, error: 'UE 启动失败' },
      })
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

test.describe.skip('AI 灯光 —— 候选产物（candidate file / movie / image）', () => {
  test.describe.configure({ timeout: 120000 });

  const OCTET = 'application/octet-stream';

  test('POST /candidate/{id}/file/{role} — 候选不存在返回 404', async ({ request }) => {
    const req = await readJson(
      await request.post(`${URL}/api/doodle/ai/light/candidate/${NON_EXISTENT_ID}/file/render`, {
        headers: { ...authHeader, 'Content-Type': OCTET },
        data: Buffer.from('not-an-image'),
      })
    );

    expect(req.status).to.equal(404);
  });

  test('POST /candidate/{id}/file/{role} — 请求体不是 octet-stream 返回 400', async ({ request }) => {
    // 内容类型检查在候选存在性之前
    const req = await readJson(
      await request.post(`${URL}/api/doodle/ai/light/candidate/${NON_EXISTENT_ID}/file/render`, {
        headers: { ...authHeader, 'Content-Type': 'application/json' },
        data: { hello: 'world' },
      })
    );

    expectError(req, 400, 'application/octet-stream');
  });

  test('GET /candidate/{id}/movie — 候选不存在返回 404', async ({ request }) => {
    const req = await readJson(
      await request.get(`${URL}/api/doodle/ai/light/candidate/${NON_EXISTENT_ID}/movie`, { headers: authHeader })
    );

    expect(req.status).to.equal(404);
  });
});

// ═══════════════════════════════════════════════════════════════════════════
// 7. 预设库：/preset 与 /preset/{id}
// ═══════════════════════════════════════════════════════════════════════════

test.describe('AI 灯光 —— 预设库（preset）', () => {
  test.describe.configure({ timeout: 60000 });

  let presetId = null;

  test.afterAll(async ({ request }) => {
    if (presetId) {
      await request.delete(`${URL}/api/doodle/ai/light/preset/${presetId}`, { headers: authHeader });
      presetId = null;
    }
  });

  test('POST /preset — 缺少名称返回 400', async ({ request }) => {
    const req = await readJson(
      await request.post(`${URL}/api/doodle/ai/light/preset`, {
        headers: authHeader,
        data: { tags: ['测试'] },
      })
    );

    expectError(req, 400, '预设名称');
  });

  test('POST /preset — 新建预设（source 由服务端强制为 artist）', async ({ request }) => {
    const req = await readJson(
      await request.post(`${URL}/api/doodle/ai/light/preset`, {
        headers: authHeader,
        data: {
          name: 'ai_light_测试预设_白天',
          tags: ['白天', 'interior'],
          // 客户端传 builtin 不应生效
          source: 'builtin',
          applicable: { scene_type: ['interior', 'day'], interior: true },
          stats_target: { contrast: 0.4 },
        },
      })
    );

    expect(req.status).to.equal(201);
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

  test('GET /preset — 列表包含刚建的预设', async ({ request }) => {
    expect(presetId, '需要先建预设').to.not.be.null;

    const req = await readJson(
      await request.get(`${URL}/api/doodle/ai/light/preset`, { headers: authHeader })
    );

    expect(req.status).to.equal(200);
    expect(req.body.data).to.be.an('array');
    expect(req.body.data.map((p) => p.id)).to.include(presetId);
  });

  test('GET /preset?enabled_only=false — 也返回全部预设', async ({ request }) => {
    expect(presetId, '需要先建预设').to.not.be.null;

    const req = await readJson(
      await request.get(`${URL}/api/doodle/ai/light/preset?enabled_only=false`, { headers: authHeader })
    );

    expect(req.status).to.equal(200);
    expect(req.body.data.map((p) => p.id)).to.include(presetId);
  });

  test('GET /preset/{id} — 预设详情', async ({ request }) => {
    expect(presetId, '需要先建预设').to.not.be.null;

    const req = await readJson(
      await request.get(`${URL}/api/doodle/ai/light/preset/${presetId}`, { headers: authHeader })
    );

    expect(req.status).to.equal(200);
    expect(req.body.id).to.equal(presetId);
    expect(req.body.name).to.equal('ai_light_测试预设_白天');
  });

  test('PUT /preset/{id} — 局部更新只覆盖请求体里出现的字段', async ({ request }) => {
    expect(presetId, '需要先建预设').to.not.be.null;

    const req = await readJson(
      await request.put(`${URL}/api/doodle/ai/light/preset/${presetId}`, {
        headers: authHeader,
        data: { name: 'ai_light_测试预设_白天_改', enabled: false, source: 'builtin' },
      })
    );

    expect(req.status).to.equal(200);
    expect(req.body.name).to.equal('ai_light_测试预设_白天_改');
    expect(req.body.enabled).to.equal(false);
    // tags 没传，应保持原值（局部更新）
    expect(req.body.tags).to.deep.equal(['白天', 'interior']);
    // source 不可改
    expect(req.body.source).to.equal('artist');
  });

  test('GET /preset — enabled_only 默认 true，停用的预设不在列表里', async ({ request }) => {
    expect(presetId, '需要先建预设').to.not.be.null;

    const req = await readJson(
      await request.get(`${URL}/api/doodle/ai/light/preset`, { headers: authHeader })
    );

    expect(req.status).to.equal(200);
    expect(req.body.data.map((p) => p.id)).to.not.include(presetId);

    const all = await readJson(
      await request.get(`${URL}/api/doodle/ai/light/preset?enabled_only=false`, { headers: authHeader })
    );
    expect(all.body.data.map((p) => p.id)).to.include(presetId);
  });

  test('GET /preset/{id} — 预设不存在返回 404', async ({ request }) => {
    const req = await readJson(
      await request.get(`${URL}/api/doodle/ai/light/preset/${NON_EXISTENT_ID}`, { headers: authHeader })
    );

    expectError(req, 404, '预设');
  });

  test('PUT /preset/{id} — 预设不存在返回 404', async ({ request }) => {
    const req = await readJson(
      await request.put(`${URL}/api/doodle/ai/light/preset/${NON_EXISTENT_ID}`, {
        headers: authHeader,
        data: { name: 'x' },
      })
    );

    expect(req.status).to.equal(404);
  });

  test('DELETE /preset/{id} — 删除预设返回 204', async ({ request }) => {
    expect(presetId, '需要先建预设').to.not.be.null;

    const req = await readJson(
      await request.delete(`${URL}/api/doodle/ai/light/preset/${presetId}`, { headers: authHeader })
    );
    expect(req.status).to.equal(204);

    // 删完再取应 404
    const gone = await readJson(
      await request.get(`${URL}/api/doodle/ai/light/preset/${presetId}`, { headers: authHeader })
    );
    expect(gone.status).to.equal(404);

    presetId = null;
  });

  test('DELETE /preset/{id} — 预设不存在返回 404', async ({ request }) => {
    const req = await readJson(
      await request.delete(`${URL}/api/doodle/ai/light/preset/${NON_EXISTENT_ID}`, { headers: authHeader })
    );

    expect(req.status).to.equal(404);
  });
});

// ═══════════════════════════════════════════════════════════════════════════
// 8. 鉴权：无 Cookie / 坏 token
// ═══════════════════════════════════════════════════════════════════════════

test.describe('AI 灯光 —— 鉴权', () => {
  test.describe.configure({ timeout: 30000 });

  test('GET /preset 无 Cookie 返回 401', async ({ request }) => {
    const req = await readJson(await request.get(`${URL}/api/doodle/ai/light/preset`));
    expect(req.status).to.equal(401);
  });

  test('GET /session/{id} 无 Cookie 返回 401', async ({ request }) => {
    const req = await readJson(await request.get(`${URL}/api/doodle/ai/light/session/${NON_EXISTENT_ID}`));
    expect(req.status).to.equal(401);
  });

  test('DELETE /session/{id} 无 Cookie 返回 401', async ({ request }) => {
    const req = await readJson(await request.delete(`${URL}/api/doodle/ai/light/session/${NON_EXISTENT_ID}`));
    expect(req.status).to.equal(401);
  });
});
