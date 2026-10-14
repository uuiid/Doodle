import { test } from '@playwright/test';
import { expect } from 'chai';
import fs from 'node:fs';

import { URL, JWT, PROJECT_ID, authHeader, filePart } from './config.js';
import { readJson, expectError, waitFor } from './helpers.js';

test.describe('seedance2 测试', () => {
  let subprojectId = null;

  test('POST /api/seedance2/subproject — 创建子项目', async ({ request }) => {
    const res = await request.post(`${URL}/api/seedance2/subproject`, {
      headers: authHeader,
      data: {
        name: 'test_subproject',
        project_id: PROJECT_ID,
      },
    });
    const req = await readJson(res);
    expect(req.status).to.equal(201);
    subprojectId = req.body.id;
    console.log('POST 返回值:', JSON.stringify(req.body, null, 2));
  });

  test('GET /api/seedance2/subproject/{id} — 获取子项目详情', async ({ request }) => {
    expect(subprojectId).to.not.be.null;
    const res = await request.get(`${URL}/api/seedance2/subproject/${subprojectId}`, { headers: authHeader });
    const req = await readJson(res);
    expect(req.status).to.equal(200);
    console.log('GET instance 返回值:', JSON.stringify(req.body, null, 2));
  });

  test('PUT /api/seedance2/subproject/{id} — 更新子项目', async ({ request }) => {
    expect(subprojectId).to.not.be.null;
    const res = await request.put(`${URL}/api/seedance2/subproject/${subprojectId}`, {
      headers: authHeader,
      data: { name: 'test_subproject_updated' },
    });
    const req = await readJson(res);
    expect(req.status).to.equal(200);
    console.log('PUT 返回值:', JSON.stringify(req.body, null, 2));
  });

  test('GET /api/seedance2/subproject — 获取子项目列表', async ({ request }) => {
    const res = await request.get(`${URL}/api/seedance2/subproject`, { headers: authHeader });
    const req = await readJson(res);
    expect(req.status).to.equal(200);
    console.log('GET 返回值:', JSON.stringify(req.body, null, 2));
  });

  test('DELETE /api/seedance2/subproject/{id} — 删除子项目', async ({ request }) => {
    expect(subprojectId).to.not.be.null;
    const res = await request.delete(`${URL}/api/seedance2/subproject/${subprojectId}`, { headers: authHeader });
    const req = await readJson(res);
    expect(req.status).to.equal(204);
    console.log('DELETE 返回值:', JSON.stringify(req.body, null, 2));
  });
});

test.describe('seedance2 person link 测试', () => {
  const personId = '69a8d093-dcab-4890-8f9d-c51ef065d03b';

  test.describe('生命周期', () => {
    let subprojectId = null;
    let linkId = null;

    test.beforeAll(async ({ request }) => {
      const res = await request.post(`${URL}/api/seedance2/subproject`, {
        headers: authHeader,
        data: {
          name: 'test_subproject_person_link',
          project_id: PROJECT_ID,
        },
      });
      const req = await readJson(res);
      subprojectId = req.body.id;
    });

    test.afterAll(async ({ request }) => {
      if (subprojectId) {
        await request.delete(`${URL}/api/seedance2/subproject/${subprojectId}`, { headers: authHeader });
      }
    });

    test('POST /api/seedance2/subproject/{subproject_id}/person — 添加参与人员', async ({ request }) => {
      expect(subprojectId).to.not.be.null;
      const res = await request.post(`${URL}/api/seedance2/subproject/${subprojectId}/person`, {
        headers: authHeader,
        data: {
          subproject_id: subprojectId,
          person_id: personId,
        },
      });
      const req = await readJson(res);
      expect(req.status).to.equal(201);
      linkId = req.body.id;
      console.log('POST person link 返回值:', JSON.stringify(req.body, null, 2));
    });

    test('DELETE /api/seedance2/subproject/{subproject_id}/person — 移除参与人员', async ({ request }) => {
      expect(subprojectId).to.not.be.null;
      const res = await request.delete(`${URL}/api/seedance2/subproject/${subprojectId}/person`, {
        headers: authHeader,
        data: { person_id: personId },
      });
      const req = await readJson(res);
      expect(req.status).to.equal(200);
      console.log('DELETE person link 返回值:', JSON.stringify(req.body, null, 2));
    });
  });
});

