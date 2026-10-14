import { test } from '@playwright/test';
import { expect } from 'chai';

import { URL, authHeader } from './config.js';
import { readJson } from './helpers.js';

test.describe('library测试', () => {
  let root_uuid, child_uuid, model_node_uuid;

  test('创建树', async ({ request }) => {
    const res = await request.post(`${URL}/api/doodle/model_library/assets_tree`, {
      data: {
        label: '测试类别',
        order: 0,
      },
    });
    const req = await readJson(res);
    expect(req.status).to.equal(201);
    expect(req.body).to.have.keys('id', 'parent_id', 'label', 'order');
    root_uuid = req.body.id;

    const res2 = await request.post(`${URL}/api/doodle/model_library/assets_tree`, {
      data: {
        parent_id: root_uuid,
        label: '测试子类别',
        order: 0,
      },
    });
    const req2 = await readJson(res2);
    expect(req2.status).to.equal(201);
    expect(req2.body).to.have.keys('id', 'parent_id', 'label', 'order');
    child_uuid = req2.body.id;
  });

  test('创建模型', async ({ request }) => {
    const res5 = await request.post(`${URL}/api/doodle/model_library/assets`, {
      data: {
        label: '测试模型',
        parents: [root_uuid, child_uuid],
        path: 'D:\\test\\model\\test.ma',
        notes: '测试模型des',
        active: true,
        has_thumbnail: false,
        extension: 'png',
        tree_node_id: child_uuid,
      },
    });
    const req5 = await readJson(res5);
    expect(req5.status).to.equal(201);
    expect(req5.body).to.have.keys(
      'id',
      'label',
      'parents',
      'path',
      'notes',
      'active',
      'has_thumbnail',
      'extension'
    );
    expect(req5.body.parents).to.be.an('array');
    expect(req5.body.parents).to.have.lengthOf(2);

    model_node_uuid = req5.body.id;
  });

  test('查询树', async ({ request }) => {
    const res7 = await request.get(`${URL}/api/doodle/model_library/assets_tree`);
    const req7 = await readJson(res7);
    expect(req7.status).to.equal(200);
    expect(req7.body).to.be.an('array');
    expect(req7.body[0]).to.have.keys('id', 'parent_id', 'label', 'order');
    expect(req7.body).to.have.lengthOf(3);
  });

  test('查询模型', async ({ request }) => {
    const res8 = await request.get(`${URL}/api/doodle/model_library/assets`);
    const req8 = await readJson(res8);
    expect(req8.status).to.equal(200);
    expect(req8.body).to.be.an('array');
    expect(req8.body[0]).to.have.keys(
      'id',
      'label',
      'parents',
      'path',
      'notes',
      'active',
      'has_thumbnail',
      'extension'
    );
    expect(req8.body).to.have.lengthOf(1);
    expect(req8.body[0].id).to.equal(model_node_uuid);
    expect(req8.body[0].parents).to.be.an('array');
    expect(req8.body[0].parents).to.have.lengthOf(2);
  });

  test('删除树和模型之间的关联', async ({ request }) => {
    const res10 = await request.delete(
      `${URL}/api/doodle/model_library/assets_tree/${root_uuid}/assets/${model_node_uuid}`
    );
    const req10 = await readJson(res10);
    expect(req10.status).to.equal(200);

    const res8 = await request.get(`${URL}/api/doodle/model_library/assets`);
    const req8 = await readJson(res8);
    expect(req8.status).to.equal(200);
    expect(req8.body).to.be.an('array');
    expect(req8.body[0]).to.have.keys(
      'id',
      'label',
      'parents',
      'path',
      'notes',
      'active',
      'has_thumbnail',
      'extension'
    );
    expect(req8.body).to.have.lengthOf(1);
    expect(req8.body[0].id).to.equal(model_node_uuid);
    expect(req8.body[0].parents).to.be.an('array');
    expect(req8.body[0].parents).to.have.lengthOf(1);
  });

  test('添加树和模型之间的关联', async ({ request }) => {
    const res11 = await request.post(
      `${URL}/api/doodle/model_library/assets_tree/${root_uuid}/assets/${model_node_uuid}`
    );
    const req11 = await readJson(res11);
    expect(req11.status).to.equal(200);

    const res8 = await request.get(`${URL}/api/doodle/model_library/assets`);
    const req8 = await readJson(res8);
    expect(req8.status).to.equal(200);
    expect(req8.body).to.be.an('array');
    expect(req8.body[0]).to.have.keys(
      'id',
      'label',
      'parents',
      'path',
      'notes',
      'active',
      'has_thumbnail',
      'extension'
    );
    expect(req8.body).to.have.lengthOf(1);
    expect(req8.body[0].id).to.equal(model_node_uuid);
    expect(req8.body[0].parents).to.be.an('array');
    expect(req8.body[0].parents).to.have.lengthOf(2);
  });

  test('修改模型为未激活', async ({ request }) => {
    if (!model_node_uuid) {
      const res8 = await request.get(`${URL}/api/doodle/model_library/assets`);
      const req8 = await readJson(res8);
      expect(req8.status).to.equal(200);
      expect(req8.body).to.be.an('array');
      expect(req8.body[0]).to.have.keys(
        'id',
        'label',
        'parents',
        'path',
        'notes',
        'active',
        'has_thumbnail',
        'extension'
      );
      expect(req8.body).to.have.lengthOf(1);
      model_node_uuid = req8.body[0].id;
    }
    const res12 = await request.put(`${URL}/api/doodle/model_library/assets/${model_node_uuid}`, {
      data: {
        active: false,
      },
    });
    const req12 = await readJson(res12);
    expect(req12.status).to.equal(200);

    const res8 = await request.get(`${URL}/api/doodle/model_library/assets`);
    const req8 = await readJson(res8);
    expect(req8.status).to.equal(200);
    expect(req8.body).to.be.an('array');
    expect(req8.body[0]).to.have.keys(
      'id',
      'label',
      'parents',
      'path',
      'notes',
      'active',
      'has_thumbnail',
      'extension'
    );
    expect(req8.body[0].parents).to.be.an('array');
    expect(req8.body[0].parents).to.have.lengthOf(2);
    expect(req8.body).to.have.lengthOf(1);
    expect(req8.body[0].id).to.equal(model_node_uuid);
    expect(req8.body[0].active).to.equal(false);
  });

  test('删除模型', async ({ request }) => {
    const res12 = await request.delete(`${URL}/api/doodle/model_library/assets/${model_node_uuid}`, {
      headers: authHeader,
    });
    const req12 = await readJson(res12);
    expect(req12.status).to.equal(200);
    const res8 = await request.get(`${URL}/api/doodle/model_library/assets`);
    const req8 = await readJson(res8);
    expect(req8.status).to.equal(200);
    expect(req8.body).to.be.an('array');
    expect(req8.body).to.have.lengthOf(0);
  });

  test.skip('上下文', async ({ request }) => {
    const res = await request.get(`${URL}/api/doodle/model_library/context`);
    const req = await readJson(res);
    expect(req.status).to.equal(200);
    expect(req.body).to.have.keys('tree_nodes');
    expect(req.body.tree_nodes).to.be.an('array');
    console.log(req.body);
  });
});
