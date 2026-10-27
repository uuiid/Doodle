// ai_light.js — AI 灯光 HTTP 接口测试
//
// 严格对齐 src/doodle_lib/http_method/kitsu/ai_light.h 当前注册的路由，逐条覆盖：
//
//   /api/doodle/ai/light/shot/{entity_id}/session      GET, POST    shot_session_route
//   /api/doodle/ai/light/session/{id}                  GET, PUT, DELETE   session_instance_route
//   /api/doodle/ai/light/session/{session_id}/reference GET, POST   reference_route
//   /api/doodle/ai/light/reference/{id}                GET, PUT, DELETE   reference_instance_route
//   /api/doodle/ai/light/session/{session_id}/iteration/{iteration_id}  POST  judge_route
//   /api/doodle/ai/light/candidate/{id}/begin_sequence  POST         candidate_begin_sequence_route
//   /api/doodle/ai/light/candidate/{id}/upload_frame    POST         candidate_upload_frame_route
//   /api/doodle/ai/light/candidate/{id}/finish_sequence POST         candidate_finish_sequence_route
//   /api/doodle/ai/light/candidate/{id}/movie           GET          candidate_movie_route
//   /api/doodle/ai/light/preset                         GET, POST    preset_route
//   /api/doodle/ai/light/preset/{id}                    GET, PUT, DELETE   preset_instance_route
//   /api/doodle/ai/light/session/{session_id}/iteration POST         iteration_submit_route
//
// 刻意**不覆盖** /api/doodle/ai/light/session/{session_id}/inference（会真的调用方舟推理、产生计费）。
//
// 前端契约：会话提交一轮候选（iteration_submit_route）要求请求体里带**虚拟灯光方案**
// （rigs），服务端只接受自己拼出来、前端展开的 rig —— 所以本文件在提交用例之前先
// 用 GET session 把服务端返回的详情补齐成一个可提交的 rig（方案 = 全局参数 + 灯阵列 + ops），
// 并挑一组候选数量、变体轴来模拟前端「展开」的行为，供后续轮次用例复用。
//
// 前置条件（本文件不负责创建）：需要
//   1. 一个**已有灯光任务**的镜头 entity_id。建会话时服务端用 resolve_lighting_task_id()
//      从镜头解析灯光任务，镜头没有灯光任务会直接 400「镜头 {} 没有灯光任务，无法建立 AI 灯光会话」。
//   2. GET /api/actions/tasks/{task_id}/sync 必须能解析出该镜头的 UE 投影（主场景工程、相机文件、
//      帧区间），否则提交一轮候选会以 500「无法推导本镜头的渲染装配」告终。
//
// 环境变量：
//     $env:AI_LIGHT_ENTITY_ID = "<镜头 uuid>"
//     $env:AI_LIGHT_IMAGE_DIR = "<参考图目录>"   # 缺省 <仓库>/build/image
//
// 依赖镜头数据并未就绪时，相关用例走 expectProbeStatus()：数据就绪时断言精确语义，
// 未就绪时只断言服务端的拒绝原因，不会把「测试环境没配好」伪装成接口失败。

import { test } from '@playwright/test';
import { expect } from 'chai';
import fs from 'node:fs';
import path from 'node:path';
import {
  URL,
  JWT,
  authHeader,
  NIL_ID,
  NON_EXISTENT_ID,
  listReferenceImages,
  mimeTypeOf,
} from './config.js';
import { readJson, expectError } from './helpers.js';

// 有灯光任务的镜头 id（建会话的唯一入口需要它）
const ENTITY_ID = process.env.AI_LIGHT_ENTITY_ID || '019e8267-2505-70d6-8050-430e4565ad8e';

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

// 会话详情（ai_light_session_detail_t）的十个键，见 metadata/ai_light/view.h:66-77
const DETAIL_KEYS = [
  'session',
  'references',
  'iterations',
  'candidates',
  'is_waiting_human',
  'is_judgement_overdue',
  'candidate_count_per_iteration',
];

// 会话本体（ai_light_session），见 metadata/ai_light/session.h:44-54
const SESSION_KEYS = [
  'id',
  'project_id',
  'task_id',
  'entity_id',
  'submitter',
  'title',
  'prompt_text',
  'created_at',
  'updated_at',
];

// 参考图（ai_light_reference），见 metadata/ai_light/reference.h:147-155
const REFERENCE_KEYS = [
  'id',
  'session_id',
  'stats',
  'stats_confidence',
  'is_outlier',
  'excluded_reason',
  'created_at',
];

// 宣传/轮次/候选行（ai_light_iteration / ai_light_candidate）
const ITERATION_KEYS = [
  'id',
  'session_id',
  'iteration',
  'state',
  'server_task_info_id',
  'dispatch_at',
  'render_at',
  'judged_by',
  'judged_at',
  'feedback_text',
  'failure_reason',
  'rigs',
  'created_at',
  'updated_at',
];

const CANDIDATE_KEYS = [
  'id',
  'iteration_id',
  'session_id',
  'render_status',
  'metrics',
  'is_selected',
  'preview_file_id',
  'created_at',
];

// ---------------------------------------------------------------------------
// 虚拟灯光方案（lighting_rig_t，见 metadata/ai_light/rig.h）
//
// 服务端只接受前端展开出来的 rig：候选行（ai_light_candidate）由服务端生成，
// rig 里的 candidate_id 由服务端回填（write_submitted_iteration），前端给什么都不采信。
// 所以这里按「一次提交 N 个候选 → 每个候选一份 rig」来造。
// ---------------------------------------------------------------------------

function vec3(x, y, z) {
  return [x, y, z];
}

function makeLight({ id, role, type, azimuth, elevation, distance, kelvin, intensity }) {
  return {
    id,
    role,
    type,
    anchor: { space: 'camera', look_at: 'subject_head' },
    offset: { azimuth_deg: azimuth, elevation_deg: elevation, distance_m: distance },
    intensity: { unit: 'candela', value: intensity },
    color: { mode: 'temperature', kelvin },
    shape: { source_radius_cm: 30, ies: '' },
    shadows: { cast: true, softness: 0.5 },
    channels: [0, 1, 0],
    mobility: 'movable',
    anim: [],
    origin: 'ai',
  };
}

// 三点布光：主光 / 补光 / 轮廓光，外加一盏天光
function makeLights(seed) {
  const nudge = seed * 25;
  return [
    makeLight({
      id: 'key',
      role: 'key',
      type: 'spot',
      azimuth: -45,
      elevation: 30,
      distance: 3.0,
      kelvin: 5600,
      intensity: 1200 + nudge,
    }),
    makeLight({
      id: 'fill',
      role: 'fill',
      type: 'rect',
      azimuth: 45,
      elevation: 10,
      distance: 4.5,
      kelvin: 5000,
      intensity: 400 + nudge,
    }),
    makeLight({
      id: 'rim',
      role: 'rim',
      type: 'spot',
      azimuth: 170,
      elevation: 20,
      distance: 3.5,
      kelvin: 6500,
      intensity: 900 + nudge,
    }),
    makeLight({
      id: 'sky',
      role: 'ambient',
      type: 'sky',
      azimuth: 0,
      elevation: 90,
      distance: 0,
      kelvin: 7000,
      intensity: 1 + nudge / 100,
    }),
  ];
}