test.describe('seedance2 episodes 测试', () => {
  let subprojectId = null;
  let episodeId = null;

  test.beforeAll(async ({ request }) => {
    const res = await request.post(`${URL}/api/seedance2/subproject`, {
      headers: authHeader,
      data: {
        name: 'test_subproject_episodes',
        project_id: PROJECT_ID,
      },
    });
    const req = await readJson(res);
    subprojectId = req.body.id;
  });

  test.afterAll(async ({ request }) => {
    if (subprojectId) {
      await request.delete(`${URL}/api/seedance2/subproject/${subprojectId}`, { headers: authHeader });
    }
  });

  test('POST /api/seedance2/subproject/{subproject_id}/episodes — 创建剧集', async ({ request }) => {
    expect(subprojectId).to.not.be.null;
    const res = await request.post(`${URL}/api/seedance2/subproject/${subprojectId}/episodes`, {
      headers: authHeader,
      data: {
        name: 'sc001',
        subproject_id: subprojectId,
        description: '测试剧集',
      },
    });
    const req = await readJson(res);
    expect(req.status).to.equal(201);
    episodeId = req.body.id;
    console.log('POST episodes 返回值:', JSON.stringify(req.body, null, 2));
  });

  test('GET /api/seedance2/subproject/{subproject_id}/episodes — 获取剧集列表', async ({ request }) => {
    expect(subprojectId).to.not.be.null;
    expect(episodeId).to.not.be.null;
    const res = await request.get(`${URL}/api/seedance2/subproject/${subprojectId}/episodes`, {
      headers: authHeader,
    });
    const req = await readJson(res);
    expect(req.status).to.equal(200);
    expect(req.body).to.be.an('array');
    expect(req.body.some((e) => e.id === episodeId)).to.equal(true);
    console.log('GET episodes list 返回值:', JSON.stringify(req.body, null, 2));
  });

  test('GET /api/seedance2/subproject/{subproject_id}/episodes/{episode_id} — 获取剧集详情', async ({ request }) => {
    expect(episodeId).to.not.be.null;
    const res = await request.get(`${URL}/api/seedance2/subproject/${subprojectId}/episodes/${episodeId}`, {
      headers: authHeader,
    });
    const req = await readJson(res);
    expect(req.status).to.equal(200);
    console.log('GET episodes instance 返回值:', JSON.stringify(req.body, null, 2));
  });

  test('PUT /api/seedance2/subproject/{subproject_id}/episodes/{episode_id} — 更新剧集', async ({ request }) => {
    expect(episodeId).to.not.be.null;
    const res = await request.put(`${URL}/api/seedance2/subproject/${subprojectId}/episodes/${episodeId}`, {
      headers: authHeader,
      data: { name: 'sc001_updated', description: '更新后的剧集' },
    });
    const req = await readJson(res);
    expect(req.status).to.equal(200);
    console.log('PUT episodes 返回值:', JSON.stringify(req.body, null, 2));
  });

  test('DELETE /api/seedance2/subproject/{subproject_id}/episodes/{episode_id} — 删除剧集', async ({ request }) => {
    expect(episodeId).to.not.be.null;
    const res = await request.delete(`${URL}/api/seedance2/subproject/${subprojectId}/episodes/${episodeId}`, {
      headers: authHeader,
    });
    const req = await readJson(res);
    expect(req.status).to.equal(200);
    console.log('DELETE episodes 返回值:', JSON.stringify(req.body, null, 2));
  });
});

