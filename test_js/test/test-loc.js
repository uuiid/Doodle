import { test, expect as pwExpect } from '@playwright/test';
import { expect } from 'chai';
import { v4 as uuidv4 } from 'uuid';

import { io } from 'socket.io-client';

import { URL } from './config.js';
import { readJson } from './helpers.js';

const VIDEO_PATH = process.env.DOODLE_VIDEO_SAMPLE || 'D:/test_files/ai/xuyang_data_set/xy_00.mov';

test.describe('doodle 本地服务器测试', () => {
  test.describe('提交作业测试', () => {
    test('连接事件测试', async () => {
      test.setTimeout(10000);

      const socket = io(`${URL}/socket.io/`);
      try {
        await new Promise((resolve, reject) => {
          const timer = setTimeout(() => reject(new Error('socket.io 连接超时')), 8000);
          socket.on('connect', () => {
            clearTimeout(timer);
            resolve();
          });
          socket.io.on('connect_error', (error) => {
            clearTimeout(timer);
            reject(new Error(`socket.io 连接失败: ${error?.message ?? error}`));
          });
        });
        pwExpect(socket.connected).toBe(true);
      } finally {
        socket.close();
      }
    });

    test('should 提交maya作业', async ({ request }) => {
      test.setTimeout(20000);
      const socket = io(`${URL}/socket.io/`);
      try {
        const res = await request.post(`${URL}/api/doodle/task`, {
          data: {
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
          },
        });
        const req = await readJson(res);
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
          'type'
        );

        const eventName = await new Promise((resolve, reject) => {
          const timer = setTimeout(() => reject(new Error('等待 doodle:task_info:update 事件超时')), 15000);
          socket.onAny((name, data) => {
            clearTimeout(timer);
            try {
              expect(name).to.equal('doodle:task_info:update');
              expect(data).to.have.keys(
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
                'type'
              );
              resolve(name);
            } catch (err) {
              reject(err);
            }
          });
        });
        pwExpect(eventName).toBe('doodle:task_info:update');
      } finally {
        socket.close();
      }
    });

    test('should maya替换引用文件', async ({ request }) => {
      const res = await request.post(`${URL}/api/doodle/task`, {
        data: {
          name: '测试maya作业',
          source_computer: '本机',
          submitter: uuidv4(),
          run_computer_id: uuidv4(),
          type: 'replace_maya_ref',
          task_data: {
            path: 'C:\\Users\\63418\\Desktop\\JJ_EP001_SC001.ma',
            file_list: [
              [
                'C:\\Users\\63418\\Desktop\\JJ_EP001_SC001.ma',
                'C:\\Users\\63418\\Desktop\\JJ_EP001_SC001.ma',
              ],
              [
                'C:\\Users\\63418\\Desktop\\JJ_EP001_SC001.ma',
                'C:\\Users\\63418\\Desktop\\JJ_EP001_SC001.ma',
              ],
            ],
          },
        },
      });
      const req = await readJson(res);
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
        'type'
      );
      expect(req.body.type).to.equal('replace_maya_ref');
    });

    test('自动灯光作业', async ({ request }) => {
      await request.post(`${URL}/api/doodle/task`, {
        data: {
          name: '测试maya 自动灯光作业',
          source_computer: '本机',
          submitter: uuidv4(),
          run_computer_id: uuidv4(),
          type: 'auto_light',
          task_data: {
            path: 'consequat anim occaecat',
            is_sim: true,
            camera_film_aperture: 1.75,
            episodes: 91,
            shot: {
              shot: 14,
              shot_enum: 'J',
            },
            image_size: {
              width: 1920,
              height: 1080,
            },
            layering: false,
            project: {
              name: '面领二族',
              path: 'sunt',
              en_str: 'cillum Excepteur',
              code: '17',
              auto_upload_path: 'nisi id sed quis voluptate',
            },
            bind_skin: true,
          },
        },
      });
    });

    test('获取视频截图', async ({ request }) => {
      const res = await request.post(`${URL}/api/doodle/video/thumbnail`, {
        data: {
          video_path: VIDEO_PATH,
          time: Math.random(),
        },
      });
      const req = await readJson(res);
      expect(req.status).to.equal(200);
    });

    test('获取视频截图 end', async ({ request }) => {
      const res = await request.post(`${URL}/api/doodle/video/thumbnail`, {
        data: {
          video_path: VIDEO_PATH,
          time: Math.random(),
        },
      });
      const req = await readJson(res);
      expect(req.status).to.equal(200);
    });
  });
});
