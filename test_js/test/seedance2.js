// const superagent = require('superagent');
import request from 'superagent';
import { expect } from 'chai';

import fs from 'node:fs';
import { URL, JWT } from './config.js';

describe('seedance2 测试', function () {

  const authHeader = { 'Cookie': `access_token_cookie=${JWT}` };
  let subprojectId = null;

  it('POST /api/seedance2/subproject — 创建子项目', async function () {
    const req = await request.post(`${URL}/api/seedance2/subproject`)
      .set(authHeader)
      .send({
        name: 'test_subproject',
        project_id: 'c340051a-45a6-4af1-a750-efefe639c75b',
      });
    expect(req.status).to.equal(201);
    subprojectId = req.body.id;
    console.log('POST 返回值:', JSON.stringify(req.body, null, 2));
  });

  it('GET /api/seedance2/subproject/{id} — 获取子项目详情', async function () {
    expect(subprojectId).to.not.be.null;
    const req = await request.get(`${URL}/api/seedance2/subproject/${subprojectId}`).set(authHeader);
    expect(req.status).to.equal(200);
    console.log('GET instance 返回值:', JSON.stringify(req.body, null, 2));
  });

  it('PUT /api/seedance2/subproject/{id} — 更新子项目', async function () {
    expect(subprojectId).to.not.be.null;
    const req = await request.put(`${URL}/api/seedance2/subproject/${subprojectId}`)
      .set(authHeader)
      .send({ name: 'test_subproject_updated' });
    expect(req.status).to.equal(200);
    console.log('PUT 返回值:', JSON.stringify(req.body, null, 2));
  });

  it('GET /api/seedance2/subproject — 获取子项目列表', async function () {
    const req = await request.get(`${URL}/api/seedance2/subproject`).set(authHeader);
    expect(req.status).to.equal(200);
    console.log('GET 返回值:', JSON.stringify(req.body, null, 2));
  });

  it('DELETE /api/seedance2/subproject/{id} — 删除子项目', async function () {
    expect(subprojectId).to.not.be.null;
    const req = await request.delete(`${URL}/api/seedance2/subproject/${subprojectId}`).set(authHeader);
    expect(req.status).to.equal(204);
    console.log('DELETE 返回值:', JSON.stringify(req.body, null, 2));
  });

});

describe('seedance2 person link 测试', function () {

  const authHeader = { 'Cookie': `access_token_cookie=${JWT}` };
  const personId = '69a8d093-dcab-4890-8f9d-c51ef065d03b';
  let subprojectId = null;
  let linkId = null;

  before(async function () {
    const req = await request.post(`${URL}/api/seedance2/subproject`)
      .set(authHeader)
      .send({
        name: 'test_subproject_person_link',
        project_id: 'c340051a-45a6-4af1-a750-efefe639c75b',
      });
    subprojectId = req.body.id;
  });

  after(async function () {
    if (subprojectId) {
      await request.delete(`${URL}/api/seedance2/subproject/${subprojectId}`).set(authHeader);
    }
  });

  it('POST /api/seedance2/subproject/{subproject_id}/person — 添加参与人员', async function () {
    expect(subprojectId).to.not.be.null;
    const req = await request.post(`${URL}/api/seedance2/subproject/${subprojectId}/person`)
      .set(authHeader)
      .send({
        subproject_id: subprojectId,
        person_id: personId,
      });
    expect(req.status).to.equal(201);
    linkId = req.body.id;
    console.log('POST person link 返回值:', JSON.stringify(req.body, null, 2));
  });

  it('DELETE /api/seedance2/subproject/{subproject_id}/person — 移除参与人员', async function () {
    expect(subprojectId).to.not.be.null;
    const req = await request.delete(`${URL}/api/seedance2/subproject/${subprojectId}/person`)
      .set(authHeader)
      .send({ person_id: personId });
    expect(req.status).to.equal(200);
    console.log('DELETE person link 返回值:', JSON.stringify(req.body, null, 2));
  });

});

