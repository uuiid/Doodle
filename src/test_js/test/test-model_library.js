// const superagent = require('superagent');
import request from 'superagent';
import { expect } from 'chai';

// const fs = require('node:fs');
import { URL } from './config.js';
import jwt_encode from 'jwt-encode';


describe('library测试', function () {
    it('上下文', async function () {
        const req = await request.get(`${URL}/api/doodle/model_library/context`);
        expect(req.status).to.equal(200);
        expect(req.body).to.have.keys('tree_nodes', 'labels');
        expect(req.body.tree_nodes).to.be.an('array');
        expect(req.body.labels).to.be.an('array');
        console.log(req.body);
    })

    
})