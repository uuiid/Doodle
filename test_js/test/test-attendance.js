import { test } from '@playwright/test';
import { expect } from 'chai';

import { authHeader } from './config.js';
import { readJson } from './helpers.js';

const user_id = '69a8d093-dcab-4890-8f9d-c51ef065d03b';
const create_date = '2025-3-5';
const URL = process.env.DOODLE_BASE_URL || 'http://localhost:50025';

test.describe('考勤测试', () => {
  test('自定义考勤测试', async ({ request }) => {
    let l_id;
    {
      const res = await request.post(`${URL}/api/doodle/attendance/${user_id}/custom`, {
        data: {
          create_date: create_date,
          start_time: '2025-03-05 09:00:00',
          end_time: '2025-03-05 18:00:00',
          remark: '测试自定义考勤',
          type: 'leave',
        },
      });
      const req = await readJson(res);
      expect(req.status).to.equal(200);
      expect(req.body).to.have.keys('id', 'start_time', 'end_time', 'remark', 'type', 'is_custom');
      expect(req.body.start_time).to.equal('2025-03-05 09:00:00.000000');
      expect(req.body.end_time).to.equal('2025-03-05 18:00:00.000000');
      l_id = req.body.id;
    }

    {
      const res = await request.get(`${URL}/api/doodle/attendance/${user_id}/${create_date}`);
      const req = await readJson(res);
      expect(req.status).to.equal(200);
      expect(req.body).to.is.an('array');
      expect(req.body[0]).to.have.keys('id', 'start_time', 'end_time', 'remark', 'type', 'is_custom');
    }

    {
      const res = await request.put(`${URL}/api/doodle/attendance/custom/${l_id}`, {
        data: {
          remark: '测试自定义考勤 modified',
        },
      });
      const req = await readJson(res);
      expect(req.status).to.equal(200);
      expect(req.body).to.have.keys('id', 'start_time', 'end_time', 'remark', 'type', 'is_custom');
      expect(req.body.remark).to.equal('测试自定义考勤 modified');
    }
    {
      const res = await request.delete(`${URL}/api/doodle/attendance/custom/${l_id}`);
      const req = await readJson(res);
      expect(req.status).to.equal(200);
    }
  });

  test('自定义任务条目', async ({ request }) => {
    const res = await request.post(`${URL}/api/doodle/computing_time/${user_id}/${create_date}/custom`, {
      data: {
        project_name: '测试',
        season: 1,
        episode: 1,
        name: 'test',
        grade: '',
        user_remark: '',
        start_time: '2025-04-10 10:26:54',
        end_time: '2025-04-10 10:26:55',
      },
    });
    const req = await readJson(res);
    expect(req.status).to.equal(200);
    expect(req.body).to.have.keys(
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
    expect(req.body.start_time).to.equal('2025-04-10 10:26:54.000000');
    expect(req.body.end_time).to.equal('2025-04-10 10:26:55.000000');
    const l_id = req.body.id;
  });
});