describe('seedance2 episodes 测试', function () {

  const authHeader = { 'Cookie': `access_token_cookie=${JWT}` };
  let subprojectId = null;
  let episodeId = null;

  before(async function () {
    const req = await request.post(`${URL}/api/seedance2/subproject`)
      .set(authHeader)
      .send({
        name: 'test_subproject_episodes',
        project_id: 'c340051a-45a6-4af1-a750-efefe639c75b',
      });
    subprojectId = req.body.id;
  });

  after(async function () {
    if (subprojectId) {
      await request.delete(`${URL}/api/seedance2/subproject/${subprojectId}`).set(authHeader);
    }
  });

  it('POST /api/seedance2/subproject/{subproject_id}/episodes — 创建剧集', async function () {
    expect(subprojectId).to.not.be.null;
    const req = await request.post(`${URL}/api/seedance2/subproject/${subprojectId}/episodes`)
      .set(authHeader)
      .send({
        name: 'sc001',
        subproject_id: subprojectId,
        description: '测试剧集',
      });
    expect(req.status).to.equal(201);
    episodeId = req.body.id;
    console.log('POST episodes 返回值:', JSON.stringify(req.body, null, 2));
  });

  it('GET /api/seedance2/subproject/{subproject_id}/episodes — 获取剧集列表', async function () {
    expect(subprojectId).to.not.be.null;
    expect(episodeId).to.not.be.null;
    const req = await request.get(`${URL}/api/seedance2/subproject/${subprojectId}/episodes`)
      .set(authHeader);
    expect(req.status).to.equal(200);
    expect(req.body).to.be.an('array');
    expect(req.body.some((e) => e.id === episodeId)).to.equal(true);
    console.log('GET episodes list 返回值:', JSON.stringify(req.body, null, 2));
  });

  it('GET /api/seedance2/subproject/{subproject_id}/episodes/{episode_id} — 获取剧集详情', async function () {
    expect(episodeId).to.not.be.null;
    const req = await request.get(`${URL}/api/seedance2/subproject/${subprojectId}/episodes/${episodeId}`)
      .set(authHeader);
    expect(req.status).to.equal(200);
    console.log('GET episodes instance 返回值:', JSON.stringify(req.body, null, 2));
  });

  it('PUT /api/seedance2/subproject/{subproject_id}/episodes/{episode_id} — 更新剧集', async function () {
    expect(episodeId).to.not.be.null;
    const req = await request.put(`${URL}/api/seedance2/subproject/${subprojectId}/episodes/${episodeId}`)
      .set(authHeader)
      .send({ name: 'sc001_updated', description: '更新后的剧集' });
    expect(req.status).to.equal(200);
    console.log('PUT episodes 返回值:', JSON.stringify(req.body, null, 2));
  });

  it('DELETE /api/seedance2/subproject/{subproject_id}/episodes/{episode_id} — 删除剧集', async function () {
    expect(episodeId).to.not.be.null;
    const req = await request.delete(`${URL}/api/seedance2/subproject/${subprojectId}/episodes/${episodeId}`)
      .set(authHeader);
    expect(req.status).to.equal(200);
    console.log('DELETE episodes 返回值:', JSON.stringify(req.body, null, 2));
  });

});