test.describe('seedance2 entity 测试', () => {
  const projectId = PROJECT_ID;
  let subprojectId = null;
  let episodeId = null;
  let entityId = null;

  test.beforeAll(async ({ request }) => {
    const subRes = await request.post(`${URL}/api/seedance2/subproject`, {
      headers: authHeader,
      data: {
        name: 'test_subproject_entity',
        project_id: projectId,
      },
    });
    subprojectId = (await readJson(subRes)).body.id;

    const clsRes = await request.post(`${URL}/api/seedance2/subproject/${subprojectId}/episodes`, {
      headers: authHeader,
      data: {
        name: 'sc001',
        subproject_id: subprojectId,
      },
    });
    episodeId = (await readJson(clsRes)).body.id;
  });

  test.afterAll(async ({ request }) => {
    if (subprojectId) {
      await request.delete(`${URL}/api/seedance2/subproject/${subprojectId}`, { headers: authHeader });
    }
  });

  test('POST /api/seedance2/subproject/{subproject_id}/entity — 创建实体', async ({ request }) => {
    expect(episodeId).to.not.be.null;
    const res = await request.post(`${URL}/api/seedance2/subproject/${subprojectId}/entity`, {
      headers: authHeader,
      data: {
        name: 'test_entity',
        project_uuid_id: projectId,
        ai_episode_id: episodeId,
        ai_category_id: '01a03b8f-d9f3-71f1-808f-a098a698c889',
      },
    });
    const req = await readJson(res);
    expect(req.status).to.equal(201);
    entityId = req.body.id;
    console.log('POST entity 返回值:', JSON.stringify(req.body, null, 2));
  });

  test('POST .../entity/{entity_id}/reference — 上传实体参考文件', async ({ request }) => {
    test.setTimeout(330000);
    expect(entityId).to.not.be.null;
    const mp4Path = process.env.SEEDANCE2_REFERENCE_FILE || 'D:\\无标题(1).jpg';
    test.skip(!fs.existsSync(mp4Path), `参考文件不存在: ${mp4Path}`);
    const res = await request.post(
      `${URL}/api/seedance2/subproject/${subprojectId}/entity/${entityId}/reference`,
      {
        headers: authHeader,
        multipart: { file: filePart(mp4Path, '无标题(1).jpg') },
        timeout: 300000,
      }
    );
    const req = await readJson(res);
    expect(req.status).to.equal(201);
    console.log('POST entity reference 返回值:', JSON.stringify(req.body, null, 2));
  });

  test('GET .../episodes/{episode_id}/entity — 获取实体列表', async ({ request }) => {
    expect(episodeId).to.not.be.null;
    const res = await request.get(
      `${URL}/api/seedance2/subproject/${subprojectId}/episodes/${episodeId}/entity`,
      { headers: authHeader }
    );
    const req = await readJson(res);
    expect(req.status).to.equal(200);
    console.log('GET entity list 返回值:', JSON.stringify(req.body, null, 2));
  });

  test('GET .../entity/{entity_id} — 获取实体详情', async ({ request }) => {
    expect(entityId).to.not.be.null;
    const res = await request.get(`${URL}/api/seedance2/subproject/${subprojectId}/entity/${entityId}`, {
      headers: authHeader,
    });
    const req = await readJson(res);
    expect(req.status).to.equal(200);
    console.log('GET entity instance 返回值:', JSON.stringify(req.body, null, 2));
  });

  test('PUT .../entity/{entity_id} — 更新实体', async ({ request }) => {
    expect(entityId).to.not.be.null;
    const res = await request.put(`${URL}/api/seedance2/subproject/${subprojectId}/entity/${entityId}`, {
      headers: authHeader,
      data: { name: 'test_entity_updated' },
    });
    const req = await readJson(res);
    expect(req.status).to.equal(200);
    console.log('PUT entity 返回值:', JSON.stringify(req.body, null, 2));
  });

  test('POST .../entity/{entity_id}/depth — 上传视频进行深度估计', async ({ request }) => {
    test.setTimeout(330000);
    expect(entityId).to.not.be.null;
    const mp4Path = process.env.SEEDANCE2_DEPTH_FILE || 'D:\\test_files\\test_depth.mp4';
    test.skip(!fs.existsSync(mp4Path), `深度估计视频不存在: ${mp4Path}`);
    const res = await request.post(
      `${URL}/api/seedance2/subproject/${subprojectId}/entity/${entityId}/depth`,
      {
        headers: authHeader,
        multipart: { file: filePart(mp4Path, 'test_depth.mp4') },
        timeout: 300000,
      }
    );
    const req = await readJson(res);
    expect(req.status).to.equal(201);
    console.log('POST depth 返回值:', JSON.stringify(req.body, null, 2));
  });

  test('DELETE .../entity/{entity_id} — 删除实体', async ({ request }) => {
    expect(entityId).to.not.be.null;
    const res = await request.delete(`${URL}/api/seedance2/subproject/${subprojectId}/entity/${entityId}`, {
      headers: authHeader,
    });
    const req = await readJson(res);
    expect(req.status).to.equal(200);
    console.log('DELETE entity 返回值:', JSON.stringify(req.body, null, 2));
  });
});