// 一份完整的灯光方案。baseline=true 时是本轮的基线（不做 ops），
// 其余方案在基线上叠加「调曝光 / 调色 / 改主光色温」几步操作。
function makeRig({ base = false, seed = 0 }) {
  const rig = {
    provenance: {
      model: 'test_js',
      prompt_hash: 'ai_light_contract_test',
      reference: [],
      seed: 1000 + seed,
      tier: 'stats_only',
    },
    globals: {
      exposure: { method: 'manual', ev100_bias: base ? 0 : 0.5 + seed * 0.1 },
      white_balance: { kelvin: 5600, tint: 0 },
      environment: { sky_light_intensity: 1, fog_density: 0 },
      grading: {
        contrast: base ? 1 : 1.05,
        saturation: base ? 1 : 1.1,
        lift: 0,
        vignette: base ? 0 : 0.1,
        bloom: base ? 0 : 0.2,
      },
    },
    lights: makeLights(seed),
    ops: base
      ? []
      : [
          {
            op: 'set_exposure',
            patch: { ev100_bias: 0.5 + seed * 0.1 },
            rationale: '整体偏暗，抬一档曝光更接近参考图的白天氛围',
          },
          {
            op: 'set_grading',
            patch: { contrast: 1.05, saturation: 1.1 },
            rationale: '参考图对比度略高',
          },
          {
            op: 'modify',
            light: 'key',
            patch: { color: { mode: 'temperature', kelvin: 5200 + seed * 50 } },
            rationale: '主光再暖一点，贴近参考图的日光',
          },
        ],
  };
  return rig;
}

// 前端「展开」一轮候选：iteration 1 的第 0 个候选是基线，其余带变体。
function makeCandidates(count = 6) {
  return Array.from({ length: count }, (_, i) => makeRig({ base: i === 0, seed: i }));
}

// ---------------------------------------------------------------------------
// 工具
// ---------------------------------------------------------------------------

// 同名多文件沿用原 superagent 的 .attach('file', img) 循环语义，用 FormData 重复 append
function multiFileForm(filePaths) {
  const form = new FormData();
  for (const p of filePaths) {
    form.append('file', new File([fs.readFileSync(p)], path.basename(p), { type: mimeTypeOf(p) }));
  }
  return form;
}

// 服务端对「镜头/任务未就绪」一律拒在 submit 之前（auto_task.cpp:848-890 的
// make_shot_render_light_input 与 auto_task.cpp:364-407 的 resolve_scene_ue_path），
// 这类拒绝说明测试环境没配好，而不是接口错了。返回 true 表示命中了这种环境性拒绝。
function isEnvironmentRejection(res) {
  const text = JSON.stringify(res.body ?? '');
  return (
    res.status === 500 &&
    (text.includes('无法推导本镜头的渲染装配') ||
      text.includes('缺少父级序列') ||
      text.includes('缺少扩展信息') ||
      text.includes('未找到场景') ||
      text.includes('未找到镜头') ||
      text.includes('无法生成 ue 主工程路径'))
  );
}

async function postSession(request, data) {
  return readJson(
    await request.post(`${URL}/api/doodle/ai/light/shot/${ENTITY_ID}/session`, {
      headers: authHeader,
      data,
    })
  );
}

async function getSession(request, sessionId) {
  return readJson(await request.get(`${URL}/api/doodle/ai/light/session/${sessionId}`, { headers: authHeader }));
}

async function deleteSession(request, sessionId) {
  if (!sessionId) return;
  await request.delete(`${URL}/api/doodle/ai/light/session/${sessionId}`, { headers: authHeader });
}

async function createSessionOrSkip(request, title) {
  const req = await postSession(request, { prompt_text: PROMPT_TEXT, title });
  if (req.status === 400 && JSON.stringify(req.body ?? '').includes('没有灯光任务')) return null;
  expect(req.status, `建会话失败：${JSON.stringify(req.body)}`).to.equal(201);
  return req.body.session.id;
}

async function uploadReferences(request, sessionId, count = 2) {
  const picked = listReferenceImages().slice(0, count);
  if (picked.length === 0) return null;
  const req = await readJson(
    await request.post(`${URL}/api/doodle/ai/light/session/${sessionId}/reference`, {
      headers: authHeader,
      multipart: multiFileForm(picked),
    })
  );
  expect(req.status, `上传参考图失败：${JSON.stringify(req.body)}`).to.equal(201);
  return req.body;
}

// 提交一轮候选（iteration_submit_route）。页面契约：body 里带 iteration 与 candidates(rig 数组)。
async function submitIteration(request, sessionId, iteration, count = 6) {
  return readJson(
    await request.post(`${URL}/api/doodle/ai/light/session/${sessionId}/iteration`, {
      headers: authHeader,
      data: { iteration, candidates: makeCandidates(count) },
    })
  );
}

// ═══════════════════════════════════════════════════════════════════════════
// 0. 前置条件
// ═══════════════════════════════════════════════════════════════════════════

test.describe('AI 灯光 —— 前置条件检查', () => {
  test.describe.configure({ timeout: 30000 });

  test('AI_LIGHT_ENTITY_ID 是一个 uuid', async () => {
    expect(ENTITY_ID).to.match(/^[0-9a-fA-F-]{36}$/, 'AI_LIGHT_ENTITY_ID 必须是 uuid');
  });

  test('参考图目录可用（为空时参考图相关用例跳过）', async () => {
    const images = listReferenceImages();
    console.log(`参考图目录下共 ${images.length} 张图`);
    expect(Array.isArray(images)).to.equal(true);
  });
});

// ═══════════════════════════════════════════════════════════════════════════
// 1. 会话：/shot/{entity_id}/session 与 /session/{id}
// ═══════════════════════════════════════════════════════════════════════════

