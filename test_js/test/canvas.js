import { test } from '@playwright/test';
import { expect } from 'chai';

import { URL, JWT, PROJECT_ID, PERSON_ID, NIL_ID, NON_EXISTENT_ID, authHeader } from './config.js';
import { readJson } from './helpers.js';

const SUBPROJECT_URL = `${URL}/api/seedance2/subproject`;

function uuidv4() {
  return crypto.randomUUID();
}

async function postSubproject(request, name) {
  const res = await request.post(SUBPROJECT_URL, { headers: authHeader, data: { name, project_id: PROJECT_ID } });
  const req = await readJson(res);
  expect(req.status, `创建子项目失败：${JSON.stringify(req.body)}`).to.equal(201);
  return req.body.id;
}

function postCanvas(request, subprojectId, data) {
  return request
    .post(`${SUBPROJECT_URL}/${subprojectId}/canvas`, { headers: authHeader, data })
    .then(readJson);
}

function postNode(request, canvasId, data) {
  return request
    .post(`${URL}/api/seedance2/canvas/${canvasId}/node`, { headers: authHeader, data })
    .then(readJson);
}

function putNode(request, canvasId, nodeId, data) {
  return request
    .put(`${URL}/api/seedance2/canvas/${canvasId}/node/${nodeId}`, { headers: authHeader, data })
    .then(readJson);
}

function postConnection(request, canvasId, data) {
  return request
    .post(`${URL}/api/seedance2/canvas/${canvasId}/connection`, { headers: authHeader, data })
    .then(readJson);
}

function postPerson(request, canvasId, data) {
  return request
    .post(`${URL}/api/seedance2/canvas/${canvasId}/person`, { headers: authHeader, data })
    .then(readJson);
}