test.describe('seedance2 task', () => {
  const projectId = PROJECT_ID;
  const model = 'doubao-seedance-1-0-pro-250528';
  const resolution = '1080p';
  let subprojectId = null;
  let episodeId = null;
  let entityId = null;
  let taskId = null;

  test.beforeAll(async ({ request }) => {
    const subRes = await request.post(`${URL}/api/seedance2/subproject`, {
      headers: authHeader,
      data: {
        name: 'test_subproject_task',
        project_id: projectId,
      },
    });
    subprojectId = (await readJson(subRes)).body.id;

    const clsRes = await request.post(`${URL}/api/seedance2/subproject/${subprojectId}/episodes`, {
      headers: authHeader,
      data: {
        name: 'sc001',
        subproject_id: subprojectId,
      },
    });
    episodeId = (await readJson(clsRes)).body.id;

    await request.post(
      `${URL}/api/seedance2/subproject/${subprojectId}/episodes/${episodeId}/model-resolution-limit`,
      {
        headers: authHeader,
        data: { model_name: model, resolution },
      }
    );

    const catRes = await request.post(`${URL}/api/seedance2/subproject/${subprojectId}/category`, {
      headers: authHeader,
      data: { name: 'cat_task', type: 'shot', description: '任务测试类别' },
    });
    const categoryId = (await readJson(catRes)).body.id;

    const entRes = await request.post(`${URL}/api/seedance2/subproject/${subprojectId}/entity`, {
      headers: authHeader,
      data: {
        name: 'test_entity',
        project_uuid_id: projectId,
        ai_episode_id: episodeId,
        ai_category_id: categoryId,
      },
    });
    entityId = (await readJson(entRes)).body.id;
  });

  test.afterAll(async ({ request }) => {
    if (subprojectId) {
      await request.delete(`${URL}/api/seedance2/subproject/${subprojectId}`, { headers: authHeader });
    }
  });

  test('POST .../entity/{entity_id}/task — 创建任务', async ({ request }) => {
    expect(entityId).to.not.be.null;
    const res = await request.post(
      `${URL}/api/seedance2/subproject/${subprojectId}/entity/${entityId}/task`,
      {
        headers: authHeader,
        data: {
          backend: 'seedance2',
          data_request: {
            model,
            resolution,
            content: [
              { type: 'text', text: 'A test prompt for seedance2 task' },
              {
                type: 'image_url',
                image_url: { url: '/api/seedance2/pictures/01a01da2-876d-7208-803f-e1b7938d15d9.png' },
              },
            ],
          },
          ai_studio_id: '019e2075-6e42-718e-809e-33a92410c682',
          project_uuid_id: projectId,
          type: 'video',
          ai_generate_entity_id: entityId,
        },
      }
    );
    const req = await readJson(res);
    expect(req.status).to.equal(201);
    taskId = req.body.id;
    console.log('POST task 返回值:', JSON.stringify(req.body, null, 2));
  });

  test('GET .../entity/{entity_id}/task — 获取任务列表', async ({ request }) => {
    expect(entityId).to.not.be.null;
    const res = await request.get(
      `${URL}/api/seedance2/subproject/${subprojectId}/entity/${entityId}/task`,
      { headers: authHeader }
    );
    const req = await readJson(res);
    expect(req.status).to.equal(200);
    console.log('GET task list 返回值:', JSON.stringify(req.body, null, 2));
  });

  test('GET .../task/{id} — 获取任务详情', async ({ request }) => {
    expect(taskId).to.not.be.null;
    const res = await request.get(`${URL}/api/seedance2/subproject/${subprojectId}/task/${taskId}`, {
      headers: authHeader,
    });
    const req = await readJson(res);
    expect(req.status).to.equal(200);
    console.log('GET task instance 返回值:', JSON.stringify(req.body, null, 2));
  });

  test('PUT .../task/{id} — 取消任务', async ({ request }) => {
    expect(taskId).to.not.be.null;
    const res = await request.put(`${URL}/api/seedance2/subproject/${subprojectId}/task/${taskId}`, {
      headers: authHeader,
    });
    const req = await readJson(res);
    expect(req.status).to.equal(200);
    console.log('PUT task 返回值:', JSON.stringify(req.body, null, 2));
  });

  test('DELETE .../task/{id} — 归档任务', async ({ request }) => {
    expect(taskId).to.not.be.null;
    const res = await request.delete(`${URL}/api/seedance2/subproject/${subprojectId}/task/${taskId}`, {
      headers: authHeader,
    });
    const req = await readJson(res);
    expect(req.status).to.equal(200);
    console.log('DELETE task 返回值:', JSON.stringify(req.body, null, 2));
  });
});

