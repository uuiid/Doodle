// const superagent = require('superagent');
import request from 'superagent';
import { expect } from 'chai';
import { v4 as uuidv4 } from 'uuid';

import { io } from 'socket.io-client';
// globalThis.chai = chai;

// const fs = require('node:fs');
import { URL } from './config.js';
import jwt_encode from 'jwt-encode';

describe('doodle user测试', function() {
  const l_jwt = jwt_encode({
    'sub': '69a8d093-dcab-4890-8f9d-c51ef065d03b',
  }, 'secret');
  const l_project_id = 'eb2b5cde-c1cf-47a0-aba4-e196b6f774dd';
  it('配置', async function() {
    const req = await request.get(`${URL}/api/config`);
    expect(req.status).to.equal(200);
  });

  it('登录', async function() {
    const req = await request.post(`${URL}/api/auth/login`).send({
      email: 'test_mod@qq.com',
      password: 'test_mod',
    });
    expect(req.status).to.equal(200);
  });


  it('测试鉴权', async function() {
    try {
      const req = await request.get(`${URL}/api/data/user/context`);
    } catch (e) {
      expect(e.status).to.equal(401);
    }
  });
  it('测试授权', async function() {
    const req = await request.get(`${URL}/api/auth/authenticated`).set(
      'Cookie', `access_token_cookie=${l_jwt}`,
    );
    expect(req.status).to.equal(200);

  });
  it('测试登录用户的上下文', async function() {
    const req = await request.get(`${URL}/api/data/user/context`).set(
      'Cookie', `access_token_cookie=${l_jwt}`,
    );
    expect(req.status).to.equal(200);
    // console.log(req.body);
    expect(req.body).to.have.keys(
      'asset_types',
      'custom_actions',
      'departments',
      'dingding_companys',
      'notification_count',
      'persons',
      'preview_background_files',
      'project_status',
      'projects',
      'search_filter_groups',
      'search_filters',
      'status_automations',
      'studios',
      'task_status',
      'task_types',
      'user_limit',
    );
  });
  it('组织', async function() {
    const req = await request.get(`${URL}/api/data/organisations`).set(
      'Cookie', `access_token_cookie=${l_jwt}`,
    );
    expect(req.status).to.equal(200);
  });

  it('获取资产上下文', async function() {
    const req = await request.get(`${URL}/api/data/assets/with-tasks?project_id=${l_project_id}`).set(
      'Cookie', `access_token_cookie=${l_jwt}`,
    );
    expect(req.status).to.equal(200);
    console.log(req.body);
  });
  it('查询 todo', async function() {
    const req = await request.get(`${URL}/api/data/user/tasks`).set(
      'Cookie', `access_token_cookie=${l_jwt}`,
    );
    expect(req.status).to.equal(200);
  });

});