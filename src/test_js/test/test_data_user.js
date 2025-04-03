// const superagent = require('superagent');
import request from 'superagent';
import { expect } from 'chai';
import { v4 as uuidv4 } from 'uuid';

import { io } from 'socket.io-client';
// globalThis.chai = chai;

// const fs = require('node:fs');
import { URL } from './config.js';
import jwt_encode from 'jwt-encode';

describe('doodle user测试', function () {
  const l_jwt = jwt_encode({
    'sub': '69a8d093-dcab-4890-8f9d-c51ef065d03b',
  }, 'secret');
  const l_project_id = 'eb2b5cde-c1cf-47a0-aba4-e196b6f774dd';
  it('配置', async function () {
    const req = await request.get(`${URL}/api/config`);
    expect(req.status).to.equal(200);
  });

  it('登录', async function () {
    const req = await request.post(`${URL}/api/auth/login`).send({
      email: 'test_mod@qq.com',
      password: 'test_mod',
    });
    expect(req.status).to.equal(200);
  });


  it('测试鉴权', async function () {
    try {
      const req = await request.get(`${URL}/api/data/user/context`);
    } catch (e) {
      expect(e.status).to.equal(401);
    }
  });
  it('测试授权', async function () {
    const req = await request.get(`${URL}/api/auth/authenticated`).set(
      'Cookie', `access_token_cookie=${l_jwt}`,
    );
    expect(req.status).to.equal(200);

  });
  it('测试登录用户的上下文', async function () {
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
  it('组织', async function () {
    const req = await request.get(`${URL}/api/data/organisations`).set(
      'Cookie', `access_token_cookie=${l_jwt}`,
    );
    expect(req.status).to.equal(200);
  });

  it('获取资产上下文', async function () {
    const req = await request.get(`${URL}/api/data/assets/with-tasks?project_id=${l_project_id}`).set(
      'Cookie', `access_token_cookie=${l_jwt}`,
    );
    expect(req.status).to.equal(200);
    console.log(req.body);
  });
  it('查询 todo', async function () {
    const req = await request.get(`${URL}/api/data/user/tasks`).set(
      'Cookie', `access_token_cookie=${l_jwt}`,
    );
    expect(req.status).to.equal(200);
    expect(req.body).to.be.an('array');
    expect(req.body[0]).to.have.keys(
      'name',
      'description',
      'priority',
      'difficulty',
      'duration',
      'estimation',
      'completion_rate',
      'retake_count',
      'sort_order',
      'start_date',
      'due_date',
      'real_start_date',
      'end_date',
      'done_date',
      'last_comment_date',
      'nb_assets_ready',
      'data',
      'shotgun_id',
      'last_preview_file_id',
      'project_id',
      'task_type_id',
      'task_status_id',
      'entity_id',
      'assigner_id',
      'assignees',
      'id',
      'created_at',
      'updated_at',
      'project_name',
      'project_has_avatar',
      'entity_name',
      'entity_description',
      'entity_data',
      'entity_preview_file_id',
      'entity_source_id',
      'entity_type_name',
      'entity_canceled',
      'sequence_name',
      'episode_id',
      'episode_name',
      'task_estimation',
      'task_duration',
      'task_start_date',
      'task_due_date',
      'task_type_name',
      'task_type_for_entity',
      'task_status_name',
      'task_type_color',
      'task_status_color',
      'task_status_short_name',
      'last_comment',
    );
    expect(req.body[0].last_comment).to.have.keys('text', 'date', 'person_id');

  });

});