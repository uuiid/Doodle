// const superagent = require('superagent');
import request from 'superagent';
import { expect } from 'chai';
import { v4 as uuidv4 } from 'uuid';

import { io } from 'socket.io-client';
// globalThis.chai = chai;

import { URL } from './config.js';


describe('doodle user测试', function() {
  it('测试鉴权', async function() {
    const req = await request.post(`${URL}/api/data/user/context`).send({
      username: 'admin',
      password: 'admin',
    });
    expect(req.status).to.equal(401);
  });
});