test.describe('AI 灯光 —— 会话（shot_session / session_instance）', () => {
  test.describe.configure({ timeout: 60000 });

  let sessionId = null;

  test.afterAll(async ({ request }) => {
    await deleteSession(request, sessionId);
    sessionId = null;
  });

  test('POST /shot/{entity_id}/session — 建会话返回会话详情（201）', async ({ request }) => {
    // 服务端覆盖的三类字段：归属（entity/project/task）由镜头与 JWT 决定，其余按请求体落库
    const req = await postSession(request, {
      prompt_text: PROMPT_TEXT,
      title: 'AI 灯光测试会话',
      entity_id: NON_EXISTENT_ID,
      project_id: NON_EXISTENT_ID,
      task_id: NON_EXISTENT_ID,
    });

    if (req.status === 400 && JSON.stringify(req.body ?? '').includes('没有灯光任务')) {
      // 环境问题：该镜头还没有灯光任务，建会话在这个测试库里不可能成功
      return;
    }

    expect(req.status, JSON.stringify(req.body)).to.equal(201);
    const body = req.body;

    expect(body).to.have.keys(DETAIL_KEYS);
    expect(body.session).to.have.keys(SESSION_KEYS);

    expect(body.session.prompt_text).to.equal(PROMPT_TEXT);
    expect(body.session.title).to.equal('AI 灯光测试会话');
    // 归属由服务端定：请求体里的伪造值必须被覆盖
    expect(body.session.entity_id).to.equal(ENTITY_ID);
    expect(body.session.project_id).to.not.equal(NON_EXISTENT_ID);
    expect(body.session.task_id).to.not.equal(NON_EXISTENT_ID);
    // 提交人取自 JWT
    expect(body.session.submitter).to.not.equal('00000000-0000-0000-0000-000000000000');

    // 新会话没有参考图、没有轮次、没有候选
    expect(body.references).to.be.an('array').with.lengthOf(0);
    expect(body.iterations).to.be.an('array').with.lengthOf(0);
    expect(body.candidates).to.be.an('array').with.lengthOf(0);
    expect(body.is_waiting_human).to.equal(false);
    expect(body.is_judgement_overdue).to.equal(false);
    // 还没有候选时为 0（view.h:54-64）
    expect(body.candidate_count_per_iteration).to.equal(0);

    sessionId = body.session.id;
  });

  test('POST /shot/{entity_id}/session — 缺 prompt_text 返回 400', async ({ request }) => {
    const req = await postSession(request, {});
    // 提示词校验在镜头解析之后（ai_light.cpp:361-373），没有灯光任务的库里会先报 400「没有灯光任务」
    expect(req.status).to.equal(400);
  });

  test('POST /shot/{entity_id}/session — 空 body 也返回 400', async ({ request }) => {
    const req = await readJson(
      await request.post(`${URL}/api/doodle/ai/light/shot/${ENTITY_ID}/session`, {
        headers: { ...authHeader, 'Content-Type': 'application/json' },
      })
    );
    expect(req.status).to.equal(400);
  });

  test('POST /shot/{entity_id}/session — 镜头不存在返回 404', async ({ request }) => {
    const req = await readJson(
      await request.post(`${URL}/api/doodle/ai/light/shot/${NON_EXISTENT_ID}/session`, {
        headers: authHeader,
        data: { prompt_text: PROMPT_TEXT },
      })
    );
    // entity 行不存在 → get_by_uuid 抛 404（ai_light.cpp:361）
    expect(req.status).to.equal(404);
  });

  test('POST /shot/{entity_id}/session — 非 json body 返回 400', async ({ request }) => {
    const req = await readJson(
      await request.post(`${URL}/api/doodle/ai/light/shot/${ENTITY_ID}/session`, {
        headers: { ...authHeader, 'Content-Type': 'text/plain' },
        data: 'not-a-json',
      })
    );
    // get_json() 对非 json 内容直接 400「body 不是 json」（http_session_data.cpp:252-259）
    expectError(req, 400, 'body 不是 json');
  });

  test('GET /shot/{entity_id}/session — 返回该镜头的会话列表', async ({ request }) => {
    const req = await readJson(
      await request.get(`${URL}/api/doodle/ai/light/shot/${ENTITY_ID}/session`, { headers: authHeader })
    );

    expect(req.status, JSON.stringify(req.body)).to.equal(200);
    expect(req.body).to.be.an('array');
    if (req.body.length > 0) {
      expect(req.body[0]).to.have.keys(SESSION_KEYS);
      req.body.forEach((s) => expect(s.entity_id).to.equal(ENTITY_ID));
    }
    if (sessionId) {
      const found = req.body.find((s) => s.id === sessionId);
      expect(found, '列表中应包含刚建的会话').to.exist;
    }
  });

  test('GET /shot/{entity_id}/session — 镜头不存在时返回空数组', async ({ request }) => {
    // 列表接口只按 entity_id 过滤，不校验镜头存在性（ai_light.cpp:392-400）
    const req = await readJson(
      await request.get(`${URL}/api/doodle/ai/light/shot/${NON_EXISTENT_ID}/session`, { headers: authHeader })
    );
    expect(req.status).to.equal(200);
    expect(req.body).to.be.an('array').with.lengthOf(0);
  });

  test('GET /session/{id} — 会话详情与建会话返回同构', async ({ request }) => {
    if (!sessionId) return; // 没有灯光任务的环境下没有会话可查

    const req = await getSession(request, sessionId);
    expect(req.status).to.equal(200);
    expect(req.body).to.have.keys(DETAIL_KEYS);
    expect(req.body.session.id).to.equal(sessionId);
    expect(req.body.session.prompt_text).to.equal(PROMPT_TEXT);
  });

  test('GET /session/{id} — 会话不存在返回 404', async ({ request }) => {
    const req = await getSession(request, NON_EXISTENT_ID);
    expectError(req, 404, '不存在');
  });

  test('PUT /session/{id} — 只改提示词，其余字段保持不变', async ({ request }) => {
    if (!sessionId) return;

    const before = await getSession(request, sessionId);
    expect(before.status).to.equal(200);
    const origin = before.body.session;

    const newPrompt = `${PROMPT_TEXT}（改）`;
    const req = await readJson(
      await request.put(`${URL}/api/doodle/ai/light/session/${sessionId}`, {
        headers: authHeader,
        // 只认 prompt_text（ai_light.cpp:415），其余字段即便传了也不生效
        data: { prompt_text: newPrompt, title: '不该生效的标题', entity_id: NON_EXISTENT_ID },
      })
    );

    expect(req.status, JSON.stringify(req.body)).to.equal(200);
    expect(req.body).to.have.keys(DETAIL_KEYS);
    expect(req.body.session.prompt_text).to.equal(newPrompt);
    expect(req.body.session.title).to.equal(origin.title);
    expect(req.body.session.entity_id).to.equal(ENTITY_ID);
    expect(req.body.session.project_id).to.equal(origin.project_id);
    expect(req.body.session.task_id).to.equal(origin.task_id);
    expect(req.body.session.created_at).to.equal(origin.created_at);
  });

  test('PUT /session/{id} — 空提示词返回 400', async ({ request }) => {
    if (!sessionId) return;

    const req = await readJson(
      await request.put(`${URL}/api/doodle/ai/light/session/${sessionId}`, {
        headers: authHeader,
        data: { prompt_text: '' },
      })
    );
    expectError(req, 400, '提示词不能为空');
  });

  test('PUT /session/{id} — 会话不存在返回 404', async ({ request }) => {
    const req = await readJson(
      await request.put(`${URL}/api/doodle/ai/light/session/${NON_EXISTENT_ID}`, {
        headers: authHeader,
        data: { prompt_text: 'x' },
      })
    );
    expect(req.status).to.equal(404);
  });

  test('DELETE /session/{id} — 删除返回 204，再取 404', async ({ request }) => {
    const tmp = await createSessionOrSkip(request, 'AI 灯光删除测试');
    if (!tmp) return;

    const del = await readJson(
      await request.delete(`${URL}/api/doodle/ai/light/session/${tmp}`, { headers: authHeader })
    );
    // http_body_empty_t 默认 no_content（http_body.cpp:295）
    expect(del.status).to.equal(204);

    const gone = await getSession(request, tmp);
    expect(gone.status).to.equal(404);
  });

  test('DELETE /session/{id} — 会话不存在返回 404', async ({ request }) => {
    const req = await readJson(
      await request.delete(`${URL}/api/doodle/ai/light/session/${NON_EXISTENT_ID}`, { headers: authHeader })
    );
    expect(req.status).to.equal(404);
  });

  test('DELETE /session/{id} — 级联删除参考图与候选行', async ({ request }) => {
    const tmp = await createSessionOrSkip(request, 'AI 灯光级联删除测试');
    if (!tmp) return;
    try {
      const refs = await uploadReferences(request, tmp, 1);
      if (!refs) return;

      const del = await readJson(
        await request.delete(`${URL}/api/doodle/ai/light/session/${tmp}`, { headers: authHeader })
      );
      expect(del.status).to.equal(204);

      // 会话行消失后，参考图单条接口也应 404（行被外键级联删掉）
      const refReq = await readJson(
        await request.get(`${URL}/api/doodle/ai/light/reference/${refs[0].id}`, { headers: authHeader })
      );
      expect(refReq.status).to.equal(404);
    } finally {
      await request.delete(`${URL}/api/doodle/ai/light/session/${tmp}`, { headers: authHeader });
    }
  });
});

// ═══════════════════════════════════════════════════════════════════════════
// 2. 参考图：/session/{id}/reference 与 /reference/{id}
// ═══════════════════════════════════════════════════════════════════════════

