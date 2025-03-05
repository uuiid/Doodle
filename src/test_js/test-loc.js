// const superagent = require('superagent');
import request from 'superagent';
import { expect } from 'chai';
import { v4 as uuidv4 } from 'uuid';

import { io } from 'socket.io-client';
// globalThis.chai = chai;

import { URL } from './config.js';
describe('doodle 本地服务器测试', function () {
  describe('提交作业测试', function () {
    it('连接事件测试', async function () {
      this.timeout(10000);

      const socket = new io(`${URL}/socket.io/`);
      socket.on('connect', function () {
        socket.close();
      });
      socket.io.on('connect_error', (error) => {
        console.log('error', error);
      });
      afterEach(() => {
        socket.close();
      });
    });


    it('should 提交maya作业', async function () {
      afterEach(() => {
        socket.close();
      });
      this.timeout(20000);
      const socket = io(`${URL}/socket.io/`);

      const req = await request.post(`${URL}/api/doodle/task`).send({
        name: '测试maya作业',
        source_computer: '本机',
        submitter: uuidv4(),
        run_computer_id: uuidv4(),
        type: 'check_maya',
        task_data: {
          path: 'C:\\Users\\63418\\Desktop\\JJ_EP001_SC001.ma',
          camera_film_aperture: 0.56,
          image_size: {
            width: 1080,
            height: 1920,
          },
          category: 'model_maya',
        },
      });
      expect(req.status).to.equal(200);
      expect(req.body).to.have.keys(
        'end_time',
        'id',
        'last_line_log',
        'name',
        'run_computer_id',
        'run_time',
        'source_computer',
        'status',
        'submit_time',
        'submitter',
        'type',
      );
      return new Promise((resolve) => {
        socket.onAny((eventName, data) => {
          expect(eventName).to.equal('doodle:task_info:update');
          expect(data).to.have.keys('task_info_id', 'log');
          socket.close();
          resolve();
        });
      });

    });

    it('should maya替换引用文件', async function () {
      const req = await request.post(`${URL}/api/doodle/task`).send({
        name: '测试maya作业',
        source_computer: '本机',
        submitter: uuidv4(),
        run_computer_id: uuidv4(),
        type: 'check_maya',
        task_data: {
          path: 'C:\\Users\\63418\\Desktop\\JJ_EP001_SC001.ma',
          file_list: [
            [
              'C:\\Users\\63418\\Desktop\\JJ_EP001_SC001.ma',// 旧的
              'C:\\Users\\63418\\Desktop\\JJ_EP001_SC001.ma',// 新的
            ],
            [
              'C:\\Users\\63418\\Desktop\\JJ_EP001_SC001.ma',
              'C:\\Users\\63418\\Desktop\\JJ_EP001_SC001.ma',
            ]
          ]
        },
      });
      expect(req.status).to.equal(200);
      expect(req.body).to.have.keys(
        'end_time',
        'id',
        'last_line_log',
        'name',
        'run_computer_id',
        'run_time',
        'source_computer',
        'status',
        'submit_time',
        'submitter',
        'type',
      );
    });
  });
});