describe('seedance2 entity 测试', function () {

  const authHeader = { 'Cookie': `access_token_cookie=${JWT}` };
  const projectId = 'c340051a-45a6-4af1-a750-efefe639c75b';
  let subprojectId = null;
  let episodeId = null;
  let entityId = null;

  before(async function () {
    const subReq = await request.post(`${URL}/api/seedance2/subproject`)
      .set(authHeader)
      .send({
        name: 'test_subproject_entity',
        project_id: projectId,
      });
    subprojectId = subReq.body.id;

    const clsReq = await request.post(`${URL}/api/seedance2/subproject/${subprojectId}/episodes`)
      .set(authHeader)
      .send({
        name: 'sc001',
        subproject_id: subprojectId,
      });
    episodeId = clsReq.body.id;
  });

  after(async function () {
    if (subprojectId) {
      await request.delete(`${URL}/api/seedance2/subproject/${subprojectId}`).set(authHeader);
    }
  });

  it('POST /api/seedance2/subproject/{subproject_id}/entity — 创建实体', async function () {
    expect(episodeId).to.not.be.null;
    const req = await request.post(`${URL}/api/seedance2/subproject/${subprojectId}/entity`)
      .set(authHeader)
      .send({
        name: 'test_entity',
        project_uuid_id: projectId,
        ai_episode_id: episodeId,
        ai_category_id: '01a03b8f-d9f3-71f1-808f-a098a698c889'
      });
    expect(req.status).to.equal(201);
    entityId = req.body.id;
    console.log('POST entity 返回值:', JSON.stringify(req.body, null, 2));
  });

  it('POST /api/seedance2/subproject/{subproject_id}/entity/{entity_id}/reference — 上传实体参考文件', async function () {
    this.timeout(330000);
    expect(entityId).to.not.be.null;
    const mp4Path = 'D:\\无标题(1).jpg';
    const req = await request.post(`${URL}/api/seedance2/subproject/${subprojectId}/entity/${entityId}/reference`)
      .set(authHeader)
      .attach('file', fs.createReadStream(mp4Path), '无标题(1).jpg')
      .timeout(300000);
    expect(req.status).to.equal(201);
    console.log('POST entity reference 返回值:', JSON.stringify(req.body, null, 2));
  });

  it('GET /api/seedance2/subproject/{subproject_id}/episodes/{episode_id}/entity — 获取实体列表', async function () {
    expect(episodeId).to.not.be.null;
    const req = await request.get(`${URL}/api/seedance2/subproject/${subprojectId}/episodes/${episodeId}/entity`)
      .set(authHeader);
    expect(req.status).to.equal(200);
    console.log('GET entity list 返回值:', JSON.stringify(req.body, null, 2));
  });

  it('GET /api/seedance2/subproject/{subproject_id}/entity/{entity_id} — 获取实体详情', async function () {
    expect(entityId).to.not.be.null;
    const req = await request.get(`${URL}/api/seedance2/subproject/${subprojectId}/entity/${entityId}`)
      .set(authHeader);
    expect(req.status).to.equal(200);
    console.log('GET entity instance 返回值:', JSON.stringify(req.body, null, 2));
  });

  it('PUT /api/seedance2/subproject/{subproject_id}/entity/{entity_id} — 更新实体', async function () {
    expect(entityId).to.not.be.null;
    const req = await request.put(`${URL}/api/seedance2/subproject/${subprojectId}/entity/${entityId}`)
      .set(authHeader)
      .send({ name: 'test_entity_updated' });
    expect(req.status).to.equal(200);
    console.log('PUT entity 返回值:', JSON.stringify(req.body, null, 2));
  });

  it('POST /api/seedance2/subproject/{subproject_id}/entity/{entity_id}/depth — 上传视频进行深度估计', async function () {
    this.timeout(330000);
    expect(entityId).to.not.be.null;
    const mp4Path = "D:\\test_files\\test_depth.mp4";
    const req = await request.post(`${URL}/api/seedance2/subproject/${subprojectId}/entity/${entityId}/depth`)
      .set(authHeader)
      .attach('file', fs.createReadStream(mp4Path), 'test_depth.mp4')
      .timeout(300000);
    expect(req.status).to.equal(201);
    console.log('POST depth 返回值:', JSON.stringify(req.body, null, 2));
  });

  it('DELETE /api/seedance2/subproject/{subproject_id}/entity/{entity_id} — 删除实体', async function () {
    expect(entityId).to.not.be.null;
    const req = await request.delete(`${URL}/api/seedance2/subproject/${subprojectId}/entity/${entityId}`)
      .set(authHeader);
    expect(req.status).to.equal(200);
    console.log('DELETE entity 返回值:', JSON.stringify(req.body, null, 2));
  });

});

