// const superagent = require('superagent');
import request from 'superagent';
import { expect } from 'chai';

// const fs = require('node:fs');
import { URL, JWT } from './config.js';
import jwt_encode from 'jwt-encode';
import { describe } from 'mocha';
import mlog from 'mocha-logger';

describe('library测试', function () {
    var root_uuid, child_uuid, model_node_uuid, label_uuid1, label_uuid2;

    // afterEach('打印消息', function () {
    //     mlog.log("root_uuid", root_uuid);
    //     mlog.log("child_uuid", child_uuid);
    //     mlog.log("model_node_uuid", model_node_uuid);
    //     mlog.log("label_uuid1", label_uuid1);
    //     mlog.log("label_uuid2", label_uuid2);
    // })


    it('创建树', async function () {
        const req = await request.post(`${URL}/api/doodle/model_library/assets_tree`).send({
            label: '测试类别',
            order: 0,
        });
        expect(req.status).to.equal(201);
        expect(req.body).to.have.keys('id', 'parent_id', 'label', 'order');
        root_uuid = req.body.id;

        const req2 = await request.post(`${URL}/api/doodle/model_library/assets_tree`).send({
            parent_id: root_uuid,
            label: '测试子类别',
            order: 0,
        });
        expect(req2.status).to.equal(201);
        expect(req2.body).to.have.keys('id', 'parent_id', 'label', 'order');
        child_uuid = req2.body.id;
    })
    it('创建标签', async function () {

        const req3 = await request.post(`${URL}/api/doodle/model_library/label`).send({
            label: '测试标签',
        });
        expect(req3.status).to.equal(201);
        expect(req3.body).to.have.keys('id', 'label');
        label_uuid1 = req3.body.id;

        const req4 = await request.post(`${URL}/api/doodle/model_library/label`).send({
            label: '测试标签2',
        });
        expect(req4.status).to.equal(201);
        expect(req4.body).to.have.keys('id', 'label');
        label_uuid2 = req4.body.id;
        mlog.log(label_uuid1, label_uuid2);
    })

    it('创建模型', async function () {
        const req5 = await request.post(`${URL}/api/doodle/model_library/assets`).send({
            label: '测试模型',
            parent_id: root_uuid,
            path: 'D:\\test\\model\\test.ma',
            notes: '测试模型des',
            active: true,
            has_thumbnail: false,
            extension: 'png',
            tree_node_id: child_uuid,
            labels: [label_uuid1, label_uuid2],
        });
        expect(req5.status).to.equal(201);
        expect(req5.body).to.have.keys('id', 'label', 'parent_id', 'path', 'notes', 'active', 'has_thumbnail', 'extension', 'labels');
        model_node_uuid = req5.body.id;
    })


    it('查询标签', async function () {
        const req6 = await request.get(`${URL}/api/doodle/model_library/label`);
        expect(req6.status).to.equal(200);
        expect(req6.body).to.be.an('array');
        expect(req6.body[0]).to.have.keys('id', 'label');
        // 检查id是label_uuid1或者label_uuid2
        expect(req6.body[0].id).to.be.oneOf([label_uuid1, label_uuid2]);
        // 检查长度 2
        expect(req6.body).to.have.lengthOf(2);
    })
    it('查询树', async function () {
        const req7 = await request.get(`${URL}/api/doodle/model_library/assets_tree`);
        expect(req7.status).to.equal(200);
        expect(req7.body).to.be.an('array');
        expect(req7.body[0]).to.have.keys('id', 'parent_id', 'label', 'order');
        expect(req7.body).to.have.lengthOf(2);
    })
    it('查询模型', async function () {
        const req8 = await request.get(`${URL}/api/doodle/model_library/assets`);
        expect(req8.status).to.equal(200);
        expect(req8.body).to.be.an('array');
        expect(req8.body[0]).to.have.keys('id', 'label', 'parent_id', 'path', 'notes', 'active', 'has_thumbnail', 'extension', 'labels');
        expect(req8.body).to.have.lengthOf(1);
        expect(req8.body[0].id).to.equal(model_node_uuid);
        expect(req8.body[0].labels).to.have.lengthOf(2);
        expect(req8.body[0].labels).to.have.members([label_uuid1, label_uuid2]);
    })

    it('修改标签', async function () {
        const req9 = await request.put(`${URL}/api/doodle/model_library/label/${label_uuid1}`).send({
            label: '测试标签修改',
        });
        expect(req9.status).to.equal(200);
        expect(req9.body).to.have.keys('id', 'label');
        expect(req9.body.id).to.equal(label_uuid1);
        expect(req9.body.label).to.equal('测试标签修改');
    })


    it('删除标签和模型之间的关联', async function () {
        const req10 = await request.delete(`${URL}/api/doodle/model_library/label/${label_uuid1}/assets/${model_node_uuid}`);
        expect(req10.status).to.equal(200);

        const req8 = await request.get(`${URL}/api/doodle/model_library/assets`);
        expect(req8.status).to.equal(200);
        expect(req8.body).to.be.an('array');
        expect(req8.body[0]).to.have.keys('id', 'label', 'parent_id', 'path', 'notes', 'active', 'has_thumbnail', 'extension', 'labels');
        expect(req8.body).to.have.lengthOf(1);
        expect(req8.body[0].id).to.equal(model_node_uuid);
        expect(req8.body[0].labels).to.have.lengthOf(1);
        expect(req8.body[0].labels).to.have.members([label_uuid2]);
    })
    it('添加标签和模型之间的关联', async function () {
        const req11 = await request.post(`${URL}/api/doodle/model_library/label/${label_uuid1}/assets/${model_node_uuid}`);
        expect(req11.status).to.equal(200);

        const req8 = await request.get(`${URL}/api/doodle/model_library/assets`);
        expect(req8.status).to.equal(200);
        expect(req8.body).to.be.an('array');
        expect(req8.body[0]).to.have.keys('id', 'label', 'parent_id', 'path', 'notes', 'active', 'has_thumbnail', 'extension', 'labels');
        expect(req8.body).to.have.lengthOf(1);
        expect(req8.body[0].id).to.equal(model_node_uuid);
        expect(req8.body[0].labels).to.have.lengthOf(2);
        expect(req8.body[0].labels).to.have.members([label_uuid1, label_uuid2]);
    })


    it('修改模型为未激活', async function () {
        const req12 = await request.post(`${URL}/api/doodle/model_library/assets/${model_node_uuid}`).send({
            active: false,
        });
        expect(req12.status).to.equal(201);

        const req8 = await request.get(`${URL}/api/doodle/model_library/assets`);
        expect(req8.status).to.equal(200);
        expect(req8.body).to.be.an('array');
        expect(req8.body[0]).to.have.keys('id', 'label', 'parent_id', 'path', 'notes', 'active', 'has_thumbnail', 'extension', 'labels');
        expect(req8.body).to.have.lengthOf(1);
        expect(req8.body[0].id).to.equal(model_node_uuid);
        expect(req8.body[0].active).to.equal(false);
    })

    it('删除模型', async function () {
        const req12 = await request.delete(`${URL}/api/doodle/model_library/assets/${model_node_uuid}`).set('Cookie', `access_token_cookie=${JWT}`,);
        expect(req12.status).to.equal(200);
        const req8 = await request.get(`${URL}/api/doodle/model_library/assets`);
        expect(req8.status).to.equal(200);
        expect(req8.body).to.be.an('array');
        expect(req8.body).to.have.lengthOf(0);
    })

    it('删除标签', async function () {
        const req12 = await request.delete(`${URL}/api/doodle/model_library/label/${label_uuid1}`).set('Cookie', `access_token_cookie=${JWT}`,);
        expect(req12.status).to.equal(200);
        const req8 = await request.get(`${URL}/api/doodle/model_library/label`);
        expect(req8.status).to.equal(200);
        expect(req8.body).to.be.an('array');
        expect(req8.body).to.have.lengthOf(1);
    })

    it('上下文', async function () {
        const req = await request.get(`${URL}/api/doodle/model_library/context`);
        expect(req.status).to.equal(200);
        expect(req.body).to.have.keys('tree_nodes', 'labels');
        expect(req.body.tree_nodes).to.be.an('array');
        expect(req.body.labels).to.be.an('array');
        console.log(req.body);
    })


})