test.describe('seedance2 ai category 测试', () => {
  const projectId = PROJECT_ID;
  let subprojectId = null;
  let categoryId = null;
  let entityId = null;

  test.beforeAll(async ({ request }) => {
    const subRes = await request.post(`${URL}/api/seedance2/subproject`, {
      headers: authHeader,
      data: {
        name: 'test_subproject_ai_category',
        project_id: projectId,
      },
    });
    subprojectId = (await readJson(subRes)).body.id;
  });

  test.afterAll(async ({ request }) => {
    if (subprojectId) {
      await request.delete(`${URL}/api/seedance2/subproject/${subprojectId}`, { headers: authHeader });
    }
  });

  test('POST .../category — 创建类别', async ({ request }) => {
    expect(subprojectId).to.not.be.null;
    const res = await request.post(`${URL}/api/seedance2/subproject/${subprojectId}/category`, {
      headers: authHeader,
      data: {
        name: 'cat001',
        type: 'assets',
        description: '测试类别',
      },
    });
    const req = await readJson(res);
    expect(req.status).to.equal(201);
    categoryId = req.body.id;
    console.log('POST category 返回值:', JSON.stringify(req.body, null, 2));
  });

  test('GET .../category — 获取类别列表', async ({ request }) => {
    expect(subprojectId).to.not.be.null;
    const res = await request.get(`${URL}/api/seedance2/subproject/${subprojectId}/category`, {
      headers: authHeader,
    });
    const req = await readJson(res);
    expect(req.status).to.equal(200);
    console.log('GET category list 返回值:', JSON.stringify(req.body, null, 2));
  });

  test('GET .../category/{category_id} — 获取类别详情', async ({ request }) => {
    expect(categoryId).to.not.be.null;
    const res = await request.get(`${URL}/api/seedance2/subproject/${subprojectId}/category/${categoryId}`, {
      headers: authHeader,
    });
    const req = await readJson(res);
    expect(req.status).to.equal(200);
    expect(req.body.type).to.equal('assets');
    console.log('GET category instance 返回值:', JSON.stringify(req.body, null, 2));
  });

  test('PUT .../category/{category_id} — 更新类别', async ({ request }) => {
    expect(categoryId).to.not.be.null;
    const res = await request.put(`${URL}/api/seedance2/subproject/${subprojectId}/category/${categoryId}`, {
      headers: authHeader,
      data: { name: 'cat001_updated', type: 'assets', description: '更新后的类别' },
    });
    const req = await readJson(res);
    expect(req.status).to.equal(200);
    console.log('PUT category 返回值:', JSON.stringify(req.body, null, 2));
  });

  test('POST .../entity — 创建关联类别的实体', async ({ request }) => {
    expect(categoryId).to.not.be.null;
    const res = await request.post(`${URL}/api/seedance2/subproject/${subprojectId}/entity`, {
      headers: authHeader,
      data: {
        name: 'test_entity_ai_category',
        project_uuid_id: projectId,
        ai_category_id: categoryId,
      },
    });
    const req = await readJson(res);
    expect(req.status).to.equal(201);
    entityId = req.body.id;
    console.log('POST entity 返回值:', JSON.stringify(req.body, null, 2));
  });

  test('GET .../category/{category_id}/entity — 获取类别下实体列表', async ({ request }) => {
    expect(categoryId).to.not.be.null;
    const res = await request.get(
      `${URL}/api/seedance2/subproject/${subprojectId}/category/${categoryId}/entity`,
      { headers: authHeader }
    );
    const req = await readJson(res);
    expect(req.status).to.equal(200);
    console.log('GET category entity 返回值:', JSON.stringify(req.body, null, 2));
  });

  test('DELETE .../category/{category_id} — 删除类别', async ({ request }) => {
    expect(categoryId).to.not.be.null;
    const res = await request.delete(`${URL}/api/seedance2/subproject/${subprojectId}/category/${categoryId}`, {
      headers: authHeader,
    });
    const req = await readJson(res);
    expect(req.status).to.equal(200);
    console.log('DELETE category 返回值:', JSON.stringify(req.body, null, 2));
  });

  test('DELETE .../entity/{entity_id} — 清理关联实体', async ({ request }) => {
    expect(entityId).to.not.be.null;
    const res = await request.delete(`${URL}/api/seedance2/subproject/${subprojectId}/entity/${entityId}`, {
      headers: authHeader,
    });
    const req = await readJson(res);
    expect(req.status).to.equal(200);
    console.log('DELETE entity 返回值:', JSON.stringify(req.body, null, 2));
  });
});

