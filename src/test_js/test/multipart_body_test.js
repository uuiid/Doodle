// multipart_body_test.js — 测试 multipart/form-data 文件上传
// 基于 HAR 文件: E:\cache\down\添加评论.har
// URL: http://192.168.20.89:50026/api/pictures/preview-files/01a084e2-9a70-7313-8070-b5196f069fc2
//

import request from 'superagent';
import { expect } from 'chai';
import fs from 'node:fs';
import { URL, JWT } from './config.js';

describe('multipart_body 表单上传测试', function () {
  this.timeout(30000);

  const authHeader = { 'Cookie': `access_token_cookie=${JWT}` };

  // HAR 中的测试文件路径
  const testFilePath = 'E:/cache/down/sadsadasd.mp4';
  // HAR 中的任务 ID (用于创建评论)
  const taskId = '25122c8b-c1af-45fa-a6a4-f1cfbf22c510';

  let commentId = null;
  let previewFileId = null;

  before('检查测试文件是否存在', function () {
    expect(fs.existsSync(testFilePath), `测试文件不存在: ${testFilePath}`).to.be.true;
  });

  after(async function () {
    // 清理: 删除创建的评论
    if (commentId) {
      try {
        await request
          .delete(`${URL}/api/actions/tasks/${taskId}/comments/${commentId}`)
          .set(authHeader);
      } catch (e) {
        console.log('清理评论失败 (可能无权限):', e.message);
      }
    }
  });

  it('步骤1: 创建评论 (获取 comment_id)', async function () {
    const req = await request
      .post(`${URL}/api/actions/tasks/${taskId}/comment`)
      .set(authHeader)
      .send({
        task_status_id: '4ffc748e-4e58-4336-ba83-51910253514e',
        comment: 'multipart_body 测试评论',
        checklist: [],
        links: null,
      });
    expect(req.status).to.equal(201);
    expect(req.body).to.have.property('id');
    commentId = req.body.id;
    console.log('comment_id:', commentId);
  });

  it('步骤2: 添加预览 (获取 preview_file_id)', async function () {
    expect(commentId, '需要先创建评论').to.not.be.null;

    const req = await request
      .post(`${URL}/api/actions/tasks/${taskId}/comments/${commentId}/add-preview`)
      .set(authHeader)
      .send({});
    expect(req.status).to.equal(201);
    expect(req.body).to.have.property('id');
    previewFileId = req.body.id;
    console.log('preview_file_id:', previewFileId);
  });

  it('步骤3: 上传文件 (multipart/form-data)', async function () {
    expect(previewFileId, '需要先获取 preview_file_id').to.not.be.null;

    const fileStats = fs.statSync(testFilePath);
    console.log(`上传文件: ${testFilePath} (${fileStats.size} bytes)`);

    const req = await request
      .post(`${URL}/api/pictures/preview-files/${previewFileId}`)
      .set(authHeader)
      .attach('file', testFilePath);

    // 期望 201 Created (文件上传成功)
    expect(req.status).to.equal(201);
    console.log('上传响应:', JSON.stringify(req.body, null, 2));

    // 验证响应包含文件信息
    expect(req.body).to.have.property('id');
    expect(req.body.id).to.equal(previewFileId);
    expect(req.body).to.have.property('extension');
    expect(req.body.extension).to.equal('mp4');
    expect(req.body).to.have.property('file_size');
    console.log('上传文件大小:', req.body.file_size);
  });

  it('步骤4: 验证上传的文件可访问', async function () {
    expect(previewFileId, '需要先上传文件').to.not.be.null;

    // 获取评论详情, 验证预览文件已关联
    const req = await request
      .get(`${URL}/api/actions/tasks/${taskId}/comments/${commentId}`)
      .set(authHeader);

    expect(req.status).to.equal(200);
    console.log('评论详情:', JSON.stringify(req.body, null, 2));

    // 检查 previews 数组中包含上传的文件
    if (req.body.previews && req.body.previews.length > 0) {
      const preview = req.body.previews.find(p => p.id === previewFileId);
      expect(preview, '评论中应包含上传的预览文件').to.exist;
      if (preview) {
        expect(preview.extension).to.equal('mp4');
      }
    }
  });
});

describe('multipart_body 错误处理测试', function () {
  this.timeout(15000);

  const authHeader = { 'Cookie': `access_token_cookie=${JWT}` };
  const nonExistentId = '00000000-0000-0000-0000-000000000000';

  it('上传到不存在的 preview_file_id 应返回 404', async function () {
    try {
      await request
        .post(`${URL}/api/pictures/preview-files/${nonExistentId}`)
        .set(authHeader)
        .attach('file', 'E:/cache/down/sadsadasd.mp4');
      expect.fail('应抛出 404 错误');
    } catch (err) {
      expect(err.status).to.equal(404);
    }
  });

  it('不附带文件上传应返回错误', async function () {
    try {
      await request
        .post(`${URL}/api/pictures/preview-files/${nonExistentId}`)
        .set(authHeader)
        .set('Content-Type', 'multipart/form-data')
        .send();
      expect.fail('应抛出错误');
    } catch (err) {
      // 可能是 400 或 404
      expect(err.status).to.be.oneOf([400, 404]);
    }
  });
});