test.describe('seedance2 画布 — 生命周期', () => {
  let subprojectId = null;
  let canvasId = null;
  let textNodeId = null;
  let imageNodeId = null;
  let groupNodeId = null;
  let connectionId = null;

  test.beforeAll(async ({ request }) => {
    subprojectId = await postSubproject(request, 'test_canvas_lifecycle');
  });

  test.afterAll(async ({ request }) => {
    if (subprojectId) {
      await request.delete(`${SUBPROJECT_URL}/${subprojectId}`, { headers: authHeader });
    }
  });

  test('POST /api/seedance2/subproject/{subproject_id}/canvas — 创建画布', async ({ request }) => {
    expect(subprojectId).to.not.be.null;
    const req = await postCanvas(request, subprojectId, {
      title: 'test_canvas',
      description: '测试画布',
      viewport: { x: 10.5, y: -20.25, k: 0.75 },
      background_mode: 'lines',
    });

    expect(req.status, JSON.stringify(req.body)).to.equal(201);
    expect(req.body.id).to.be.a('string');
    expect(req.body.title).to.equal('test_canvas');
    expect(req.body.description).to.equal('测试画布');
    expect(req.body.subproject_id).to.equal(subprojectId);
    expect(req.body.project_uuid_id).to.equal(PROJECT_ID);
    expect(req.body.user_id).to.equal(PERSON_ID);
    expect(req.body.viewport).to.deep.equal({ x: 10.5, y: -20.25, k: 0.75 });
    expect(req.body.background_mode).to.equal('lines');

    canvasId = req.body.id;
    console.log('POST canvas 返回值:', JSON.stringify(req.body, null, 2));
  });

  test('POST .../canvas — 缺少 title 应返回 400', async ({ request }) => {
    const req = await postCanvas(request, subprojectId, { description: '没有标题' });
    expect(req.status).to.equal(400);
    console.log('缺少 title 的响应:', JSON.stringify(req.body));
  });

  test('POST .../canvas — 非法 background_mode 应返回 400', async ({ request }) => {
    const req = await postCanvas(request, subprojectId, { title: 'bad_bg', background_mode: 'checkerboard' });
    expect(req.status).to.equal(400);
    console.log('非法 background_mode 的响应:', JSON.stringify(req.body));
  });

  test('GET /api/seedance2/subproject/{subproject_id}/canvas — 获取画布列表', async ({ request }) => {
    expect(canvasId).to.not.be.null;
    const res = await request.get(`${SUBPROJECT_URL}/${subprojectId}/canvas`, { headers: authHeader });
    const req = await readJson(res);

    expect(req.status).to.equal(200);
    expect(req.body).to.be.an('array');
    expect(req.body.map((c) => c.id)).to.include(canvasId);
    console.log('GET canvas list 返回值:', JSON.stringify(req.body, null, 2));
  });

  test('GET /api/seedance2/canvas/{canvas_id} — 空画布详情应返回空节点与连线', async ({ request }) => {
    expect(canvasId).to.not.be.null;
    const res = await request.get(`${URL}/api/seedance2/canvas/${canvasId}`, { headers: authHeader });
    const req = await readJson(res);

    expect(req.status).to.equal(200);
    expect(req.body).to.have.all.keys('canvas', 'nodes', 'connections', 'person_links');
    expect(req.body.canvas.id).to.equal(canvasId);
    expect(req.body.nodes).to.deep.equal([]);
    expect(req.body.connections).to.deep.equal([]);
    expect(req.body.person_links).to.deep.equal([]);
    console.log('GET 空画布详情:', JSON.stringify(req.body, null, 2));
  });

  test('PUT /api/seedance2/canvas/{canvas_id} — 更新画布', async ({ request }) => {
    expect(canvasId).to.not.be.null;
    const res = await request.put(`${URL}/api/seedance2/canvas/${canvasId}`, {
      headers: authHeader,
      data: {
        title: 'test_canvas_updated',
        description: '更新后的画布',
        viewport: { x: 1, y: 2, k: 3 },
        background_mode: 'blank',
      },
    });
    const req = await readJson(res);

    expect(req.status).to.equal(200);
    expect(req.body.title).to.equal('test_canvas_updated');
    expect(req.body.description).to.equal('更新后的画布');
    expect(req.body.viewport).to.deep.equal({ x: 1, y: 2, k: 3 });
    expect(req.body.background_mode).to.equal('blank');
    console.log('PUT canvas 返回值:', JSON.stringify(req.body, null, 2));
  });

  test('POST /api/seedance2/canvas/{canvas_id}/node — 创建 text 节点', async ({ request }) => {
    expect(canvasId).to.not.be.null;
    const req = await postNode(request, canvasId, {
      type: 'text',
      title: '文字节点',
      position: { x: 100, y: 200 },
      width: 500,
      height: 320,
      metadata: { content: '你好，画布', fontSize: 18 },
    });

    expect(req.status, JSON.stringify(req.body)).to.equal(201);
    expect(req.body.id).to.be.a('string');
    expect(req.body.canvas_id).to.equal(canvasId);
    expect(req.body.type).to.equal('text');
    expect(req.body.title).to.equal('文字节点');
    expect(req.body.position).to.deep.equal({ x: 100, y: 200 });
    expect(req.body.width).to.equal(500);
    expect(req.body.height).to.equal(320);
    expect(req.body.metadata.content).to.equal('你好，画布');
    expect(req.body.metadata.fontSize).to.equal(18);

    textNodeId = req.body.id;
    console.log('POST text node 返回值:', JSON.stringify(req.body, null, 2));
  });

  test('POST .../node — 创建 image 节点应使用默认尺寸', async ({ request }) => {
    expect(canvasId).to.not.be.null;
    const req = await postNode(request, canvasId, { type: 'image', title: '图片节点' });

    expect(req.status).to.equal(201);
    expect(req.body.type).to.equal('image');
    expect(req.body.width).to.equal(340);
    expect(req.body.height).to.equal(240);
    expect(req.body.position).to.deep.equal({ x: 0, y: 0 });

    imageNodeId = req.body.id;
    console.log('POST image node 返回值:', JSON.stringify(req.body, null, 2));
  });

  test('POST .../node — 创建 group 节点', async ({ request }) => {
    expect(canvasId).to.not.be.null;
    const req = await postNode(request, canvasId, {
      type: 'group',
      title: '分组节点',
      position: { x: 0, y: 400 },
      width: 760,
      height: 480,
    });

    expect(req.status).to.equal(201);
    expect(req.body.type).to.equal('group');
    groupNodeId = req.body.id;
    console.log('POST group node 返回值:', JSON.stringify(req.body, null, 2));
  });

  test('POST .../node — 缺少 type 应返回 400', async ({ request }) => {
    const req = await postNode(request, canvasId, { title: '没有类型' });
    expect(req.status).to.equal(400);
    console.log('缺少 type 的响应:', JSON.stringify(req.body));
  });

  test('POST .../node — 非法 type 应返回 400', async ({ request }) => {
    const req = await postNode(request, canvasId, { type: 'not_a_type', title: '非法类型' });
    expect(req.status).to.equal(400);
    console.log('非法 type 的响应:', JSON.stringify(req.body));
  });

  test('POST .../node — group_id 指向非 group 节点应返回 400', async ({ request }) => {
    expect(textNodeId).to.not.be.null;
    const req = await postNode(request, canvasId, { type: 'text', title: '错误分组', group_id: textNodeId });
    expect(req.status).to.equal(400);
    expect(JSON.stringify(req.body)).to.include('group_id');
    console.log('group_id 指向非 group 的响应:', JSON.stringify(req.body));
  });

  test('POST .../node — group_id 为空 id 表示未分组，应创建成功', async ({ request }) => {
    const req = await postNode(request, canvasId, { type: 'text', title: '无分组节点', group_id: NIL_ID });

    expect(req.status, JSON.stringify(req.body)).to.equal(201);
    expect(req.body.group_id ?? NIL_ID).to.equal(NIL_ID);
    console.log('group_id 为空 id 的响应:', JSON.stringify(req.body));
  });

  test('POST .../node — group_id 指向不存在的节点应返回 404', async ({ request }) => {
    const req = await postNode(request, canvasId, { type: 'text', title: '分组不存在', group_id: NON_EXISTENT_ID });
    expect(req.status, JSON.stringify(req.body)).to.equal(404);
    console.log('group_id 不存在的响应:', JSON.stringify(req.body));
  });

  test('POST .../node — 合法 group_id 应写入 group_id', async ({ request }) => {
    expect(groupNodeId).to.not.be.null;
    const req = await postNode(request, canvasId, {
      type: 'video',
      title: '分组内的视频',
      group_id: groupNodeId,
    });

    expect(req.status).to.equal(201);
    expect(req.body.group_id).to.equal(groupNodeId);
    console.log('POST 分组节点返回值:', JSON.stringify(req.body, null, 2));

    await request.delete(`${URL}/api/seedance2/canvas/${canvasId}/node/${req.body.id}`, { headers: authHeader });
  });

  test('GET /api/seedance2/canvas/{canvas_id}/node — 获取节点列表', async ({ request }) => {
    expect(canvasId).to.not.be.null;
    const res = await request.get(`${URL}/api/seedance2/canvas/${canvasId}/node`, { headers: authHeader });
    const req = await readJson(res);

    expect(req.status).to.equal(200);
    expect(req.body).to.be.an('array');
    const ids = req.body.map((n) => n.id);
    expect(ids).to.include(textNodeId);
    expect(ids).to.include(imageNodeId);
    expect(ids).to.include(groupNodeId);
    console.log('GET node list 返回值:', JSON.stringify(req.body, null, 2));
  });

  test('GET /api/seedance2/canvas/{canvas_id}/node/{node_id} — 获取节点详情', async ({ request }) => {
    expect(textNodeId).to.not.be.null;
    const res = await request.get(`${URL}/api/seedance2/canvas/${canvasId}/node/${textNodeId}`, {
      headers: authHeader,
    });
    const req = await readJson(res);

    expect(req.status).to.equal(200);
    expect(req.body.id).to.equal(textNodeId);
    expect(req.body.type).to.equal('text');
    console.log('GET node instance 返回值:', JSON.stringify(req.body, null, 2));
  });

  test('GET .../node/{node_id} — 节点不属于该画布应返回 404', async ({ request }) => {
    expect(canvasId).to.not.be.null;
    const res = await request.get(`${URL}/api/seedance2/canvas/${NON_EXISTENT_ID}/node/${textNodeId}`, {
      headers: authHeader,
    });
    const req = await readJson(res);
    expect(req.status).to.equal(404);
    console.log('跨画布取节点的响应:', JSON.stringify(req.body));
  });

  test('PUT /api/seedance2/canvas/{canvas_id}/node/{node_id} — 更新节点', async ({ request }) => {
    expect(textNodeId).to.not.be.null;
    const req = await putNode(request, canvasId, textNodeId, {
      title: '文字节点_更新',
      position: { x: 123, y: 456 },
      width: 600,
      metadata: { content: '更新后的内容', fontSize: 24 },
    });

    expect(req.status).to.equal(200);
    expect(req.body.title).to.equal('文字节点_更新');
    expect(req.body.position).to.deep.equal({ x: 123, y: 456 });
    expect(req.body.width).to.equal(600);
    expect(req.body.metadata.content).to.equal('更新后的内容');
    expect(req.body.metadata.fontSize).to.equal(24);
    console.log('PUT node 返回值:', JSON.stringify(req.body, null, 2));
  });

  test('PUT .../node/{node_id} — 把节点移入 group', async ({ request }) => {
    expect(imageNodeId).to.not.be.null;
    expect(groupNodeId).to.not.be.null;
    const req = await putNode(request, canvasId, imageNodeId, { group_id: groupNodeId });

    expect(req.status).to.equal(200);
    expect(req.body.group_id).to.equal(groupNodeId);
    console.log('PUT node group_id 返回值:', JSON.stringify(req.body, null, 2));
  });

  test('PUT .../node/{node_id} — group_id 指向自身应返回 400', async ({ request }) => {
    expect(groupNodeId).to.not.be.null;
    const req = await putNode(request, canvasId, groupNodeId, { group_id: groupNodeId });
    expect(req.status).to.equal(400);
    console.log('group_id 自指的响应:', JSON.stringify(req.body));
  });

  test('PUT .../node/{node_id} — group_id 为空 id 表示移出分组', async ({ request }) => {
    expect(imageNodeId).to.not.be.null;
    const req = await putNode(request, canvasId, imageNodeId, { group_id: NIL_ID });

    expect(req.status, JSON.stringify(req.body)).to.equal(200);
    expect(req.body.group_id ?? NIL_ID).to.equal(NIL_ID);
    console.log('PUT node 移出分组返回值:', JSON.stringify(req.body, null, 2));
  });

  test('PUT .../node/{node_id} — group_id 指向不存在的节点应返回 404', async ({ request }) => {
    expect(imageNodeId).to.not.be.null;
    const req = await putNode(request, canvasId, imageNodeId, { group_id: NON_EXISTENT_ID });
    expect(req.status, JSON.stringify(req.body)).to.equal(404);
    console.log('PUT group_id 不存在的响应:', JSON.stringify(req.body));
  });

  test('POST /api/seedance2/canvas/{canvas_id}/connection — 创建连线', async ({ request }) => {
    expect(textNodeId).to.not.be.null;
    expect(imageNodeId).to.not.be.null;
    const req = await postConnection(request, canvasId, {
      from_node_id: textNodeId,
      to_node_id: imageNodeId,
    });

    expect(req.status, JSON.stringify(req.body)).to.equal(201);
    expect(req.body.id).to.be.a('string');
    expect(req.body.canvas_id).to.equal(canvasId);
    expect(req.body.from_node_id).to.equal(textNodeId);
    expect(req.body.to_node_id).to.equal(imageNodeId);

    connectionId = req.body.id;
    console.log('POST connection 返回值:', JSON.stringify(req.body, null, 2));
  });

  test('POST .../connection — 缺少 from_node_id 应返回 400', async ({ request }) => {
    const req = await postConnection(request, canvasId, { to_node_id: imageNodeId });
    expect(req.status).to.equal(400);
    console.log('缺少 from_node_id 的响应:', JSON.stringify(req.body));
  });

  test('POST .../connection — 端点不属于该画布应返回 404', async ({ request }) => {
    const req = await postConnection(request, canvasId, {
      from_node_id: textNodeId,
      to_node_id: NON_EXISTENT_ID,
    });
    expect(req.status).to.equal(404);
    console.log('端点不存在的响应:', JSON.stringify(req.body));
  });

  test('GET /api/seedance2/canvas/{canvas_id}/connection — 获取连线列表', async ({ request }) => {
    expect(connectionId).to.not.be.null;
    const res = await request.get(`${URL}/api/seedance2/canvas/${canvasId}/connection`, { headers: authHeader });
    const req = await readJson(res);

    expect(req.status).to.equal(200);
    expect(req.body).to.be.an('array');
    expect(req.body.map((c) => c.id)).to.include(connectionId);
    console.log('GET connection list 返回值:', JSON.stringify(req.body, null, 2));
  });

  test('DELETE /api/seedance2/canvas/{canvas_id}/connection/{connection_id} — 删除连线', async ({ request }) => {
    expect(connectionId).to.not.be.null;
    const res = await request.delete(`${URL}/api/seedance2/canvas/${canvasId}/connection/${connectionId}`, {
      headers: authHeader,
    });
    const req = await readJson(res);
    expect(req.status).to.equal(204);

    const listRes = await request.get(`${URL}/api/seedance2/canvas/${canvasId}/connection`, { headers: authHeader });
    const list = await readJson(listRes);
    expect(list.body.map((c) => c.id)).to.not.include(connectionId);
    console.log('DELETE connection 后列表:', JSON.stringify(list.body));
  });

  test('DELETE .../connection/{connection_id} — 已删除的连线应返回 404', async ({ request }) => {
    expect(connectionId).to.not.be.null;
    const res = await request.delete(`${URL}/api/seedance2/canvas/${canvasId}/connection/${connectionId}`, {
      headers: authHeader,
    });
    const req = await readJson(res);
    expect(req.status).to.equal(404);
    console.log('重复删除连线的响应:', JSON.stringify(req.body));
  });

  test('GET /api/seedance2/canvas/{canvas_id} — 详情应同时返回节点与连线', async ({ request }) => {
    expect(canvasId).to.not.be.null;
    const connRes = await postConnection(request, canvasId, {
      from_node_id: textNodeId,
      to_node_id: imageNodeId,
    });
    const conn = await readJson(connRes);
    expect(conn.status).to.equal(201);

    const res = await request.get(`${URL}/api/seedance2/canvas/${canvasId}`, { headers: authHeader });
    const req = await readJson(res);

    expect(req.status).to.equal(200);
    expect(req.body.canvas.id).to.equal(canvasId);
    expect(req.body.nodes.map((n) => n.id)).to.have.members([textNodeId, imageNodeId, groupNodeId]);
    expect(req.body.connections.map((c) => c.id)).to.deep.equal([conn.body.id]);
    expect(req.body.person_links).to.deep.equal([]);
    console.log('GET 画布详情:', JSON.stringify(req.body, null, 2));
  });

  test('DELETE .../node/{node_id} — 删除节点应级联删除其连线', async ({ request }) => {
    expect(imageNodeId).to.not.be.null;
    const res = await request.delete(`${URL}/api/seedance2/canvas/${canvasId}/node/${imageNodeId}`, {
      headers: authHeader,
    });
    expect(res.status()).to.equal(204);

    const detailRes = await request.get(`${URL}/api/seedance2/canvas/${canvasId}`, { headers: authHeader });
    const detail = await readJson(detailRes);
    expect(detail.body.nodes.map((n) => n.id)).to.not.include(imageNodeId);
    expect(detail.body.connections).to.deep.equal([]);
    console.log('删除节点后的画布详情:', JSON.stringify(detail.body, null, 2));
  });

  test('DELETE .../node/{node_id} — 已删除的节点应返回 404', async ({ request }) => {
    expect(imageNodeId).to.not.be.null;
    const res = await request.delete(`${URL}/api/seedance2/canvas/${canvasId}/node/${imageNodeId}`, {
      headers: authHeader,
    });
    const req = await readJson(res);
    expect(req.status).to.equal(404);
    console.log('重复删除节点的响应:', JSON.stringify(req.body));
  });

  test('DELETE /api/seedance2/canvas/{canvas_id} — 删除画布', async ({ request }) => {
    expect(canvasId).to.not.be.null;
    const res = await request.delete(`${URL}/api/seedance2/canvas/${canvasId}`, { headers: authHeader });
    expect(res.status()).to.equal(204);

    const listRes = await request.get(`${SUBPROJECT_URL}/${subprojectId}/canvas`, { headers: authHeader });
    const list = await readJson(listRes);
    expect(list.body.map((c) => c.id)).to.not.include(canvasId);
    console.log('删除画布后的列表:', JSON.stringify(list.body));

    canvasId = null;
  });

  test('GET /api/seedance2/canvas/{canvas_id} — 已删除的画布应返回 404', async ({ request }) => {
    const res = await request.get(`${URL}/api/seedance2/canvas/${NON_EXISTENT_ID}`, { headers: authHeader });
    const req = await readJson(res);
    expect(req.status).to.equal(404);
    console.log('不存在画布的响应:', JSON.stringify(req.body));
  });
});

