import { test } from '@playwright/test';
import { expect } from 'chai';

import { URL, authHeader } from './config.js';
import { readJson } from './helpers.js';

const USER_ID = 'c6dfe23a-5bbb-4cc3-9493-6440b8628448';
const YEAR_MONTH = '2025-02';

test.describe('工作表测试', () => {
  test('添加自定义条目', async ({ request }) => {
    const res = await request.post(`${URL}/api/doodle/computing_time/${USER_ID}/${YEAR_MONTH}/custom`, {
      headers: authHeader,
      data: {
        project_id: 'eb2b5cde-c1cf-47a0-aba4-e196b6f774dd',
        season: 1,
        episode: 2,
        name: '1',
        grade: 's',
        user_remark: '',
        start_time: '2025-02-10 18:11:14',
        end_time: '2025-02-10 18:11:16',
      },
    });
    const req = await readJson(res);
    expect(req.status).to.equal(201);
    expect(req.body).to.is.an('array');
    expect(req.body[0]).to.have.keys(
      'id',
      'start_time',
      'end_time',
      'project_name',
      'season',
      'episode',
      'name',
      'grade',
      'user_remark'
    );
    expect(req.body[0].start_time).to.equal('2025-02-10 18:11:14.000000');
    expect(req.body[0].end_time).to.equal('2025-02-10 18:11:16.000000');
    const l_id = req.body[0].id;
  });

  test('添加引用task', async ({ request }) => {
    await request.post(`${URL}/api/doodle/computing_time/${USER_ID}/${YEAR_MONTH}`, {
      headers: authHeader,
    });
  });

  test('获取考勤', async ({ request }) => {
    const res = await request.get(`${URL}/api/doodle/computing_time/${USER_ID}/${YEAR_MONTH}`, {
      headers: authHeader,
    });
    const req = await readJson(res);
    expect(req.status).to.equal(200);
    expect(req.body).to.is.an('array');
    expect(req.body[0]).to.have.keys(
      'task_id',
      'task_name',
      'task_last_preview_file_id',
      'entity_ji_shu_lie',
      'entity_deng_ji',
      'entity_gui_dang',
      'entity_bian_hao',
      'entity_pin_yin_ming_cheng',
      'entity_ban_ben',
      'entity_ji_du',
      'entity_kai_shi_ji_shu',
      'project_uuid',
      'project_name',
      'id',
      'work_start_time',
      'work_end_time',
      'work_duration',
      'work_remark',
      'work_user_remark',
      'work_year_month'
    );
    console.info(req.body);
  });
});
