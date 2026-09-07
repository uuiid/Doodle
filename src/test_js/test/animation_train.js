import request from 'superagent';
import { expect } from 'chai';

import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

import { URL } from './config.js';

const MODEL_PATH = 'D:\\ai_mod\\onnx-models--nvidia--Kimodo-SOMA-RP-v1.1';
const SEND_DAV_PATH = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '../../../build/send_dav.json');
const RES_DAV_PATH = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '../../../build/res_dav.json');

describe('ai animation train 测试', function () {
  this.timeout(120000);

  let sendDavData = null;

  before(function () {
    sendDavData = JSON.parse(fs.readFileSync(SEND_DAV_PATH, 'utf-8'));
  });

  it('POST /api/doodle/ai/animation/train/settings — 加载 Kimodo 模型', async function () {
    const req = await request.post(`${URL}/api/doodle/ai/animation/train/settings`)
      .send({ model_path: MODEL_PATH });
    expect(req.status).to.equal(200);
    expect(req.body).to.have.property('model_path', MODEL_PATH);
    expect(req.body).to.have.property('skeleton');
    console.log('POST settings 返回模型路径:', req.body.model_path);
  });

  it('POST /api/doodle/ai/animation/train — 生成动画', async function () {
    expect(sendDavData).to.not.be.null;
    const req = await request.post(`${URL}/api/doodle/ai/animation/train`)
      .send(sendDavData);
    expect(req.status).to.equal(200);
    expect(req.body).to.have.property('local_rot_mats');
    expect(req.body).to.have.property('global_rot_mats');
    expect(req.body).to.have.property('root_positions');
    expect(req.body).to.have.property('smooth_root_pos');
    expect(req.body).to.have.property('global_root_heading');
    console.log('POST train 返回值字段:', Object.keys(req.body));
    fs.writeFileSync(RES_DAV_PATH, JSON.stringify(req.body, null, 2));
  });
});