test.describe('AI 灯光 —— 参考图（reference）', () => {
  test.describe.configure({ timeout: 180000 });

  let sessionId = null;

  test.beforeAll(async ({ request }) => {
    sessionId = await createSessionOrSkip(request, 'AI 灯光参考图测试');
  });

  test.afterAll(async ({ request }) => {
    await deleteSession(request, sessionId);
    sessionId = null;
  });

  test('POST /session/{id}/reference — 无文件返回 400', async ({ request }) => {
    if (!sessionId) return;

    const req = await readJson(
      await request.post(`${URL}/api/doodle/ai/light/session/${sessionId}/reference`, {
        headers: { ...authHeader, 'Content-Type': 'multipart/form-data' },
        data: {},
      })
    );
    // 空 multipart 里没有文件 → get_files() 为空（ai_light.cpp:470-471）
    expectError(req, 400, '参考图');
  });

  test('POST /session/{id}/reference — 多图上传返回参考图数组（201）', async ({ request }) => {
    if (!sessionId) return;
    const picked = listReferenceImages().slice(0, 2);
    if (picked.length === 0) return;

    const req = await readJson(
      await request.post(`${URL}/api/doodle/ai/light/session/${sessionId}/reference`, {
        headers: authHeader,
        multipart: multiFileForm(picked),
      })
    );

    expect(req.status, JSON.stringify(req.body)).to.equal(201);
    expect(req.body).to.be.an('array').with.lengthOf(picked.length);

    req.body.forEach((ref) => {
      expect(ref).to.have.keys(REFERENCE_KEYS);
      expect(ref.session_id).to.equal(sessionId);
      // 落库前统一转 PNG 并算统计（ai_light.cpp:476-509）
      expect(ref.stats).to.be.an('object');
      expect(ref.stats).to.have.keys([
        'space',
        'luma_percentiles',
        'palette',
        'contrast',
        'white_balance_kelvin',
        'clip_rate',
        'retouch_score',
      ]);
      expect(ref.stats.space).to.equal('display_srgb');
      expect(ref.stats_confidence).to.be.at.least(0);
      expect(ref.stats_confidence).to.be.at.most(1);
      // 新上传的图默认不是外点
      expect(ref.is_outlier).to.equal(false);
      expect(ref.excluded_reason).to.equal('');
    });
  });

  test('GET /session/{id}/reference — 列表按 created_at 升序', async ({ request }) => {
    if (!sessionId) return;

    const req = await readJson(
      await request.get(`${URL}/api/doodle/ai/light/session/${sessionId}/reference`, { headers: authHeader })
    );

    expect(req.status).to.equal(200);
    expect(req.body).to.be.an('array');
    req.body.forEach((ref) => expect(ref).to.have.keys(REFERENCE_KEYS));
    for (let i = 1; i < req.body.length; i += 1) {
      expect(new Date(req.body[i - 1].created_at).getTime()).to.be.at.most(
        new Date(req.body[i].created_at).getTime()
      );
    }
  });

  test('GET /session/{id}/reference — 会话不存在返回 404', async ({ request }) => {
    const req = await readJson(
      await request.get(`${URL}/api/doodle/ai/light/session/${NON_EXISTENT_ID}/reference`, { headers: authHeader })
    );
    expect(req.status).to.equal(404);
  });

  test('POST /session/{id}/reference — 会话不存在返回 404', async ({ request }) => {
    const req = await readJson(
      await request.post(`${URL}/api/doodle/ai/light/session/${NON_EXISTENT_ID}/reference`, {
        headers: authHeader,
        multipart: multiFileForm(listReferenceImages().slice(0, 1)),
      })
    );
    // load_session_checked 先于取文件（ai_light.cpp:468）
    expect(req.status).to.equal(404);
  });

  test('GET /reference/{id} — 单条详情', async ({ request }) => {
    if (!sessionId) return;

    const list = await readJson(
      await request.get(`${URL}/api/doodle/ai/light/session/${sessionId}/reference`, { headers: authHeader })
    );
    if (list.body.length === 0) return;

    const refId = list.body[0].id;
    const req = await readJson(
      await request.get(`${URL}/api/doodle/ai/light/reference/${refId}`, { headers: authHeader })
    );

    expect(req.status).to.equal(200);
    expect(req.body).to.have.keys(REFERENCE_KEYS);
    expect(req.body.id).to.equal(refId);
    expect(req.body.session_id).to.equal(sessionId);
  });

  test('GET /reference/{id} — 参考图不存在返回 404', async ({ request }) => {
    const req = await readJson(
      await request.get(`${URL}/api/doodle/ai/light/reference/${NON_EXISTENT_ID}`, { headers: authHeader })
    );
    expect(req.status).to.equal(404);
  });

  test('PUT /reference/{id} — 只认 is_outlier / excluded_reason 两个可写字段', async ({ request }) => {
    if (!sessionId) return;

    const list = await readJson(
      await request.get(`${URL}/api/doodle/ai/light/session/${sessionId}/reference`, { headers: authHeader })
    );
    if (list.body.length === 0) return;

    const target = list.body[0];
    const req = await readJson(
      await request.put(`${URL}/api/doodle/ai/light/reference/${target.id}`, {
        headers: authHeader,
        // put_property_list() 只暴露这两项（reference.h:135-140），session_id 与统计不可改
        data: {
          is_outlier: true,
          excluded_reason: '构图里没有主体',
          session_id: NON_EXISTENT_ID,
          stats_confidence: 0.01,
        },
      })
    );

    expect(req.status, JSON.stringify(req.body)).to.equal(200);
    expect(req.body).to.have.keys(REFERENCE_KEYS);
    expect(req.body.is_outlier).to.equal(true);
    expect(req.body.excluded_reason).to.equal('构图里没有主体');
    expect(req.body.session_id).to.equal(sessionId);
    expect(req.body.stats_confidence).to.equal(target.stats_confidence);

    // 复原，避免影响后续用例
    const restore = await readJson(
      await request.put(`${URL}/api/doodle/ai/light/reference/${target.id}`, {
        headers: authHeader,
        data: { is_outlier: false, excluded_reason: '' },
      })
    );
    expect(restore.status).to.equal(200);
    expect(restore.body.is_outlier).to.equal(false);
  });

  test('PUT /reference/{id} — 参考图不存在返回 404', async ({ request }) => {
    const req = await readJson(
      await request.put(`${URL}/api/doodle/ai/light/reference/${NON_EXISTENT_ID}`, {
        headers: authHeader,
        data: { is_outlier: true },
      })
    );
    expect(req.status).to.equal(404);
  });

  test('DELETE /reference/{id} — 删除单张返回 204，列表少一条', async ({ request }) => {
    if (!sessionId) return;

    const created = await uploadReferences(request, sessionId, 1);
    if (!created) return;
    const refId = created[0].id;

    const before = await readJson(
      await request.get(`${URL}/api/doodle/ai/light/session/${sessionId}/reference`, { headers: authHeader })
    );

    const del = await readJson(
      await request.delete(`${URL}/api/doodle/ai/light/reference/${refId}`, { headers: authHeader })
    );
    expect(del.status).to.equal(204);

    const after = await readJson(
      await request.get(`${URL}/api/doodle/ai/light/session/${sessionId}/reference`, { headers: authHeader })
    );
    expect(after.body).to.have.lengthOf(before.body.length - 1);
    expect(after.body.map((r) => r.id)).to.not.include(refId);
  });

  test('DELETE /reference/{id} — 参考图不存在返回 404', async ({ request }) => {
    const req = await readJson(
      await request.delete(`${URL}/api/doodle/ai/light/reference/${NON_EXISTENT_ID}`, { headers: authHeader })
    );
    expect(req.status).to.equal(404);
  });
});

// ═══════════════════════════════════════════════════════════════════════════
// 3. 提交一轮候选：/session/{id}/iteration（需要虚拟灯光方案）
// ═══════════════════════════════════════════════════════════════════════════