test.describe('seedance2 task — transfer_station 后端', () => {
  const projectId = PROJECT_ID;
  const model = 'nano-banana-2';
  const resolution = '1K';
  const unpricedModel = 'nano-banana-pro-vt';
  const unauthorizedModel = 'gpt-image-2';

  let subprojectId = null;
  let episodeId = null;
  let entityId = null;
  let taskId = null;

  async function waitForTaskStatus(request, id, statuses, timeoutMs = 40000) {
    return waitFor(
      async () => {
        const res = await request.get(`${URL}/api/seedance2/subproject/${subprojectId}/task/${id}`, {
          headers: authHeader,
        });
        return (await readJson(res)).body;
      },
      (last) => statuses.includes(last?.status),
      timeoutMs
    );
  }

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

  function postTask(request, body) {
    return request
      .post(`${URL}/api/seedance2/subproject/${subprojectId}/entity/${entityId}/task`, {
        headers: authHeader,
        data: body,
      })
      .then(readJson);
  }

  test.beforeAll(async ({ request }) => {
    const subRes = await request.post(`${URL}/api/seedance2/subproject`, {
      headers: authHeader,
      data: {
        name: 'test_subproject_transfer_station',
        project_id: projectId,
      },
    });
    subprojectId = (await readJson(subRes)).body.id;

    const clsRes = await request.post(`${URL}/api/seedance2/subproject/${subprojectId}/episodes`, {
      headers: authHeader,
      data: {
        name: 'sc001',
        subproject_id: subprojectId,
      },
    });
    episodeId = (await readJson(clsRes)).body.id;

    await request.post(
      `${URL}/api/seedance2/subproject/${subprojectId}/episodes/${episodeId}/model-resolution-limit`,
      {
        headers: authHeader,
        data: { model_name: model, resolution },
      }
    );

    const catRes = await request.post(`${URL}/api/seedance2/subproject/${subprojectId}/category`, {
      headers: authHeader,
      data: { name: 'cat_transfer_station', type: 'shot', description: '中转站测试类别' },
    });
    const categoryId = (await readJson(catRes)).body.id;

    const entRes = await request.post(`${URL}/api/seedance2/subproject/${subprojectId}/entity`, {
      headers: authHeader,
      data: {
        name: 'test_entity_transfer_station',
        project_uuid_id: projectId,
        ai_episode_id: episodeId,
        ai_category_id: categoryId,
      },
    });
    entityId = (await readJson(entRes)).body.id;
  });

  test.afterAll(async ({ request }) => {
    if (subprojectId) {
      await request.delete(`${URL}/api/seedance2/subproject/${subprojectId}`, { headers: authHeader });
    }
  });

  test('POST .../entity/{entity_id}/task — 使用 transfer_station 后端提交图片任务', async ({ request }) => {
    expect(entityId).to.not.be.null;
    const req = await postTask(request, transferStationBody());
    expect(req.status).to.equal(201);
    taskId = req.body.id;
    expect(taskId).to.be.a('string');
    console.log('POST transfer_station task 返回值:', JSON.stringify(req.body, null, 2));
  });

  test('GET .../task/{id} — backend 与提示词应按 transfer_station 规则落库', async ({ request }) => {
    expect(taskId).to.not.be.null;
    const res = await request.get(`${URL}/api/seedance2/subproject/${subprojectId}/task/${taskId}`, {
      headers: authHeader,
    });
    const req = await readJson(res);
    expect(req.status).to.equal(200);
    expect(req.body.backend).to.equal('transfer_station');
    expect(req.body.type).to.equal('picture');
    expect(req.body.text_prompt).to.equal('一只边牧与古牧正在抖音直播间直播带货截图');
    console.log('GET transfer_station task 返回值:', JSON.stringify(req.body, null, 2));
  });

  test('POST .../task — 缺少模型名称应返回 400', async ({ request }) => {
    const req = await postTask(request, transferStationBody({ data_request: { prompt: '没有模型名称的请求' } }));
    expect(req.status).to.equal(400);
  });

  test('POST .../task — 缺少分辨率应返回 400', async ({ request }) => {
    const req = await postTask(request, transferStationBody({ data_request: { model, prompt: '没有分辨率的请求' } }));
    expect(req.status).to.equal(400);
  });

  test('POST .../task — 未授权的模型或分辨率应返回 401', async ({ request }) => {
    const req = await postTask(
      request,
      transferStationBody({
        data_request: { model: unauthorizedModel, prompt: '未授权模型', aspectRatio: '2048x2048' },
      })
    );
    expect(req.status).to.equal(401);
  });

  test('未登记定价的模型提交后任务应失败且不扣费', async ({ request }) => {
    test.setTimeout(90000);
    await request.post(
      `${URL}/api/seedance2/subproject/${subprojectId}/episodes/${episodeId}/model-resolution-limit`,
      {
        headers: authHeader,
        data: { model_name: unpricedModel, resolution },
      }
    );

    const createReq = await postTask(
      request,
      transferStationBody({
        data_request: { model: unpricedModel, prompt: '未登记定价的模型', imageSize: resolution },
      })
    );
    expect(createReq.status).to.equal(201);

    const task = await waitForTaskStatus(request, createReq.body.id, ['failed'], 80000);
    expect(task.status).to.equal('failed');
    expect(JSON.stringify(task.data_response)).to.include('未登记定价');
    expect(task.completion_tokens).to.equal(0);
    console.log('未登记定价任务返回值:', JSON.stringify(task, null, 2));
  });
});