describe('seedance2 task', function () {

  const authHeader = { 'Cookie': `access_token_cookie=${JWT}` };
  const projectId = 'c340051a-45a6-4af1-a750-efefe639c75b';
  const model = 'doubao-seedance-1-0-pro-250528';
  const resolution = '1080p';
  let subprojectId = null;
  let episodeId = null;
  let entityId = null;
  let taskId = null;

  before(async function () {
    const subReq = await request.post(`${URL}/api/seedance2/subproject`)
      .set(authHeader)
      .send({
        name: 'test_subproject_task',
        project_id: projectId,
      });
    subprojectId = subReq.body.id;

    const clsReq = await request.post(`${URL}/api/seedance2/subproject/${subprojectId}/episodes`)
      .set(authHeader)
      .send({
        name: 'sc001',
        subproject_id: subprojectId,
      });
    episodeId = clsReq.body.id;

    // 授权「模型 + 分辨率」组合, 否则提交任务会被 401 拒绝
    await request.post(`${URL}/api/seedance2/subproject/${subprojectId}/episodes/${episodeId}/model-resolution-limit`)
      .set(authHeader)
      .send({ model_name: model, resolution });

    // 实体必须同时带 ai_category_id 与 ai_episode_id:
    // ai_category_id 是 ai_generate_entity 反序列化的必填项, ai_episode_id 用于提交任务时的模型授权校验
    const catReq = await request.post(`${URL}/api/seedance2/subproject/${subprojectId}/category`)
      .set(authHeader)
      .send({ name: 'cat_task', type: 'shot', description: '任务测试类别' });
    const categoryId = catReq.body.id;

    const entReq = await request.post(`${URL}/api/seedance2/subproject/${subprojectId}/entity`)
      .set(authHeader)
      .send({
        name: 'test_entity',
        project_uuid_id: projectId,
        ai_episode_id: episodeId,
        ai_category_id: categoryId,
      });
    entityId = entReq.body.id;
  });

  after(async function () {
    if (subprojectId) {
      await request.delete(`${URL}/api/seedance2/subproject/${subprojectId}`).set(authHeader);
    }
  });

  it('POST /api/seedance2/subproject/{subproject_id}/entity/{entity_id}/task — 创建任务', async function () {
    expect(entityId).to.not.be.null;
    const req = await request.post(`${URL}/api/seedance2/subproject/${subprojectId}/entity/${entityId}/task`)
      .set(authHeader)
      .send({
        backend: 'seedance2',
        data_request: {
          model,
          resolution,
          content: [
            { type: 'text', text: 'A test prompt for seedance2 task' },
            { type: 'image_url', image_url: { url: "/api/seedance2/pictures/01a01da2-876d-7208-803f-e1b7938d15d9.png" } }
          ],
        },
        ai_studio_id: '019e2075-6e42-718e-809e-33a92410c682',
        project_uuid_id: projectId,
        type: 'video',
        ai_generate_entity_id: entityId,
      });
    expect(req.status).to.equal(201);
    taskId = req.body.id;
    console.log('POST task 返回值:', JSON.stringify(req.body, null, 2));
  });

  it('GET /api/seedance2/subproject/{subproject_id}/entity/{entity_id}/task — 获取任务列表', async function () {
    expect(entityId).to.not.be.null;
    const req = await request.get(`${URL}/api/seedance2/subproject/${subprojectId}/entity/${entityId}/task`)
      .set(authHeader);
    expect(req.status).to.equal(200);
    console.log('GET task list 返回值:', JSON.stringify(req.body, null, 2));
  });

  it('GET /api/seedance2/subproject/{subproject_id}/task/{id} — 获取任务详情', async function () {
    expect(taskId).to.not.be.null;
    const req = await request.get(`${URL}/api/seedance2/subproject/${subprojectId}/task/${taskId}`)
      .set(authHeader);
    expect(req.status).to.equal(200);
    console.log('GET task instance 返回值:', JSON.stringify(req.body, null, 2));
  });

  it('PUT /api/seedance2/subproject/{subproject_id}/task/{id} — 取消任务', async function () {
    expect(taskId).to.not.be.null;
    const req = await request.put(`${URL}/api/seedance2/subproject/${subprojectId}/task/${taskId}`)
      .set(authHeader);
    expect(req.status).to.equal(200);
    console.log('PUT task 返回值:', JSON.stringify(req.body, null, 2));
  });

  it('DELETE /api/seedance2/subproject/{subproject_id}/task/{id} — 归档任务', async function () {
    expect(taskId).to.not.be.null;
    const req = await request.delete(`${URL}/api/seedance2/subproject/${subprojectId}/task/${taskId}`)
      .set(authHeader);
    expect(req.status).to.equal(200);
    console.log('DELETE task 返回值:', JSON.stringify(req.body, null, 2));
  });

});