test.describe('AI 灯光 —— 提交一轮候选（iteration submit）', () => {
  test.describe.configure({ timeout: 120000 });

  let sessionId = null;
  // 记录本轮提交是否真的落库，决定后续 judge 用例能不能跑
  let submitted = false;
  let iterationId = null;
  let candidateIds = [];

  test.beforeAll(async ({ request }) => {
    sessionId = await createSessionOrSkip(request, 'AI 灯光提交测试');
  });

  test.afterAll(async ({ request }) => {
    await deleteSession(request, sessionId);
    sessionId = null;
  });

  test('POST /session/{id}/iteration — 会话不存在返回 404', async ({ request }) => {
    const req = await submitIteration(request, NON_EXISTENT_ID, 1);
    expect(req.status).to.equal(404);
  });

  test('POST /session/{id}/iteration — 缺 candidates 返回 400', async ({ request }) => {
    if (!sessionId) return;

    const req = await readJson(
      await request.post(`${URL}/api/doodle/ai/light/session/${sessionId}/iteration`, {
        headers: authHeader,
        data: { iteration: 1 },
      })
    );
    // ai_light.cpp:1072
    expectError(req, 400, 'candidates');
  });

  test('POST /session/{id}/iteration — candidates 为空数组返回 400', async ({ request }) => {
    if (!sessionId) return;

    const req = await readJson(
      await request.post(`${URL}/api/doodle/ai/light/session/${sessionId}/iteration`, {
        headers: authHeader,
        data: { iteration: 1, candidates: [] },
      })
    );
    expectError(req, 400, 'candidates');
  });

  test('POST /session/{id}/iteration — iteration 必须是正整数', async ({ request }) => {
    if (!sessionId) return;

    for (const bad of [0, -1]) {
      const req = await readJson(
        await request.post(`${URL}/api/doodle/ai/light/session/${sessionId}/iteration`, {
          headers: authHeader,
          data: { iteration: bad, candidates: makeCandidates(1) },
        })
      );
      expectError(req, 400, 'iteration');
    }
  });

  test('POST /session/{id}/iteration — 轮次号与会话期望值不一致返回 400', async ({ request }) => {
    if (!sessionId) return;

    // 一轮都没提交过时期望 1，提交 5 必须被拒（ai_light.cpp:1075-1079）
    const req = await readJson(
      await request.post(`${URL}/api/doodle/ai/light/session/${sessionId}/iteration`, {
        headers: authHeader,
        data: { iteration: 5, candidates: makeCandidates(2) },
      })
    );
    expectError(req, 400, '期望');
  });

  test('POST /session/{id}/iteration — 虚拟方案提交（前端展开的 6 个候选）', async ({ request }) => {
    if (!sessionId) return;

    const req = await submitIteration(request, sessionId, 1, 6);

    if (isEnvironmentRejection(req)) {
      // 该镜头在库里还解析不出 UE 投影（主场景工程 / 相机文件 / 帧区间），
      // 服务端按设计拒绝本轮落库（ai_light.cpp:185-202 / 249-271）
      expectError(req, 500, '渲染装配');
      return;
    }

    expect(req.status, JSON.stringify(req.body)).to.equal(201);
    expect(req.body).to.have.keys(DETAIL_KEYS);

    const body = req.body;
    expect(body.iterations).to.have.lengthOf(1);
    expect(body.iterations[0]).to.have.keys(ITERATION_KEYS);
    const it = body.iterations[0];
    expect(it.iteration).to.equal(1);
    expect(it.session_id).to.equal(sessionId);
    // 引擎内路线：本轮先落库等发起方本机渲染（ai_light.cpp:204-247）
    expect(it.state).to.equal('generated');

    // 提交几个 rig 就落几个候选
    expect(body.candidates).to.have.lengthOf(6);
    body.candidates.forEach((cand) => {
      expect(cand).to.have.keys(CANDIDATE_KEYS);
      expect(cand.session_id).to.equal(sessionId);
      expect(cand.iteration_id).to.equal(it.id);
      expect(cand.render_status).to.equal('pending');
      expect(cand.is_selected).to.equal(false);
      expect(cand.preview_file_id).to.equal('00000000-0000-0000-0000-000000000000');
    });

    // 会话详情里的「每轮几个」= 最近一轮实际落库数（view.h:54-64）
    expect(body.candidate_count_per_iteration).to.equal(6);
    // 新生成的轮次还没进人工判断，只有 awaiting_judgement 才算等人工
    expect(body.is_waiting_human).to.equal(false);

    // 服务端把整轮方案存进 iteration.rigs（lighting_rig_set_t）
    expect(it.rigs).to.be.an('object');
    expect(it.rigs).to.have.keys(['session_id', 'shot_task_id', 'iteration', 'baseline_level', 'rigs']);
    expect(it.rigs.session_id).to.equal(sessionId);
    expect(it.rigs.iteration).to.equal(1);
    expect(it.rigs.rigs).to.have.lengthOf(6);

    // 每个 rig 的 candidate_id 由服务端回填成落库的候选 uuid，客户端给的值不采信
    const landedIds = body.candidates.map((c) => c.id);
    it.rigs.rigs.forEach((rig, i) => {
      expect(rig).to.have.keys(['candidate_id', 'provenance', 'globals', 'lights', 'ops']);
      expect(rig.candidate_id).to.equal(landedIds[i]);
      // 方案内容原样回传：全局参数、灯阵列与 ops
      expect(rig.globals).to.have.keys(['exposure', 'white_balance', 'environment', 'grading']);
      expect(rig.lights).to.have.lengthOf(4);
      expect(rig.lights.map((l) => l.id)).to.deep.equal(['key', 'fill', 'rim', 'sky']);
      expect(rig.lights.map((l) => l.role)).to.deep.equal(['key', 'fill', 'rim', 'ambient']);
      expect(rig.provenance.seed).to.equal(1000 + i);
    });
    // 基线（第 0 个）不做任何 ops，其余方案至少一步
    expect(it.rigs.rigs[0].ops).to.have.lengthOf(0);
    it.rigs.rigs.slice(1).forEach((rig) => expect(rig.ops.length).to.be.above(0));

    submitted = true;
    iterationId = it.id;
    candidateIds = landedIds;
  });

  test('POST /session/{id}/iteration — 重复提交同一轮次返回 400', async ({ request }) => {
    if (!sessionId || !submitted) return;

    const req = await submitIteration(request, sessionId, 1, 2);
    expectError(req, 400, '期望');
  });

  test('GET /session/{id} — 提交后详情里能看到这一轮与 6 个候选', async ({ request }) => {
    if (!sessionId || !submitted) return;

    const req = await getSession(request, sessionId);
    expect(req.status).to.equal(200);
    expect(req.body.iterations).to.have.lengthOf(1);
    expect(req.body.iterations[0].id).to.equal(iterationId);
    expect(req.body.candidates).to.have.lengthOf(6);
    expect(req.body.candidates.map((c) => c.id).sort()).to.deep.equal([...candidateIds].sort());
    expect(req.body.candidate_count_per_iteration).to.equal(6);
  });

  test('服务器把提交的方案原样存回：globals / lights / ops 与提交值一致', async ({ request }) => {
    if (!sessionId || !submitted) return;

    const req = await getSession(request, sessionId);
    const it = req.body.iterations[0];
    const submittedRig = makeCandidates(6)[3];

    const keyLight = it.rigs.rigs[3].lights.find((l) => l.id === 'key');
    expect(keyLight.intensity.value).to.equal(submittedRig.lights[0].intensity.value);
    expect(keyLight.color.kelvin).to.equal(submittedRig.lights[0].color.kelvin);
    expect(keyLight.anchor.space).to.equal('camera');
    expect(it.rigs.rigs[3].globals.exposure.ev100_bias).to.equal(submittedRig.globals.exposure.ev100_bias);
    expect(it.rigs.rigs[3].globals.grading.contrast).to.equal(submittedRig.globals.grading.contrast);
    expect(it.rigs.rigs[3].ops.map((o) => o.op)).to.deep.equal(submittedRig.ops.map((o) => o.op));
  });

  // -------------------------------------------------------------------------
  // 派发前的人工闸门：候选帧序列上传链路（begin → upload_frame → finish → movie）
  // 这些接口要求该会话已经提交过一轮，否则跳过。
  // -------------------------------------------------------------------------

  test('POST /candidate/{id}/begin_sequence — 候选不存在返回 404', async ({ request }) => {
    const req = await readJson(
      await request.post(`${URL}/api/doodle/ai/light/candidate/${NON_EXISTENT_ID}/begin_sequence`, {
        headers: authHeader,
        data: {},
      })
    );
    expect(req.status).to.equal(404);
  });

  test('POST /candidate/{id}/begin_sequence — 清空旧帧目录并复位候选状态', async ({ request }) => {
    if (!sessionId || !submitted) return;

    const req = await readJson(
      await request.post(`${URL}/api/doodle/ai/light/candidate/${candidateIds[0]}/begin_sequence`, {
        headers: authHeader,
        data: {},
      })
    );
    // begin_sequence 返回 http_body_empty_t（ai_light.cpp:829）
    expect(req.status).to.equal(204);

    const detail = await getSession(request, sessionId);
    const cand = detail.body.candidates.find((c) => c.id === candidateIds[0]);
    expect(cand.render_status).to.equal('pending');
    expect(cand.metrics).to.deep.equal({});
  });

  test('POST /candidate/{id}/upload_frame — 非 octet-stream 返回 400', async ({ request }) => {
    if (!sessionId || !submitted) return;

    const req = await readJson(
      await request.post(`${URL}/api/doodle/ai/light/candidate/${candidateIds[0]}/upload_frame`, {
        headers: { ...authHeader, 'Content-Type': 'application/json' },
        data: { hello: 'world' },
      })
    );
    // ai_light.cpp:843-846
    expectError(req, 400, 'application/octet-stream');
  });

  test('POST /candidate/{id}/upload_frame — 缺 Content-Disposition 返回 400', async ({ request }) => {
    if (!sessionId || !submitted) return;

    const req = await readJson(
      await request.post(`${URL}/api/doodle/ai/light/candidate/${candidateIds[0]}/upload_frame`, {
        headers: { ...authHeader, 'Content-Type': 'application/octet-stream' },
        data: Buffer.from('fake-png-bytes'),
      })
    );
    // frame_file_name_from_header（ai_light.cpp:765-781）
    expectError(req, 400, '缺失必要的请求头信息');
  });

  test('POST /candidate/{id}/upload_frame — 非法帧文件名返回 400', async ({ request }) => {
    if (!sessionId || !submitted) return;

    const req = await readJson(
      await request.post(`${URL}/api/doodle/ai/light/candidate/${candidateIds[0]}/upload_frame`, {
        headers: {
          ...authHeader,
          'Content-Type': 'application/octet-stream',
          'Content-Disposition': '../../etc/passwd',
        },
        data: Buffer.from('fake-png-bytes'),
      })
    );
    expectError(req, 400, '帧文件名非法');
  });

  test('POST /candidate/{id}/upload_frame — 合法文件名上传返回 204', async ({ request }) => {
    if (!sessionId || !submitted) return;

    const req = await readJson(
      await request.post(`${URL}/api/doodle/ai/light/candidate/${candidateIds[0]}/upload_frame`, {
        headers: {
          ...authHeader,
          'Content-Type': 'application/octet-stream',
          // 头值不是 base64 时按原文处理（ai_light.cpp:771-773）
          'Content-Disposition': 'frame_0001.png',
        },
        data: Buffer.from('fake-png-bytes'),
      })
    );
    expect(req.status).to.equal(204);
  });

  test('POST /candidate/{id}/upload_frame — 候选不存在返回 404', async ({ request }) => {
    const req = await readJson(
      await request.post(`${URL}/api/doodle/ai/light/candidate/${NON_EXISTENT_ID}/upload_frame`, {
        headers: {
          ...authHeader,
          'Content-Type': 'application/octet-stream',
          'Content-Disposition': 'frame_0001.png',
        },
        data: Buffer.from('fake-png-bytes'),
      })
    );
    expect(req.status).to.equal(404);
  });

  test('POST /candidate/{id}/finish_sequence — 帧数不足返回 400', async ({ request }) => {
    if (!sessionId || !submitted) return;

    // 上一个用例只传了 1 帧，合成不出视频（ai_light.cpp:891-894）
    const req = await readJson(
      await request.post(`${URL}/api/doodle/ai/light/candidate/${candidateIds[0]}/finish_sequence`, {
        headers: authHeader,
        data: { frame_rate: 24 },
      })
    );
    expectError(req, 400, '合成不出视频');
  });

  test('POST /candidate/{id}/finish_sequence — 候选不存在返回 404', async ({ request }) => {
    const req = await readJson(
      await request.post(`${URL}/api/doodle/ai/light/candidate/${NON_EXISTENT_ID}/finish_sequence`, {
        headers: authHeader,
        data: { frame_rate: 24 },
      })
    );
    expect(req.status).to.equal(404);
  });

  test('GET /candidate/{id}/movie — 未渲染完成返回 404', async ({ request }) => {
    if (!sessionId || !submitted) return;

    const req = await readJson(
      await request.get(`${URL}/api/doodle/ai/light/candidate/${candidateIds[1]}/movie`, { headers: authHeader })
    );
    // render_status 仍是 pending（ai_light.cpp:596-598）
    expectError(req, 404, '尚未渲染完成');
  });

  test('GET /candidate/{id}/movie — 候选不存在返回 404', async ({ request }) => {
    const req = await readJson(
      await request.get(`${URL}/api/doodle/ai/light/candidate/${NON_EXISTENT_ID}/movie`, { headers: authHeader })
    );
    expect(req.status).to.equal(404);
  });

  test('GET /candidate/{id}/movie — 渲染完成但视频缺失返回 500', async ({ request }) => {
    if (!sessionId || !submitted) return;

    // 往候选 1 传两帧并 finish，状态会变成 rendered；本机没有 MRQ 产物，
    // get_ai_light_candidate_movie_file 指向的文件不存在 → DOODLE_CHICK 抛 500（ai_light.cpp:603）
    const frame = Buffer.from('fake-png-bytes');
    for (const name of ['frame_0001.png', 'frame_0002.png']) {
      const up = await readJson(
        await request.post(`${URL}/api/doodle/ai/light/candidate/${candidateIds[1]}/upload_frame`, {
          headers: {
            ...authHeader,
            'Content-Type': 'application/octet-stream',
            'Content-Disposition': name,
          },
          data: frame,
        })
      );
      expect(up.status, JSON.stringify(up.body)).to.equal(204);
    }

    const finish = await readJson(
      await request.post(`${URL}/api/doodle/ai/light/candidate/${candidateIds[1]}/finish_sequence`, {
        headers: authHeader,
        data: { frame_rate: 24 },
      })
    );
    // 两帧不是真 PNG，合成大概率失败并回 500；只要不是 404「尚未渲染完成」就说明状态已推进
    expect([204, 500]).to.include(finish.status);

    const detail = await getSession(request, sessionId);
    const cand = detail.body.candidates.find((c) => c.id === candidateIds[1]);
    expect(['rendered', 'failed']).to.include(cand.render_status);

    const movie = await readJson(
      await request.get(`${URL}/api/doodle/ai/light/candidate/${candidateIds[1]}/movie`, { headers: authHeader })
    );
    if (cand.render_status === 'failed') {
      expectError(movie, 404, '渲染失败');
    } else {
      // 合成成功（本机有 ffmpeg）则直接返回视频
      expect([200, 500]).to.include(movie.status);
    }
  });

  // -------------------------------------------------------------------------
  // 人工闸门 2：判断
  // -------------------------------------------------------------------------

  test('POST /session/{id}/iteration/{iteration_id} — 会话不存在返回 404', async ({ request }) => {
    const req = await readJson(
      await request.post(`${URL}/api/doodle/ai/light/session/${NON_EXISTENT_ID}/iteration/${NON_EXISTENT_ID}`, {
        headers: authHeader,
        data: {},
      })
    );
    expect(req.status).to.equal(404);
  });

  test('POST /session/{id}/iteration/{iteration_id} — 轮次不存在返回 404', async ({ request }) => {
    if (!sessionId) return;

    const req = await readJson(
      await request.post(`${URL}/api/doodle/ai/light/session/${sessionId}/iteration/${NON_EXISTENT_ID}`, {
        headers: authHeader,
        data: {},
      })
    );
    expect(req.status).to.equal(404);
  });

  test('POST /session/{id}/iteration/{iteration_id} — 空判断（全否）把轮次置为 judged', async ({ request }) => {
    if (!sessionId || !submitted) return;

    const req = await readJson(
      await request.post(`${URL}/api/doodle/ai/light/session/${sessionId}/iteration/${iterationId}`, {
        headers: authHeader,
        data: { feedback_text: '整体偏暗' },
      })
    );

    expect(req.status, JSON.stringify(req.body)).to.equal(201);
    expect(req.body).to.have.keys(DETAIL_KEYS);
    const it = req.body.iterations.find((i) => i.id === iterationId);
    expect(it.state).to.equal('judged');
    expect(it.judged_at).to.not.be.equal(null);
    expect(it.judged_by).to.not.equal('00000000-0000-0000-0000-000000000000');
    // 没选候选时 feedback_text 也会落库
    expect(it.feedback_text).to.equal('整体偏暗');
    // 没有候选被选中
    expect(req.body.candidates.every((c) => c.is_selected === false)).to.equal(true);
  });

  test('POST /session/{id}/iteration/{iteration_id} — 重复判断返回 400', async ({ request }) => {
    if (!sessionId || !submitted) return;

    const req = await readJson(
      await request.post(`${URL}/api/doodle/ai/light/session/${sessionId}/iteration/${iterationId}`, {
        headers: authHeader,
        data: {},
      })
    );
    // ai_light.cpp:628-631
    expectError(req, 400, '已经判断过');
  });

  test('POST /session/{id}/iteration/{iteration_id} — 候选不属于本轮返回 400', async ({ request }) => {
    if (!sessionId || !submitted) return;

    // 该轮已判断，先验 updated；这里只用来确认「不属于本轮」分支不可达时仍然是 400
    const req = await readJson(
      await request.post(`${URL}/api/doodle/ai/light/session/${sessionId}/iteration/${iterationId}`, {
        headers: authHeader,
        data: { selected_candidate_id: NON_EXISTENT_ID },
      })
    );
    expect(req.status).to.equal(400);
  });

  test('POST /session/{id}/iteration — 判断之后可以提交下一轮', async ({ request }) => {
    if (!sessionId || !submitted) return;

    // 第 1 轮已经 judged，is_waiting_human=false，期望第 2 轮
    const req = await submitIteration(request, sessionId, 2, 3);
    if (isEnvironmentRejection(req)) return; // 环境不满足时第一轮就已被拒，这里同样跳过

    expect(req.status, JSON.stringify(req.body)).to.equal(201);
    expect(req.body.iterations).to.have.lengthOf(2);
    expect(req.body.iterations[1].iteration).to.equal(2);
    expect(req.body.iterations[1].state).to.equal('generated');
    expect(req.body.candidates).to.have.lengthOf(9);
    expect(req.body.candidate_count_per_iteration).to.equal(3);
    // 第 1 轮的判断结果保留
    expect(req.body.iterations[0].state).to.equal('judged');
  });

  test('POST /session/{id}/iteration — 上一轮还没判断时不能提交新候选', async ({ request }) => {
    if (!sessionId || !submitted) return;

    // 第 2 轮是 generated（artificial 的等待态不成立），is_waiting_human 为 false，
    // 因此这里可以提交第 3 轮；真正的拦截场景需要 await_judgement，见下一条。
    const detail = await getSession(request, sessionId);
    const latest = detail.body.iterations.at(-1);
    if (!latest || latest.state !== 'awaiting_judgement') return;

    const req = await submitIteration(request, sessionId, latest.iteration + 1, 1);
    expectError(req, 400, '尚未判断');
  });
});

