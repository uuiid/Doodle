// const superagent = require('superagent');
import request from 'superagent';
import { expect } from 'chai';

// const fs = require('node:fs');
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

describe('seedance2 classification 测试', function () {

  const authHeader = { 'Cookie': `access_token_cookie=${JWT}` };
  let subprojectId = null;
  let classificationId = null;

  before(async function () {
    const req = await request.post(`${URL}/api/seedance2/subproject`)
      .set(authHeader)
      .send({
        name: 'test_subproject_classification',
        project_id: 'c340051a-45a6-4af1-a750-efefe639c75b',
      });
    subprojectId = req.body.id;
  });

  after(async function () {
    if (subprojectId) {
      await request.delete(`${URL}/api/seedance2/subproject/${subprojectId}`).set(authHeader);
    }
  });

  it('POST /api/seedance2/subproject/{subproject_id}/classification — 创建分类', async function () {
    expect(subprojectId).to.not.be.null;
    const req = await request.post(`${URL}/api/seedance2/subproject/${subprojectId}/classification`)
      .set(authHeader)
      .send({
        name: 'sc001',
        subproject_id: subprojectId,
        description: '测试分类',
      });
    expect(req.status).to.equal(201);
    classificationId = req.body.id;
    console.log('POST classification 返回值:', JSON.stringify(req.body, null, 2));
  });

  it('GET /api/seedance2/subproject/{subproject_id}/classification — 获取分类列表', async function () {
    expect(subprojectId).to.not.be.null;
    const req = await request.get(`${URL}/api/seedance2/subproject/${subprojectId}/classification`)
      .set(authHeader);
    expect(req.status).to.equal(200);
    console.log('GET classification list 返回值:', JSON.stringify(req.body, null, 2));
  });

  it('GET /api/seedance2/subproject/{subproject_id}/classification/{classification_id} — 获取分类详情', async function () {
    expect(classificationId).to.not.be.null;
    const req = await request.get(`${URL}/api/seedance2/subproject/${subprojectId}/classification/${classificationId}`)
      .set(authHeader);
    expect(req.status).to.equal(200);
    console.log('GET classification instance 返回值:', JSON.stringify(req.body, null, 2));
  });

  it('PUT /api/seedance2/subproject/{subproject_id}/classification/{classification_id} — 更新分类', async function () {
    expect(classificationId).to.not.be.null;
    const req = await request.put(`${URL}/api/seedance2/subproject/${subprojectId}/classification/${classificationId}`)
      .set(authHeader)
      .send({ name: 'sc001_updated', description: '更新后的分类' });
    expect(req.status).to.equal(200);
    console.log('PUT classification 返回值:', JSON.stringify(req.body, null, 2));
  });

  it('DELETE /api/seedance2/subproject/{subproject_id}/classification/{classification_id} — 删除分类', async function () {
    expect(classificationId).to.not.be.null;
    const req = await request.delete(`${URL}/api/seedance2/subproject/${subprojectId}/classification/${classificationId}`)
      .set(authHeader);
    expect(req.status).to.equal(200);
    console.log('DELETE classification 返回值:', JSON.stringify(req.body, null, 2));
  });

});

describe('seedance2 entity 测试', function () {

  const authHeader = { 'Cookie': `access_token_cookie=${JWT}` };
  const projectId = 'c340051a-45a6-4af1-a750-efefe639c75b';
  let subprojectId = null;
  let classificationId = null;
  let entityId = null;

  before(async function () {
    const subReq = await request.post(`${URL}/api/seedance2/subproject`)
      .set(authHeader)
      .send({
        name: 'test_subproject_entity',
        project_id: projectId,
      });
    subprojectId = subReq.body.id;

    const clsReq = await request.post(`${URL}/api/seedance2/subproject/${subprojectId}/classification`)
      .set(authHeader)
      .send({
        name: 'sc001',
        subproject_id: subprojectId,
      });
    classificationId = clsReq.body.id;
  });

  after(async function () {
    if (subprojectId) {
      await request.delete(`${URL}/api/seedance2/subproject/${subprojectId}`).set(authHeader);
    }
  });

  it('POST /api/seedance2/subproject/{subproject_id}/classification/{classification_id}/entity — 创建实体', async function () {
    expect(classificationId).to.not.be.null;
    const req = await request.post(`${URL}/api/seedance2/subproject/${subprojectId}/classification/${classificationId}/entity`)
      .set(authHeader)
      .send({
        name: 'test_entity',
        project_uuid_id: projectId,
        ai_generate_classification_id: classificationId,
      });
    expect(req.status).to.equal(201);
    entityId = req.body.id;
    console.log('POST entity 返回值:', JSON.stringify(req.body, null, 2));
  });

  it('POST /api/seedance2/subproject/{subproject_id}/entity/{entity_id}/preview — 上传实体预览图', async function () {
    expect(entityId).to.not.be.null;
    // 最小 1x1 红色 PNG
    const pngBuffer = Buffer.from('iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mP8/5+hHgAHggJ/PchI7wAAAABJRU5ErkJggg==', 'base64');
    const req = await request.post(`${URL}/api/seedance2/subproject/${subprojectId}/entity/${entityId}/preview`)
      .set(authHeader)
      .attach('file', pngBuffer, 'test_preview.png');
    expect(req.status).to.equal(200);
    console.log('POST entity preview 返回值:', JSON.stringify(req.body, null, 2));
  });

  it('GET /api/seedance2/subproject/{subproject_id}/classification/{classification_id}/entity — 获取实体列表', async function () {
    expect(classificationId).to.not.be.null;
    const req = await request.get(`${URL}/api/seedance2/subproject/${subprojectId}/classification/${classificationId}/entity`)
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

  it('DELETE /api/seedance2/subproject/{subproject_id}/entity/{entity_id} — 删除实体', async function () {
    expect(entityId).to.not.be.null;
    const req = await request.delete(`${URL}/api/seedance2/subproject/${subprojectId}/entity/${entityId}`)
      .set(authHeader);
    expect(req.status).to.equal(200);
    console.log('DELETE entity 返回值:', JSON.stringify(req.body, null, 2));
  });

});