test.describe('seedance2 画布 — 可见人员', () => {
  let subprojectId = null;
  let canvasId = null;
  let otherPersonId = null;

  test.beforeAll(async ({ request }) => {
    subprojectId = await postSubproject(request, 'test_canvas_person');
    const canvasReq = await postCanvas(request, subprojectId, { title: 'test_canvas_person' });
    canvasId = canvasReq.body.id;

    const personsRes = await request.get(`${URL}/api/person`, { headers: authHeader });
    const persons = await readJson(personsRes);
    if (persons.status === 200 && Array.isArray(persons.body)) {
      const candidate = persons.body.find((p) => p.id !== PERSON_ID);
      otherPersonId = candidate ? candidate.id : null;
    }
    console.log('可用人员 id:', otherPersonId);
  });

  test.afterAll(async ({ request }) => {
    if (subprojectId) {
      await request.delete(`${SUBPROJECT_URL}/${subprojectId}`, { headers: authHeader });
    }
  });

  test('POST /api/seedance2/canvas/{canvas_id}/person — person_ids 为空应返回 400', async ({ request }) => {
    expect(canvasId).to.not.be.null;
    const req = await postPerson(request, canvasId, { person_ids: [] });
    expect(req.status).to.equal(400);
    expect(JSON.stringify(req.body)).to.include('person_ids');
    console.log('空 person_ids 的响应:', JSON.stringify(req.body));
  });

  test('POST .../person — 缺少 person_ids 应返回 400', async ({ request }) => {
    const req = await postPerson(request, canvasId, {});
    expect(req.status).to.equal(400);
    console.log('缺少 person_ids 的响应:', JSON.stringify(req.body));
  });

  test('POST .../person — 人员不存在应返回 400（外键约束）', async ({ request }) => {
    const req = await postPerson(request, canvasId, { person_ids: [NON_EXISTENT_ID] });
    expect(req.status).to.equal(400);
    console.log('人员不存在的响应:', JSON.stringify(req.body));
  });

  test('POST .../person — 添加可见人员后可读取并删除', async ({ request }) => {
    test.skip(!otherPersonId, '没有可用的第二名人员');
    const req = await postPerson(request, canvasId, { person_ids: [otherPersonId] });

    expect(req.status, JSON.stringify(req.body)).to.equal(201);
    expect(req.body).to.be.an('array');
    expect(req.body).to.have.length(1);
    const link = req.body[0];
    expect(link.id).to.be.a('string');
    expect(link.infinite_canvas_id).to.equal(canvasId);
    expect(link.person_id).to.equal(otherPersonId);
    console.log('POST person 返回值:', JSON.stringify(req.body, null, 2));

    const detailRes = await request.get(`${URL}/api/seedance2/canvas/${canvasId}`, { headers: authHeader });
    const detail = await readJson(detailRes);
    expect(detail.body.person_links.map((l) => l.id)).to.include(link.id);
    console.log('画布详情中的人员关联:', JSON.stringify(detail.body.person_links, null, 2));

    const delRes = await request.delete(`${URL}/api/seedance2/canvas_link/${link.id}`, { headers: authHeader });
    expect(delRes.status()).to.equal(204);

    const afterRes = await request.get(`${URL}/api/seedance2/canvas/${canvasId}`, { headers: authHeader });
    const after = await readJson(afterRes);
    expect(after.body.person_links).to.deep.equal([]);
    console.log('删除人员关联后的画布详情:', JSON.stringify(after.body.person_links));
  });

  test('POST .../person — 重复添加同一人员应返回 400（唯一索引）', async ({ request }) => {
    test.skip(!otherPersonId, '没有可用的第二名人员');
    const first = await postPerson(request, canvasId, { person_ids: [otherPersonId] });
    expect(first.status).to.equal(201);

    const second = await postPerson(request, canvasId, { person_ids: [otherPersonId] });
    expect(second.status).to.equal(400);
    console.log('重复添加的响应:', JSON.stringify(second.body));

    for (const link of first.body) {
      await request.delete(`${URL}/api/seedance2/canvas_link/${link.id}`, { headers: authHeader });
    }
  });

  test('DELETE /api/seedance2/canvas_link/{canvas_link_id} — 不存在的记录仍返回 204', async ({ request }) => {
    const res = await request.delete(`${URL}/api/seedance2/canvas_link/${NON_EXISTENT_ID}`, {
      headers: authHeader,
    });
    expect(res.status()).to.equal(204);
  });
});

