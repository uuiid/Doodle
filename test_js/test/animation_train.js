import { test } from '@playwright/test';
import { expect } from 'chai';

import fs from 'node:fs';
import path from 'node:path';

import { URL as BASE_URL, BUILD_DIR } from './config.js';
import { readJson } from './helpers.js';

const URL = BASE_URL.replace(':50025', ':50024');

const MODEL_PATH = process.env.ANIMATION_MODEL_PATH || 'D:\\ai_mod\\onnx-models--nvidia--Kimodo-SOMA-RP-v1.1';
const SEND_DAV_PATH = path.join(BUILD_DIR, 'send_dav.json');
const SEND_DAV_PATH_2 = path.join(BUILD_DIR, 'send_dav_2.json');
const RES_DAV_PATH = path.join(BUILD_DIR, 'res_dav.json');
const RES_DAV_PATH_2 = path.join(BUILD_DIR, 'res_dav_2.json');
const RES_SETTINGS_PATH = path.join(BUILD_DIR, 'res_dav_settings.json');

test.describe('ai animation train 测试', () => {
  test.describe.configure({ timeout: 1200000 });

  let sendDavData = null;
  let sendDavData2 = null;

  test.beforeAll(() => {
    sendDavData = fs.existsSync(SEND_DAV_PATH) ? JSON.parse(fs.readFileSync(SEND_DAV_PATH, 'utf-8')) : null;
    sendDavData2 = fs.existsSync(SEND_DAV_PATH_2) ? JSON.parse(fs.readFileSync(SEND_DAV_PATH_2, 'utf-8')) : null;
  });

  test('POST /api/doodle/ai/animation/train/settings — 加载 Kimodo 模型', async ({ request }) => {
    const req = await readJson(
      await request.post(`${URL}/api/doodle/ai/animation/train/settings`, {
        data: { model_path: MODEL_PATH },
      })
    );
    expect(req.status).to.equal(201);
    expect(req.body).to.have.property('model_path', MODEL_PATH);
    expect(req.body).to.have.property('skeleton');
    console.log('POST settings 返回模型路径:', req.body.model_path);
    fs.writeFileSync(RES_SETTINGS_PATH, JSON.stringify(req.body, null, 2));
  });

  test('POST /api/doodle/ai/animation/train — 生成动画(重定向)', async ({ request }) => {
    test.skip(!sendDavData, 'send_dav.json 不存在');
    const req = await readJson(
      await request.post(`${URL}/api/doodle/ai/animation/train`, {
        data: sendDavData,
      })
    );
    expect(req.status).to.equal(201);
    expect(req.body).to.have.property('local_rot_mats');
    expect(req.body).to.have.property('global_rot_mats');
    expect(req.body).to.have.property('root_positions');
    expect(req.body).to.have.property('smooth_root_pos');
    expect(req.body).to.have.property('global_root_heading');
    console.log('POST train 返回值字段:', Object.keys(req.body));
    fs.writeFileSync(RES_DAV_PATH, JSON.stringify(req.body, null, 2));
  });
  test('POST /api/doodle/ai/animation/train — 生成动画', async ({ request }) => {
    test.skip(!sendDavData2, 'send_dav_2.json 不存在');
    const req = await readJson(
      await request.post(`${URL}/api/doodle/ai/animation/train`, {
        data: sendDavData2,
      })
    );
    expect(req.status).to.equal(201);
    expect(req.body).to.have.property('local_rot_mats');
    expect(req.body).to.have.property('global_rot_mats');
    expect(req.body).to.have.property('root_positions');
    expect(req.body).to.have.property('smooth_root_pos');
    expect(req.body).to.have.property('global_root_heading');
    console.log('POST train 返回值字段:', Object.keys(req.body));
    fs.writeFileSync(RES_DAV_PATH_2, JSON.stringify(req.body, null, 2));
  });
});
