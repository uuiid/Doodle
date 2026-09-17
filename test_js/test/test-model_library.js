// const superagent = require('superagent');
import request from 'superagent';
import { expect } from 'chai';

// const fs = require('node:fs');
import { URL, JWT } from './config.js';
import jwt_encode from 'jwt-encode';
import { describe } from 'mocha';
import mlog from 'mocha-logger';

describe('library测试', function () {
    var root_uuid, child_uuid, model_node_uuid;

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

    it('创建模型', async function () {
        const req5 = await request.post(`${URL}/api/doodle/model_library/assets`).send({
            label: '测试模型',
            parents: [root_uuid, child_uuid],
            path: 'D:\\test\\model\\test.ma',
            notes: '测试模型des',
            active: true,
            has_thumbnail: false,
            extension: 'png',
            tree_node_id: child_uuid,
        });
        expect(req5.status).to.equal(201);
        expect(req5.body).to.have.keys('id', 'label', 'parents', 'path', 'notes', 'active', 'has_thumbnail', 'extension');
        expect(req5.body.parents).to.be.an('array');
        expect(req5.body.parents).to.have.lengthOf(2);

        model_node_uuid = req5.body.id;
    })

    it('查询树', async function () {
        const req7 = await request.get(`${URL}/api/doodle/model_library/assets_tree`);
        expect(req7.status).to.equal(200);
        expect(req7.body).to.be.an('array');
        expect(req7.body[0]).to.have.keys('id', 'parent_id', 'label', 'order');
        expect(req7.body).to.have.lengthOf(3);
    })
    it('查询模型', async function () {
        const req8 = await request.get(`${URL}/api/doodle/model_library/assets`);
        expect(req8.status).to.equal(200);
        expect(req8.body).to.be.an('array');
        expect(req8.body[0]).to.have.keys('id', 'label', 'parents', 'path', 'notes', 'active', 'has_thumbnail', 'extension');
        expect(req8.body).to.have.lengthOf(1);
        expect(req8.body[0].id).to.equal(model_node_uuid);
        expect(req8.body[0].parents).to.be.an('array');
        expect(req8.body[0].parents).to.have.lengthOf(2);
    })


    it('删除树和模型之间的关联', async function () {
        const req10 = await request.delete(`${URL}/api/doodle/model_library/assets_tree/${root_uuid}/assets/${model_node_uuid}`);
        expect(req10.status).to.equal(200);

        const req8 = await request.get(`${URL}/api/doodle/model_library/assets`);
        expect(req8.status).to.equal(200);
        expect(req8.body).to.be.an('array');
        expect(req8.body[0]).to.have.keys('id', 'label', 'parents', 'path', 'notes', 'active', 'has_thumbnail', 'extension');
        expect(req8.body).to.have.lengthOf(1);
        expect(req8.body[0].id).to.equal(model_node_uuid);
        expect(req8.body[0].parents).to.be.an('array');
        expect(req8.body[0].parents).to.have.lengthOf(1);
    })
    it('添加树和模型之间的关联', async function () {
        const req11 = await request.post(`${URL}/api/doodle/model_library/assets_tree/${root_uuid}/assets/${model_node_uuid}`);
        expect(req11.status).to.equal(200);

        const req8 = await request.get(`${URL}/api/doodle/model_library/assets`);
        expect(req8.status).to.equal(200);
        expect(req8.body).to.be.an('array');
        expect(req8.body[0]).to.have.keys('id', 'label', 'parents', 'path', 'notes', 'active', 'has_thumbnail', 'extension');
        expect(req8.body).to.have.lengthOf(1);
        expect(req8.body[0].id).to.equal(model_node_uuid);
        expect(req8.body[0].parents).to.be.an('array');
        expect(req8.body[0].parents).to.have.lengthOf(2);
    })


    it('修改模型为未激活', async function () {
        if (!model_node_uuid) {
            const req8 = await request.get(`${URL}/api/doodle/model_library/assets`);
            expect(req8.status).to.equal(200);
            expect(req8.body).to.be.an('array');
            expect(req8.body[0]).to.have.keys('id', 'label', 'parents', 'path', 'notes', 'active', 'has_thumbnail', 'extension');
            expect(req8.body).to.have.lengthOf(1);
            model_node_uuid = req8.body[0].id;
        }
        const req12 = await request.put(`${URL}/api/doodle/model_library/assets/${model_node_uuid}`).send({
            active: false,
        });
        expect(req12.status).to.equal(200);

        const req8 = await request.get(`${URL}/api/doodle/model_library/assets`);
        expect(req8.status).to.equal(200);
        expect(req8.body).to.be.an('array');
        expect(req8.body[0]).to.have.keys('id', 'label', 'parents', 'path', 'notes', 'active', 'has_thumbnail', 'extension');
        expect(req8.body[0].parents).to.be.an('array');
        expect(req8.body[0].parents).to.have.lengthOf(2);
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


    it.skip('上下文', async function () {
        const req = await request.get(`${URL}/api/doodle/model_library/context`);
        expect(req.status).to.equal(200);
        expect(req.body).to.have.keys('tree_nodes');
        expect(req.body.tree_nodes).to.be.an('array');
        console.log(req.body);
    })


})