test.describe('seedance2 画布 — 鉴权', () => {
  let subprojectId = null;
  let canvasId = null;
  let nodeId = null;

  test.beforeAll(async ({ request }) => {
    subprojectId = await postSubproject(request, 'test_canvas_auth');
    const canvasReq = await postCanvas(request, subprojectId, { title: 'test_canvas_auth' });
    canvasId = canvasReq.body.id;
    const nodeReq = await postNode(request, canvasId, { type: 'text', title: '鉴权用节点' });
    nodeId = nodeReq.body.id;
  });

  test.afterAll(async ({ request }) => {
    if (subprojectId) {
      await request.delete(`${SUBPROJECT_URL}/${subprojectId}`, { headers: authHeader });
    }
  });

  const noAuthCases = [
    ['GET 画布详情', 'GET', () => `${URL}/api/seedance2/canvas/${canvasId}`],
    ['POST 节点', 'POST', () => `${URL}/api/seedance2/canvas/${canvasId}/node`],
    ['GET 节点列表', 'GET', () => `${URL}/api/seedance2/canvas/${canvasId}/node`],
    ['GET 节点详情', 'GET', () => `${URL}/api/seedance2/canvas/${canvasId}/node/${nodeId}`],
    ['GET 连线列表', 'GET', () => `${URL}/api/seedance2/canvas/${canvasId}/connection`],
    ['POST 可见人员', 'POST', () => `${URL}/api/seedance2/canvas/${canvasId}/person`],
    ['DELETE 可见人员', 'DELETE', () => `${URL}/api/seedance2/canvas_link/${NON_EXISTENT_ID}`],
  ];

  for (const [label, method, urlOf] of noAuthCases) {
    test(`${label} — 无 JWT 应返回 401`, async ({ request }) => {
      const res = await request.fetch(urlOf(), {
        method,
        data: method === 'GET' || method === 'DELETE' ? undefined : {},
      });
      expect(res.status()).to.equal(401);
    });
  }

  test('带 JWT 但 Cookie 值非法应返回 401', async ({ request }) => {
    const res = await request.get(`${URL}/api/seedance2/canvas/${canvasId}`, {
      headers: { Cookie: 'access_token_cookie=not-a-jwt' },
    });
    expect(res.status()).to.equal(401);
  });

  test('JWT 中的人员不存在应返回 404 或 401', async ({ request }) => {
    const bogusJwt = JWT.slice(0, -4) + 'AAAA';
    const res = await request.get(`${URL}/api/seedance2/canvas/${canvasId}`, {
      headers: { Cookie: `access_token_cookie=${bogusJwt}` },
    });
    expect([401, 404]).to.include(res.status());
    console.log('篡改 JWT 的状态码:', res.status());
  });
});

