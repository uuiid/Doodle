import { test } from '@playwright/test';
import { expect } from 'chai';

import { URL, authHeader } from './config.js';
import { readJson } from './helpers.js';

test.describe('doodle user测试', () => {
  const l_project_id = 'eb2b5cde-c1cf-47a0-aba4-e196b6f774dd';

  test('配置', async ({ request }) => {
    const res = await request.get(`${URL}/api/config`);
    const req = await readJson(res);
    expect(req.status).to.equal(200);
  });

  test('登录', async ({ request }) => {
    const res = await request.post(`${URL}/api/auth/login`, {
      data: {
        email: 'test_mod@qq.com',
        password: 'test_mod',
      },
    });
    const req = await readJson(res);
    expect(req.status).to.equal(200);
  });

  test('测试鉴权', async ({ request }) => {
    const res = await request.get(`${URL}/api/data/user/context`);
    expect(res.status()).to.equal(401);
  });

  test('测试授权', async ({ request }) => {
    const res = await request.get(`${URL}/api/auth/authenticated`, { headers: authHeader });
    const req = await readJson(res);
    expect(req.status).to.equal(200);

    expect(req.body.authenticated).to.equal(true);
    expect(req.body.user).to.have.keys(
      'active',
      'archived',
      'contract_type',
      'data',
      'departments',
      'desktop_login',
      'dingding_company_id',
      'email',
      'email_otp_enabled',
      'expiration_date',
      'fido_devices',
      'fido_enabled',
      'first_name',
      'full_name',
      'has_avatar',
      'id',
      'is_bot',
      'is_generated_from_ldap',
      'last_login_failed',
      'last_name',
      'last_presence',
      'ldap_uid',
      'locale',
      'login_failed_attemps',
      'notifications_discord_enabled',
      'notifications_discord_userid',
      'notifications_enabled',
      'notifications_mattermost_enabled',
      'notifications_mattermost_userid',
      'notifications_slack_enabled',
      'notifications_slack_userid',
      'phone',
      'preferred_two_factor_authentication',
      'role',
      'shotgun_id',
      'studio_id',
      'timezone',
      'totp_enabled'
    );
    expect(req.body.organisation).to.have.keys(
      'chat_token_discord',
      'chat_token_slack',
      'chat_webhook_mattermost',
      'dark_theme_by_default',
      'format_duration_in_hours',
      'has_avatar',
      'hd_by_default',
      'hours_by_day',
      'id',
      'name',
      'timesheets_locked',
      'use_original_file_name'
    );
  });

  test('测试登录用户的上下文', async ({ request }) => {
    const res = await request.get(`${URL}/api/data/user/context`, { headers: authHeader });
    const req = await readJson(res);
    expect(req.status).to.equal(200);
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
      'user_limit'
    );
  });

  test('组织', async ({ request }) => {
    const res = await request.get(`${URL}/api/data/organisations`, { headers: authHeader });
    const req = await readJson(res);
    expect(req.status).to.equal(200);
  });

  test('获取资产上下文', async ({ request }) => {
    const res = await request.get(`${URL}/api/data/assets/with-tasks?project_id=${l_project_id}`, {
      headers: authHeader,
    });
    const req = await readJson(res);
    expect(req.status).to.equal(200);
    console.log(req.body);
  });

  test('查询 todo', async ({ request }) => {
    const res = await request.get(`${URL}/api/data/user/tasks`, { headers: authHeader });
    const req = await readJson(res);
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
      'last_comment'
    );
    expect(req.body[1].last_comment).to.have.keys('text', 'date', 'person_id');
  });

  test('查询 done todo', async ({ request }) => {
    const res = await request.get(`${URL}/api/data/user/done-tasks`, { headers: authHeader });
    const req = await readJson(res);
    expect(req.status).to.equal(200);
  });

  test('查询 时间', async ({ request }) => {
    const res = await request.get(`${URL}/api/data/persons/69a8d093-dcab-4890-8f9d-c51ef065d03b/day-offs/`);
    const req = await readJson(res);
    expect(req.status).to.equal(200);
    expect(req.body).to.be.an('array');
    expect(req.body).to.have.lengthOf(0);
  });
});
