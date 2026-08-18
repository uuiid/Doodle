// const superagent = require('superagent');
import request from 'superagent';
import { expect } from 'chai';

// const fs = require('node:fs');
import { URL, JWT } from './config.js';

describe('seedance2 subproject 测试', function () {

  const authHeader = { 'Cookie': `access_token_cookie=${JWT}` };

  it('GET /api/seedance2/subproject — 获取子项目列表', async function () {
    const req = await request.get(`${URL}/api/seedance2/subproject`).set(authHeader);
    expect(req.status).to.equal(200);
  });

  it('POST /api/seedance2/subproject — 创建子项目', async function () {
    const req = await request.post(`${URL}/api/seedance2/subproject`)
      .set(authHeader)
      .send({
        name: 'test_subproject',
      });
    expect(req.status).to.equal(201);
  });

});