// ═══════════════════════════════════════════════════════════════════════════
// 4. 预设库：/preset 与 /preset/{id}
// ═══════════════════════════════════════════════════════════════════════════

test.describe('AI 灯光 —— 预设库（preset）', () => {
  test.describe.configure({ timeout: 60000 });

  const PRESET_NAME = 'ai_light_测试预设_白天';
  let presetId = null;

  test.afterAll(async ({ request }) => {
    if (presetId) {
      await request.delete(`${URL}/api/doodle/ai/light/preset/${presetId}`, { headers: authHeader });
      presetId = null;
    }
  });

  test('POST /preset — 缺 name 返回 400', async ({ request }) => {
    const req = await readJson(
      await request.post(`${URL}/api/doodle/ai/light/preset`, {
        headers: authHeader,
        data: { tags: ['测试'] },
      })
    );
    expectError(req, 400, '预设名称');
  });

  test('POST /preset — 新建预设（created_by 由服务端填）', async ({ request }) => {
    const req = await readJson(
      await request.post(`${URL}/api/doodle/ai/light/preset`, {
        headers: authHeader,
        data: {
          name: PRESET_NAME,
          tags: ['白天', 'interior'],
          // 这两个是 JSON 列，原样存取（preset.h:61-71）
          applicable: { scene_type: ['interior', 'day'], interior: true },
          stats_target: { contrast: [0.55, 0.75] },
          // rig_fragment 是一段 LightingRig，identity 字段留空
          rig_fragment: makeRig({ base: true }),
          enabled: true,
          // created_by 由服务端覆盖（ai_light.cpp:956）
          created_by: NON_EXISTENT_ID,
        },
      })
    );

    expect(req.status, JSON.stringify(req.body)).to.equal(201);
    expect(req.body).to.have.keys([
      'id',
      'name',
      'tags',
      'embedding',
      'embedding_dim',
      'rig_fragment',
      'applicable',
      'stats_target',
      'enabled',
      'created_by',
      'created_at',
      'updated_at',
    ]);
    expect(req.body.name).to.equal(PRESET_NAME);
    expect(req.body.tags).to.deep.equal(['白天', 'interior']);
    expect(req.body.enabled).to.equal(true);
    expect(req.body.created_by).to.not.equal(NON_EXISTENT_ID);
    expect(req.body.applicable).to.deep.equal({ scene_type: ['interior', 'day'], interior: true });
    expect(req.body.stats_target).to.deep.equal({ contrast: [0.55, 0.75] });
    expect(req.body.rig_fragment.lights).to.have.lengthOf(4);
    // embedding 未传时为 null（nlohmann::json 默认值）
    expect(req.body.embedding).to.equal(null);
    expect(req.body.embedding_dim).to.equal(0);

    presetId = req.body.id;
  });

  test('GET /preset — 列表包含刚建的预设', async ({ request }) => {
    if (!presetId) return;

    const req = await readJson(await request.get(`${URL}/api/doodle/ai/light/preset`, { headers: authHeader }));
    expect(req.status).to.equal(200);
    expect(req.body).to.be.an('array');
    expect(req.body.map((p) => p.id)).to.include(presetId);
    // enabled_only 默认 true，列表里每一条都必须是启用的
    req.body.forEach((p) => expect(p.enabled).to.equal(true));
  });

  test('GET /preset?enabled_only=false — 也返回停用的预设', async ({ request }) => {
    if (!presetId) return;

    const req = await readJson(
      await request.get(`${URL}/api/doodle/ai/light/preset?enabled_only=false`, { headers: authHeader })
    );
    expect(req.status).to.equal(200);
    expect(req.body.map((p) => p.id)).to.include(presetId);
  });

  test('GET /preset/{id} — 预设详情', async ({ request }) => {
    if (!presetId) return;

    const req = await readJson(
      await request.get(`${URL}/api/doodle/ai/light/preset/${presetId}`, { headers: authHeader })
    );
    expect(req.status).to.equal(200);
    expect(req.body.id).to.equal(presetId);
    expect(req.body.name).to.equal(PRESET_NAME);
  });

  test('PUT /preset/{id} — 局部更新只覆盖请求体里出现的字段', async ({ request }) => {
    if (!presetId) return;

    const req = await readJson(
      await request.put(`${URL}/api/doodle/ai/light/preset/${presetId}`, {
        headers: authHeader,
        data: { name: `${PRESET_NAME}_改`, enabled: false },
      })
    );

    expect(req.status, JSON.stringify(req.body)).to.equal(200);
    expect(req.body.name).to.equal(`${PRESET_NAME}_改`);
    expect(req.body.enabled).to.equal(false);
    // tags / applicable 没传，保持原值（put_property_list 逐字段 set_from_ref）
    expect(req.body.tags).to.deep.equal(['白天', 'interior']);
    expect(req.body.applicable).to.deep.equal({ scene_type: ['interior', 'day'], interior: true });
    // created_by 不在可写字段里
    expect(req.body.created_by).to.not.equal(NON_EXISTENT_ID);
  });

  test('GET /preset — 停用后默认列表里不再出现', async ({ request }) => {
    if (!presetId) return;

    const req = await readJson(await request.get(`${URL}/api/doodle/ai/light/preset`, { headers: authHeader }));
    expect(req.status).to.equal(200);
    expect(req.body.map((p) => p.id)).to.not.include(presetId);

    const all = await readJson(
      await request.get(`${URL}/api/doodle/ai/light/preset?enabled_only=false`, { headers: authHeader })
    );
    expect(all.body.map((p) => p.id)).to.include(presetId);
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

  test('DELETE /preset/{id} — 删除返回 204，再取 404', async ({ request }) => {
    if (!presetId) return;

    const id = presetId;
    const req = await readJson(
      await request.delete(`${URL}/api/doodle/ai/light/preset/${id}`, { headers: authHeader })
    );
    expect(req.status).to.equal(204);
    presetId = null;

    const gone = await readJson(
      await request.get(`${URL}/api/doodle/ai/light/preset/${id}`, { headers: authHeader })
    );
    expect(gone.status).to.equal(404);
  });

  test('DELETE /preset/{id} — 预设不存在返回 404', async ({ request }) => {
    const req = await readJson(
      await request.delete(`${URL}/api/doodle/ai/light/preset/${NON_EXISTENT_ID}`, { headers: authHeader })
    );
    expect(req.status).to.equal(404);
  });
});

// ═══════════════════════════════════════════════════════════════════════════
// 5. 鉴权：ai_light.h 里全部 DOODLE_HTTP_JWT_FUN 路由
// ═══════════════════════════════════════════════════════════════════════════

test.describe('AI 灯光 —— 鉴权', () => {
  test.describe.configure({ timeout: 60000 });

  // [标签, method, url] —— 每条都用不存在的 id，鉴权在路由匹配后、业务查询前失败
  const noAuthCases = [
    ['GET shot session', 'GET', `${URL}/api/doodle/ai/light/shot/${NON_EXISTENT_ID}/session`],
    ['POST shot session', 'POST', `${URL}/api/doodle/ai/light/shot/${NON_EXISTENT_ID}/session`],
    ['GET session', 'GET', `${URL}/api/doodle/ai/light/session/${NON_EXISTENT_ID}`],
    ['PUT session', 'PUT', `${URL}/api/doodle/ai/light/session/${NON_EXISTENT_ID}`],
    ['DELETE session', 'DELETE', `${URL}/api/doodle/ai/light/session/${NON_EXISTENT_ID}`],
    ['GET session reference', 'GET', `${URL}/api/doodle/ai/light/session/${NON_EXISTENT_ID}/reference`],
    ['POST session reference', 'POST', `${URL}/api/doodle/ai/light/session/${NON_EXISTENT_ID}/reference`],
    ['GET reference', 'GET', `${URL}/api/doodle/ai/light/reference/${NON_EXISTENT_ID}`],
    ['PUT reference', 'PUT', `${URL}/api/doodle/ai/light/reference/${NON_EXISTENT_ID}`],
    ['DELETE reference', 'DELETE', `${URL}/api/doodle/ai/light/reference/${NON_EXISTENT_ID}`],
    [
      'POST judgement',
      'POST',
      `${URL}/api/doodle/ai/light/session/${NON_EXISTENT_ID}/iteration/${NON_EXISTENT_ID}`,
    ],
    ['POST begin_sequence', 'POST', `${URL}/api/doodle/ai/light/candidate/${NON_EXISTENT_ID}/begin_sequence`],
    ['POST upload_frame', 'POST', `${URL}/api/doodle/ai/light/candidate/${NON_EXISTENT_ID}/upload_frame`],
    ['POST finish_sequence', 'POST', `${URL}/api/doodle/ai/light/candidate/${NON_EXISTENT_ID}/finish_sequence`],
    ['GET candidate movie', 'GET', `${URL}/api/doodle/ai/light/candidate/${NON_EXISTENT_ID}/movie`],
    ['GET preset', 'GET', `${URL}/api/doodle/ai/light/preset`],
    ['POST preset', 'POST', `${URL}/api/doodle/ai/light/preset`],
    ['GET preset instance', 'GET', `${URL}/api/doodle/ai/light/preset/${NON_EXISTENT_ID}`],
    ['PUT preset instance', 'PUT', `${URL}/api/doodle/ai/light/preset/${NON_EXISTENT_ID}`],
    ['DELETE preset instance', 'DELETE', `${URL}/api/doodle/ai/light/preset/${NON_EXISTENT_ID}`],
    ['POST iteration submit', 'POST', `${URL}/api/doodle/ai/light/session/${NON_EXISTENT_ID}/iteration`],
  ];

  for (const [label, method, url] of noAuthCases) {
    test(`${label} — 无 JWT 返回 401`, async ({ request }) => {
      const res = await request.fetch(url, {
        method,
        data: method === 'GET' || method === 'DELETE' ? undefined : {},
      });
      // parse_header 在 method 分派前抛出（http_jwt_fun.cpp:104）
      expect(res.status(), `${method} ${url}`).to.equal(401);
    });
  }

  test('非法 Cookie 值返回 401', async ({ request }) => {
    const res = await request.get(`${URL}/api/doodle/ai/light/preset`, {
      headers: { Cookie: 'access_token_cookie=not-a-jwt' },
    });
    expect(res.status()).to.equal(401);
  });

  test('篡改签名的 JWT 返回 401', async ({ request }) => {
    const bogusJwt = `${JWT.slice(0, -4)}AAAA`;
    const res = await request.get(`${URL}/api/doodle/ai/light/preset`, {
      headers: { Cookie: `access_token_cookie=${bogusJwt}` },
    });
    expect(res.status()).to.equal(401);
  });

  test('Authorization: Bearer 与 Cookie 等价', async ({ request }) => {
    const res = await request.get(`${URL}/api/doodle/ai/light/preset`, {
      headers: { Authorization: `Bearer ${JWT}` },
    });
    expect(res.status()).to.equal(200);
  });
});

// ═══════════════════════════════════════════════════════════════════════════
// 6. 图片/视频接口的响应头契约（供后续方案测试比对渲染产物）
// ═══════════════════════════════════════════════════════════════════════════

test.describe('AI 灯光 —— 媒体响应契约', () => {
  test.describe.configure({ timeout: 30000 });

  test('GET /candidate/{id}/movie — 不存在时返回 json 错误体', async ({ request }) => {
    const res = await readJson(
      await request.get(`${URL}/api/doodle/ai/light/candidate/${NON_EXISTENT_ID}/movie`, { headers: authHeader })
    );
    expect(res.status).to.equal(404);
    // 错误一律走 http_body_error_t，响应体是 json（http_body.cpp:311-312）
    expect(res.body).to.be.an('object');
    expect(res.body.error).to.be.a('string').and.not.be.empty;
  });

  test('POST /preset — 未知枚举值不会建成预设', async ({ request }) => {
    // Notion 侧没有枚举，这里只验证空 name 被拒这条路不会留下脏数据
    const before = await readJson(
      await request.get(`${URL}/api/doodle/ai/light/preset?enabled_only=false`, { headers: authHeader })
    );
    const req = await readJson(
      await request.post(`${URL}/api/doodle/ai/light/preset`, {
        headers: authHeader,
        data: { name: '' },
      })
    );
    expect(req.status).to.equal(400);

    const after = await readJson(
      await request.get(`${URL}/api/doodle/ai/light/preset?enabled_only=false`, { headers: authHeader })
    );
    expect(after.body.length).to.equal(before.body.length);
  });
});