test.describe('seedance2 画布 — 枚举与默认值', () => {
  let subprojectId = null;
  let canvasId = null;

  test.beforeAll(async ({ request }) => {
    subprojectId = await postSubproject(request, 'test_canvas_enum');
    const canvasReq = await postCanvas(request, subprojectId, { title: 'test_canvas_enum' });
    canvasId = canvasReq.body.id;
  });

  test.afterAll(async ({ request }) => {
    if (subprojectId) {
      await request.delete(`${SUBPROJECT_URL}/${subprojectId}`, { headers: authHeader });
    }
  });

  const validTypes = ['text', 'image', 'panorama', 'video', 'audio', 'config', 'director', 'group'];

  for (const type of validTypes) {
    test(`POST .../node — type=${type} 应创建成功`, async ({ request }) => {
      const req = await postNode(request, canvasId, { type, title: `节点_${type}` });
      expect(req.status, JSON.stringify(req.body)).to.equal(201);
      expect(req.body.type).to.equal(type);

      const delRes = await request.delete(`${URL}/api/seedance2/canvas/${canvasId}/node/${req.body.id}`, {
        headers: authHeader,
      });
      expect(delRes.status()).to.equal(204);
    });
  }

  test('POST .../canvas — background_mode 缺省应为 dots', async ({ request }) => {
    const req = await postCanvas(request, subprojectId, { title: 'default_bg' });
    expect(req.status).to.equal(201);
    expect(req.body.background_mode).to.equal('dots');
    expect(req.body.viewport).to.deep.equal({ x: 0, y: 0, k: 1 });
    console.log('默认背景模式的画布:', JSON.stringify(req.body, null, 2));

    await request.delete(`${URL}/api/seedance2/canvas/${req.body.id}`, { headers: authHeader });
  });

  for (const mode of ['blank', 'lines', 'dots']) {
    test(`PUT .../canvas — background_mode=${mode} 应更新成功`, async ({ request }) => {
      const create = await postCanvas(request, subprojectId, { title: `bg_${mode}` });
      expect(create.status).to.equal(201);

      const res = await request.put(`${URL}/api/seedance2/canvas/${create.body.id}`, {
        headers: authHeader,
        data: { background_mode: mode },
      });
      const req = await readJson(res);
      expect(req.status).to.equal(200);
      expect(req.body.background_mode).to.equal(mode);

      await request.delete(`${URL}/api/seedance2/canvas/${create.body.id}`, { headers: authHeader });
    });
  }

  test('POST .../node — 未知 metadata 字段应原样保留', async ({ request }) => {
    const metadata = {
      content: '带任意字段',
      previewUrl: 'https://example.com/a.png',
      naturalWidth: 1920,
      naturalHeight: 1080,
      customPluginField: { nested: [1, 2, 3] },
    };
    const req = await postNode(request, canvasId, { type: 'panorama', title: '任意字段', metadata });

    expect(req.status).to.equal(201);
    expect(req.body.metadata).to.deep.equal(metadata);

    const getRes = await request.get(`${URL}/api/seedance2/canvas/${canvasId}/node/${req.body.id}`, {
      headers: authHeader,
    });
    const got = await readJson(getRes);
    expect(got.body.metadata).to.deep.equal(metadata);
    console.log('任意 metadata 回读:', JSON.stringify(got.body.metadata, null, 2));

    await request.delete(`${URL}/api/seedance2/canvas/${canvasId}/node/${req.body.id}`, { headers: authHeader });
  });

  test('PUT .../node — metadata 整体替换而非合并', async ({ request }) => {
    const create = await postNode(request, canvasId, {
      type: 'text',
      title: 'metadata 替换',
      metadata: { content: 'first', fontSize: 12 },
    });
    expect(create.status).to.equal(201);

    const putReq = await putNode(request, canvasId, create.body.id, { metadata: { content: 'second' } });
    expect(putReq.status).to.equal(200);
    expect(putReq.body.metadata).to.deep.equal({ content: 'second' });
    console.log('metadata 替换后的节点:', JSON.stringify(putReq.body.metadata));

    await request.delete(`${URL}/api/seedance2/canvas/${canvasId}/node/${create.body.id}`, { headers: authHeader });
  });

  test('POST .../node — 生成的 id 应为合法 uuid', async ({ request }) => {
    const req = await postNode(request, canvasId, { type: 'text', title: 'uuid 检查' });
    expect(req.status).to.equal(201);
    expect(req.body.id).to.match(/^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/i);

    await request.delete(`${URL}/api/seedance2/canvas/${canvasId}/node/${req.body.id}`, { headers: authHeader });
  });

  test('POST .../node — 客户端自带 uuid 应被忽略', async ({ request }) => {
    const fakeId = uuidv4();
    const req = await postNode(request, canvasId, { id: fakeId, type: 'text', title: '自带 id' });
    expect(req.status).to.equal(201);
    expect(req.body.id).to.not.equal(fakeId);
    console.log('客户端自带 id 被忽略:', fakeId, '→', req.body.id);

    await request.delete(`${URL}/api/seedance2/canvas/${canvasId}/node/${req.body.id}`, { headers: authHeader });
  });
});
