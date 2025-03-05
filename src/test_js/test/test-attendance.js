import request from 'superagent';
import { expect } from 'chai';
import { v4 as uuidv4 } from 'uuid';


import { URL } from './config.js';

const user_id = '69a8d093-dcab-4890-8f9d-c51ef065d03b';
const create_date = '2025-3-5';
describe('考勤测试', function () {
  describe('自定义考勤测试', function () {
    var l_id;
    it('创建考勤', async function () {

      const req = await request.post(`${URL}/api/doodle/attendance/${user_id}/custom`).send({
        create_date: create_date,
        start_time: '2025-3-5 9:00:00',
        end_time: '2025-3-5 18:00:00',
        remark: '测试自定义考勤',
        type: 'leave',
      });
      expect(req.status).to.equal(200);
      expect(req.body).to.have.keys('id',
        'start_time', 'end_time', 'remark', 'type', 'is_custom');
      l_id = req.body.id;
    });

    it('should 获取考勤', async function () {
      const req = await request.get(`${URL}/api/doodle/attendance/${user_id}/${create_date}`);
      expect(req.status).to.equal(200);
      expect(req.body).to.is.an('array');
      expect(req.body[0]).to.have.keys('id',
        'start_time', 'end_time', 'remark', 'type', 'is_custom');
    },
    );

    it('should 修改考勤', () => {
      const req = request.put(`${URL}/api/doodle/attendance/${l_id}`).send({
        remark: '测试自定义考勤 modified',
      });
      expect(req.status).to.equal(200);
      expect(req.body).to.have.keys('id',
        'start_time', 'end_time', 'remark', 'type', 'is_custom');
      expect(req.body.remark).to.equal('测试自定义考勤 modified');
    });
  });
});