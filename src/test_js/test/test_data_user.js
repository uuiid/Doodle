// const superagent = require('superagent');
import request from 'superagent';
import { expect } from 'chai';
import { v4 as uuidv4 } from 'uuid';

import { io } from 'socket.io-client';
// globalThis.chai = chai;

import { URL } from './config.js';


describe('doodle user测试', function () {
  it('测试鉴权', async function () {
    try {
      const req = await request.get(`${URL}/api/data/user/context`);
    } catch (e) {
      expect(e.status).to.equal(401);
    }
  });
});