describe('seedance2 ai category 测试', function () {

  const authHeader = { 'Cookie': `access_token_cookie=${JWT}` };
  const projectId = 'c340051a-45a6-4af1-a750-efefe639c75b';
  let subprojectId = null;
  let categoryId = null;
  let entityId = null;

  before(async function () {
    const subReq = await request.post(`${URL}/api/seedance2/subproject`)
      .set(authHeader)
      .send({
        name: 'test_subproject_ai_category',
        project_id: projectId,
      });
    subprojectId = subReq.body.id;
  });

  after(async function () {
    if (subprojectId) {
      await request.delete(`${URL}/api/seedance2/subproject/${subprojectId}`).set(authHeader);
    }
  });

  it('POST /api/seedance2/subproject/{subproject_id}/category — 创建类别', async function () {
    expect(subprojectId).to.not.be.null;
    const req = await request.post(`${URL}/api/seedance2/subproject/${subprojectId}/category`)
      .set(authHeader)
      .send({
        name: 'cat001',
        type: 'assets',
        description: '测试类别',
      });
    expect(req.status).to.equal(201);
    categoryId = req.body.id;
    console.log('POST category 返回值:', JSON.stringify(req.body, null, 2));
  });

  it('GET /api/seedance2/subproject/{subproject_id}/category — 获取类别列表', async function () {
    expect(subprojectId).to.not.be.null;
    const req = await request.get(`${URL}/api/seedance2/subproject/${subprojectId}/category`)
      .set(authHeader);
    expect(req.status).to.equal(200);
    console.log('GET category list 返回值:', JSON.stringify(req.body, null, 2));
  });

  it('GET /api/seedance2/subproject/{subproject_id}/category/{category_id} — 获取类别详情', async function () {
    expect(categoryId).to.not.be.null;
    const req = await request.get(`${URL}/api/seedance2/subproject/${subprojectId}/category/${categoryId}`)
      .set(authHeader);
    expect(req.status).to.equal(200);
    expect(req.body.type).to.equal('assets');
    console.log('GET category instance 返回值:', JSON.stringify(req.body, null, 2));
  });

  it('PUT /api/seedance2/subproject/{subproject_id}/category/{category_id} — 更新类别', async function () {
    expect(categoryId).to.not.be.null;
    const req = await request.put(`${URL}/api/seedance2/subproject/${subprojectId}/category/${categoryId}`)
      .set(authHeader)
      .send({ name: 'cat001_updated', type: 'assets', description: '更新后的类别' });
    expect(req.status).to.equal(200);
    console.log('PUT category 返回值:', JSON.stringify(req.body, null, 2));
  });

  it('POST /api/seedance2/subproject/{subproject_id}/entity — 创建关联类别的实体', async function () {
    expect(categoryId).to.not.be.null;
    const req = await request.post(`${URL}/api/seedance2/subproject/${subprojectId}/entity`)
      .set(authHeader)
      .send({
        name: 'test_entity_ai_category',
        project_uuid_id: projectId,
        ai_category_id: categoryId,
      });
    expect(req.status).to.equal(201);
    entityId = req.body.id;
    console.log('POST entity 返回值:', JSON.stringify(req.body, null, 2));
  });

  it('GET /api/seedance2/subproject/{subproject_id}/category/{category_id}/entity — 获取类别下实体列表', async function () {
    expect(categoryId).to.not.be.null;
    const req = await request.get(`${URL}/api/seedance2/subproject/${subprojectId}/category/${categoryId}/entity`)
      .set(authHeader);
    expect(req.status).to.equal(200);
    console.log('GET category entity 返回值:', JSON.stringify(req.body, null, 2));
  });

  it('DELETE /api/seedance2/subproject/{subproject_id}/category/{category_id} — 删除类别', async function () {
    expect(categoryId).to.not.be.null;
    const req = await request.delete(`${URL}/api/seedance2/subproject/${subprojectId}/category/${categoryId}`)
      .set(authHeader);
    expect(req.status).to.equal(200);
    console.log('DELETE category 返回值:', JSON.stringify(req.body, null, 2));
  });

  it('DELETE /api/seedance2/subproject/{subproject_id}/entity/{entity_id} — 清理关联实体', async function () {
    expect(entityId).to.not.be.null;
    const req = await request.delete(`${URL}/api/seedance2/subproject/${subprojectId}/entity/${entityId}`)
      .set(authHeader);
    expect(req.status).to.equal(200);
    console.log('DELETE entity 返回值:', JSON.stringify(req.body, null, 2));
  });

});

