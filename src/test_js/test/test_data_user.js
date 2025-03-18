// const superagent = require('superagent');
import request from 'superagent';
import { expect } from 'chai';
import { v4 as uuidv4 } from 'uuid';

import { io } from 'socket.io-client';
// globalThis.chai = chai;

import { URL } from './config.js';
import jwt_encode from 'jwt-encode';

describe('doodle user测试', function() {
  it('测试鉴权', async function() {
    try {
      const req = await request.get(`${URL}/api/data/user/context`);
    } catch (e) {
      expect(e.status).to.equal(401);
    }
  });
  it('测试登录用户的上下文', async function() {
    const l_jwt = jwt_encode({
      'id': 'b5a1383a-8bdc-4a79-ad33-f601fa0fc26d',
    }, 'secret');
    const req = await request.get(`${URL}/api/data/user/context`).set(
      'Cookie', `access_token_cookie=${l_jwt}`,
    );
    expect(req.status).to.equal(200);
    expect(req.body).to.have.keys('user_id', 'name');
  });
});