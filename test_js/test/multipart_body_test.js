// multipart_body_test.js — 测试 multipart/form-data 文件上传
// 基于 HAR 文件: E:\cache\down\添加评论.har
// URL: http://192.168.20.89:50026/api/pictures/preview-files/01a084e2-9a70-7313-8070-b5196f069fc2
//

import { test } from '@playwright/test';
import { expect } from 'chai';
import fs from 'node:fs';

import { URL, authHeader, filePart } from './config.js';
import { readJson } from './helpers.js';

test.describe('multipart_body 表单上传测试', () => {
  test.describe.configure({ timeout: 30000 });

  const testFilePath = process.env.MULTIPART_TEST_FILE || 'E:/cache/down/sadsadasd.mp4';
  const taskId = process.env.MULTIPART_TASK_ID || '25122c8b-c1af-45fa-a6a4-f1cfbf22c510';

  let commentId = null;
  let previewFileId = null;

  test.beforeAll(() => {
    if (!fs.existsSync(testFilePath)) {
      console.log(`测试文件不存在: ${testFilePath}，相关用例将跳过`);
    }
  });

  test.afterAll(async ({ request }) => {
    if (commentId) {
      try {
        await request.delete(`${URL}/api/actions/tasks/${taskId}/comments/${commentId}`, {
          headers: authHeader,
        });
      } catch (e) {
        console.log('清理评论失败 (可能无权限):', e.message);
      }
    }
  });

  test('步骤1: 创建评论 (获取 comment_id)', async ({ request }) => {
    const res = await request.post(`${URL}/api/actions/tasks/${taskId}/comment`, {
      headers: authHeader,
      data: {
        task_status_id: '4ffc748e-4e58-4336-ba83-51910253514e',
        comment: 'multipart_body 测试评论',
        checklist: [],
        links: null,
      },
    });
    const req = await readJson(res);
    expect(req.status).to.equal(201);
    expect(req.body).to.have.property('id');
    commentId = req.body.id;
    console.log('comment_id:', commentId);
  });

  test('步骤2: 添加预览 (获取 preview_file_id)', async ({ request }) => {
    expect(commentId, '需要先创建评论').to.not.be.null;

    const res = await request.post(`${URL}/api/actions/tasks/${taskId}/comments/${commentId}/add-preview`, {
      headers: authHeader,
      data: {},
    });
    const req = await readJson(res);
    expect(req.status).to.equal(201);
    expect(req.body).to.have.property('id');
    previewFileId = req.body.id;
    console.log('preview_file_id:', previewFileId);
  });

  test('步骤3: 上传文件 (multipart/form-data)', async ({ request }) => {
    test.skip(!fs.existsSync(testFilePath), `测试文件不存在: ${testFilePath}`);
    expect(previewFileId, '需要先获取 preview_file_id').to.not.be.null;

    const fileStats = fs.statSync(testFilePath);
    console.log(`上传文件: ${testFilePath} (${fileStats.size} bytes)`);

    const res = await request.post(`${URL}/api/pictures/preview-files/${previewFileId}`, {
      headers: authHeader,
      multipart: { file: filePart(testFilePath) },
    });
    const req = await readJson(res);

    expect(req.status).to.equal(201);
    console.log('上传响应:', JSON.stringify(req.body, null, 2));

    expect(req.body).to.have.property('id');
    expect(req.body.id).to.equal(previewFileId);
    expect(req.body).to.have.property('extension');
    expect(req.body.extension).to.equal('mp4');
    expect(req.body).to.have.property('file_size');
    console.log('上传文件大小:', req.body.file_size);
  });

  test('步骤4: 验证上传的文件可访问', async ({ request }) => {
    expect(previewFileId, '需要先上传文件').to.not.be.null;

    const res = await request.get(`${URL}/api/actions/tasks/${taskId}/comments/${commentId}`, {
      headers: authHeader,
    });
    const req = await readJson(res);

    expect(req.status).to.equal(200);
    console.log('评论详情:', JSON.stringify(req.body, null, 2));

    if (req.body.previews && req.body.previews.length > 0) {
      const preview = req.body.previews.find((p) => p.id === previewFileId);
      expect(preview, '评论中应包含上传的预览文件').to.exist;
      if (preview) {
        expect(preview.extension).to.equal('mp4');
      }
    }
  });
});

test.describe('multipart_body 错误处理测试', () => {
  test.describe.configure({ timeout: 15000 });

  const nonExistentId = '00000000-0000-0000-0000-000000000000';
  const testFilePath = process.env.MULTIPART_TEST_FILE || 'E:/cache/down/sadsadasd.mp4';

  test('上传到不存在的 preview_file_id 应返回 404', async ({ request }) => {
    test.skip(!fs.existsSync(testFilePath), `测试文件不存在: ${testFilePath}`);

    const res = await request.post(`${URL}/api/pictures/preview-files/${nonExistentId}`, {
      headers: authHeader,
      multipart: { file: filePart(testFilePath) },
    });
    expect(res.status()).to.equal(404);
  });

  test('不附带文件上传应返回错误', async ({ request }) => {
    const res = await request.post(`${URL}/api/pictures/preview-files/${nonExistentId}`, {
      headers: { ...authHeader, 'Content-Type': 'multipart/form-data' },
      data: {},
    });
    expect([400, 404]).to.include(res.status());
  });
});