describe('seedance2 task — transfer_station 后端', function () {

  const authHeader = { 'Cookie': `access_token_cookie=${JWT}` };
  const projectId = 'c340051a-45a6-4af1-a750-efefe639c75b';
  // 已在 g_model_pricings 中登记定价的模型
  const model = 'nano-banana-2';
  const resolution = '1K';
  // 上游支持但未登记定价的模型, 提交后应被 run_task 拒绝
  const unpricedModel = 'nano-banana-pro-vt';
  // 未授权给剧集的模型, 用于验证授权校验
  const unauthorizedModel = 'gpt-image-2';

  let subprojectId = null;
  let episodeId = null;
  let entityId = null;
  let taskId = null;

  // 轮询任务状态, 直到命中期望状态或超时
  async function waitForTaskStatus(id, statuses, timeoutMs = 40000) {
    const deadline = Date.now() + timeoutMs;
    let last = null;
    while (Date.now() < deadline) {
      const res = await request.get(`${URL}/api/seedance2/subproject/${subprojectId}/task/${id}`)
        .set(authHeader);
      last = res.body;
      if (statuses.includes(last.status)) return last;
      await new Promise((resolve) => setTimeout(resolve, 1500));
    }
    return last;
  }

  // 构造 transfer_station 后端的任务请求体
  function transferStationBody(overrides = {}) {
    return {
      backend: 'transfer_station',
      data_request: {
        model,
        prompt: '一只边牧与古牧正在抖音直播间直播带货截图',
        imageSize: resolution,
        aspectRatio: '1:1',
        images: [],
      },
      ai_studio_id: '019e2075-6e42-718e-809e-33a92410c682',
      project_uuid_id: projectId,
      type: 'picture',
      ai_generate_entity_id: entityId,
      ...overrides,
    };
  }

  // 提交任务, 允许非 2xx 以便断言状态码
  function postTask(body) {
    return request.post(`${URL}/api/seedance2/subproject/${subprojectId}/entity/${entityId}/task`)
      .set(authHeader)
      .ok(() => true)
      .send(body);
  }

  before(async function () {
    this.timeout(60000);
    const subReq = await request.post(`${URL}/api/seedance2/subproject`)
      .set(authHeader)
      .send({
        name: 'test_subproject_transfer_station',
        project_id: projectId,
      });
    subprojectId = subReq.body.id;

    const clsReq = await request.post(`${URL}/api/seedance2/subproject/${subprojectId}/episodes`)
      .set(authHeader)
      .send({
        name: 'sc001',
        subproject_id: subprojectId,
      });
    episodeId = clsReq.body.id;

    // 授权「模型 + 分辨率」组合, 否则提交任务会被 401 拒绝
    await request.post(`${URL}/api/seedance2/subproject/${subprojectId}/episodes/${episodeId}/model-resolution-limit`)
      .set(authHeader)
      .send({ model_name: model, resolution });

    // 实体必须同时带 ai_category_id 与 ai_episode_id:
    // ai_category_id 是 ai_generate_entity 反序列化的必填项, ai_episode_id 用于提交任务时的模型授权校验
    const catReq = await request.post(`${URL}/api/seedance2/subproject/${subprojectId}/category`)
      .set(authHeader)
      .send({ name: 'cat_transfer_station', type: 'shot', description: '中转站测试类别' });
    const categoryId = catReq.body.id;

    const entReq = await request.post(`${URL}/api/seedance2/subproject/${subprojectId}/entity`)
      .set(authHeader)
      .send({
        name: 'test_entity_transfer_station',
        project_uuid_id: projectId,
        ai_episode_id: episodeId,
        ai_category_id: categoryId,
      });
    entityId = entReq.body.id;
  });

  after(async function () {
    if (subprojectId) {
      await request.delete(`${URL}/api/seedance2/subproject/${subprojectId}`).set(authHeader);
    }
  });

  it('POST .../entity/{entity_id}/task — 使用 transfer_station 后端提交图片任务', async function () {
    expect(entityId).to.not.be.null;
    const req = await postTask(transferStationBody());
    expect(req.status).to.equal(201);
    taskId = req.body.id;
    expect(taskId).to.be.a('string');
    console.log('POST transfer_station task 返回值:', JSON.stringify(req.body, null, 2));
  });

  it('GET .../task/{id} — backend 与提示词应按 transfer_station 规则落库', async function () {
    expect(taskId).to.not.be.null;
    const req = await request.get(`${URL}/api/seedance2/subproject/${subprojectId}/task/${taskId}`)
      .set(authHeader);
    expect(req.status).to.equal(200);
    expect(req.body.backend).to.equal('transfer_station');
    expect(req.body.type).to.equal('picture');
    // 提示词取自 transfer_station 的 prompt 字段, 而非 seedance2 的 content[]
    expect(req.body.text_prompt).to.equal('一只边牧与古牧正在抖音直播间直播带货截图');
    console.log('GET transfer_station task 返回值:', JSON.stringify(req.body, null, 2));
  });

  it('POST .../task — 缺少模型名称应返回 400', async function () {
    const req = await postTask(transferStationBody({ data_request: { prompt: '没有模型名称的请求' } }));
    expect(req.status).to.equal(400);
  });

  it('POST .../task — 缺少分辨率应返回 400', async function () {
    const req = await postTask(transferStationBody({ data_request: { model, prompt: '没有分辨率的请求' } }));
    expect(req.status).to.equal(400);
  });

  it('POST .../task — 未授权的模型或分辨率应返回 401', async function () {
    const req = await postTask(
      transferStationBody({
        data_request: { model: unauthorizedModel, prompt: '未授权模型', aspectRatio: '2048x2048' },
      })
    );
    expect(req.status).to.equal(401);
  });

  it('未登记定价的模型提交后任务应失败且不扣费', async function () {
    this.timeout(90000);
    // 先授权该未定价模型, 让它通过 POST 的授权校验, 从而走到 run_task 的定价检查
    await request.post(`${URL}/api/seedance2/subproject/${subprojectId}/episodes/${episodeId}/model-resolution-limit`)
      .set(authHeader)
      .send({ model_name: unpricedModel, resolution });

    const createReq = await postTask(
      transferStationBody({
        data_request: { model: unpricedModel, prompt: '未登记定价的模型', imageSize: resolution },
      })
    );
    expect(createReq.status).to.equal(201);

    // 异步运行器拾取 preparing 任务后, 因未登记定价直接置为 failed
    // 轮询窗口取满 this.timeout, 因为运行器可能先处理上一条仍在 preparing 的任务
    const task = await waitForTaskStatus(createReq.body.id, ['failed'], 80000);
    expect(task.status).to.equal('failed');
    expect(JSON.stringify(task.data_response)).to.include('未登记定价');
    // 失败任务不扣费
    expect(task.completion_tokens).to.equal(0);
    console.log('未登记定价任务返回值:', JSON.stringify(task, null, 2));
  });

});