test.describe('seedance2 task — apipy_image 后端', () => {
  const projectId = PROJECT_ID;
  const model = 'gpt-image-2.5-vip';
  const resolution = '1024x1024';
  const unauthorizedModel = 'gpt-image-9-not-exist';
  const referenceSourcePath = process.env.APIPY_REFERENCE_FILE || 'D:\\tets.png';
  const referenceSourceName = 'tets.png';

  let subprojectId = null;
  let episodeId = null;
  let entityId = null;
  let categoryId = null;
  let taskId = null;
  let referenceUrl = null;

  async function waitForTaskStatus(request, id, statuses, timeoutMs = 40000) {
    return waitFor(
      async () => {
        const res = await request.get(`${URL}/api/seedance2/subproject/${subprojectId}/task/${id}`, {
          headers: authHeader,
        });
        return (await readJson(res)).body;
      },
      (last) => statuses.includes(last?.status),
      timeoutMs
    );
  }

  function apipyBody(overrides = {}) {
    return {
      backend: 'apipy_image',
      data_request: {
        model,
        prompt: '把参考图里的边牧换成古牧, 保持构图与直播间背景不变',
        imageSize: resolution,
        aspectRatio: resolution,
        images: referenceUrl ? [referenceUrl] : [],
      },
      ai_studio_id: '019e2075-6e42-718e-809e-33a92410c682',
      project_uuid_id: projectId,
      type: 'picture',
      ai_generate_entity_id: entityId,
      ...overrides,
    };
  }

  function postTask(request, body) {
    return request
      .post(`${URL}/api/seedance2/subproject/${subprojectId}/entity/${entityId}/task`, {
        headers: authHeader,
        data: body,
      })
      .then(readJson);
  }

  test.beforeAll(async ({ request }) => {
    test.setTimeout(360000);
    const subRes = await request.post(`${URL}/api/seedance2/subproject`, {
      headers: authHeader,
      data: {
        name: 'test_subproject_apipy_image',
        project_id: projectId,
      },
    });
    subprojectId = (await readJson(subRes)).body.id;

    const clsRes = await request.post(`${URL}/api/seedance2/subproject/${subprojectId}/episodes`, {
      headers: authHeader,
      data: {
        name: 'sc001',
        subproject_id: subprojectId,
      },
    });
    episodeId = (await readJson(clsRes)).body.id;

    await request.post(
      `${URL}/api/seedance2/subproject/${subprojectId}/episodes/${episodeId}/model-resolution-limit`,
      {
        headers: authHeader,
        data: { model_name: model, resolution },
      }
    );

    const catRes = await request.post(`${URL}/api/seedance2/subproject/${subprojectId}/category`, {
      headers: authHeader,
      data: { name: 'cat_apipy_image', type: 'shot', description: 'API易测试类别' },
    });
    categoryId = (await readJson(catRes)).body.id;

    const entRes = await request.post(`${URL}/api/seedance2/subproject/${subprojectId}/entity`, {
      headers: authHeader,
      data: {
        name: 'test_entity_apipy_image',
        project_uuid_id: projectId,
        ai_episode_id: episodeId,
        ai_category_id: categoryId,
      },
    });
    entityId = (await readJson(entRes)).body.id;

    if (!fs.existsSync(referenceSourcePath)) {
      console.log(`参考图源文件不存在，跳过上传: ${referenceSourcePath}`);
      return;
    }

    const refRes = await request.post(
      `${URL}/api/seedance2/subproject/${subprojectId}/entity/${entityId}/reference`,
      {
        headers: authHeader,
        multipart: { file: filePart(referenceSourcePath, referenceSourceName) },
        timeout: 300000,
      }
    );
    const refReq = await readJson(refRes);
    expect(refReq.status).to.equal(201);
    const preview = refReq.body.preview;
    referenceUrl = `/api/seedance2/pictures/${preview.id}${preview.extension}`;
    console.log('参考图 URL:', referenceUrl);
  });

  test.afterAll(async ({ request }) => {
    if (subprojectId) {
      await request.delete(`${URL}/api/seedance2/subproject/${subprojectId}`, { headers: authHeader });
    }
  });

  test('POST .../entity/{entity_id}/task — 使用 apipy_image 后端提交图片任务', async ({ request }) => {
    expect(entityId).to.not.be.null;
    const req = await postTask(request, apipyBody());
    expect(req.status).to.equal(201);
    taskId = req.body.id;
    expect(taskId).to.be.a('string');
    console.log('POST apipy_image task 返回值:', JSON.stringify(req.body, null, 2));
  });

  test('GET .../task/{id} — backend 与提示词应按 apipy_image 规则落库', async ({ request }) => {
    expect(taskId).to.not.be.null;
    const res = await request.get(`${URL}/api/seedance2/subproject/${subprojectId}/task/${taskId}`, {
      headers: authHeader,
    });
    const req = await readJson(res);
    expect(req.status).to.equal(200);
    expect(req.body.backend).to.equal('apipy_image');
    expect(req.body.type).to.equal('picture');
    expect(req.body.text_prompt).to.equal('把参考图里的边牧换成古牧, 保持构图与直播间背景不变');
    expect(req.body.data_request.images).to.deep.equal([referenceUrl]);
    console.log('GET apipy_image task 返回值:', JSON.stringify(req.body, null, 2));
  });

  test('使用参考图片生成图片 — apipy_image 后端应带着参考图跑到终态', async ({ request }) => {
    test.setTimeout(600000);
    expect(referenceUrl).to.not.be.null;

    const createReq = await postTask(
      request,
      apipyBody({
        data_request: {
          model,
          prompt: '以参考图中的边牧为原型, 生成一张古牧在抖音直播间直播带货的截图',
          imageSize: resolution,
          aspectRatio: resolution,
          images: [referenceUrl],
        },
      })
    );
    expect(createReq.status).to.equal(201);

    const task = await waitForTaskStatus(request, createReq.body.id, ['succeeded', 'failed'], 540000);
    console.log('使用参考图生成图片的任务返回值:', JSON.stringify(task, null, 2));

    expect(['succeeded', 'failed']).to.include(task.status);
    if (task.status === 'failed') {
      const detail = JSON.stringify(task.data_response);
      expect(detail).to.not.include('参考图 URL 必须指向本站图片路由');
      expect(detail).to.not.include('参考图扩展名必须是');
      expect(detail).to.not.include('参考图最多');
      console.log('使用参考图生成图片失败原因:', detail);
    } else {
      expect(task.status).to.equal('succeeded');
      expect(task.preview_file).to.not.equal('00000000-0000-0000-0000-000000000000');
    }
  });

  test('POST .../task — 缺少模型名称应返回 400', async ({ request }) => {
    const req = await postTask(request, apipyBody({ data_request: { prompt: '没有模型名称的请求' } }));
    expect(req.status).to.equal(400);
  });

  test('POST .../task — 缺少分辨率应返回 400', async ({ request }) => {
    const req = await postTask(request, apipyBody({ data_request: { model, prompt: '没有分辨率的请求' } }));
    expect(req.status).to.equal(400);
  });

  test('POST .../task — 未授权的模型或分辨率应返回 401', async ({ request }) => {
    const req = await postTask(
      request,
      apipyBody({
        data_request: { model: unauthorizedModel, prompt: '未授权模型', aspectRatio: '2048x2048' },
      })
    );
    expect(req.status).to.equal(401);
  });

  test('POST .../task — 参考图 URL 不指向本站图片路由应返回 400', async ({ request }) => {
    const req = await postTask(
      request,
      apipyBody({
        data_request: {
          model,
          prompt: '外部参考图',
          imageSize: resolution,
          aspectRatio: resolution,
          images: ['https://example.com/somewhere/else.png'],
        },
      })
    );
    expect(req.status).to.equal(400);
  });

  test('POST .../task — 参考图超过 8 张应返回 400', async ({ request }) => {
    const req = await postTask(
      request,
      apipyBody({
        data_request: {
          model,
          prompt: '参考图过多',
          imageSize: resolution,
          aspectRatio: resolution,
          images: Array.from({ length: 9 }, () => referenceUrl),
        },
      })
    );
    expect(req.status).to.equal(400);
  });
});
