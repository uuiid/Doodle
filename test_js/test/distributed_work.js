import { test, expect as pwExpect } from '@playwright/test';

import { URL as HTTP_URL } from './config.js';

const URL = process.env.DISTRIBUTED_WORK_URL || HTTP_URL.replace(/^http/, 'ws');
const WS_PATH = '/api/data/computers';

function openComputerSocket(payload, timeoutMs = 15000) {
  return new Promise((resolve, reject) => {
    const socket = new WebSocket(`${URL}${WS_PATH}`);
    const timer = setTimeout(() => {
      socket.close();
      reject(new Error(`WebSocket 连接超时（${timeoutMs}ms）`));
    }, timeoutMs);

    socket.addEventListener('open', () => {
      clearTimeout(timer);
      socket.send(JSON.stringify(payload));
      resolve(socket);
    });

    socket.addEventListener('error', (event) => {
      clearTimeout(timer);
      reject(new Error(`WebSocket 连接失败: ${event?.message ?? 'unknown'}`));
    });
  });
}

function waitForMessage(socket, timeoutMs = 15000) {
  return new Promise((resolve, reject) => {
    const timer = setTimeout(() => {
      socket.removeEventListener('message', onMessage);
      reject(new Error(`等待 WebSocket 消息超时（${timeoutMs}ms）`));
    }, timeoutMs);

    function onMessage(event) {
      clearTimeout(timer);
      socket.removeEventListener('message', onMessage);
      try {
        resolve(JSON.parse(event.data));
      } catch {
        resolve(event.data);
      }
    }

    socket.addEventListener('message', onMessage);
  });
}

test.describe('分布式工作机 WebSocket', () => {
  test.describe.configure({ timeout: 90000 });

  test('单台工作机上线并回执', async () => {
    const hardwareId = crypto.randomUUID();
    const name = `DESKTOP-${hardwareId.slice(0, 8)}`;

    const socket = await openComputerSocket({ name, state: 'free', hardware_id: hardwareId });
    try {
      const message = await waitForMessage(socket);
      console.log('收到服务端下发:', JSON.stringify(message));
      pwExpect(message).toBeTruthy();

      socket.send(JSON.stringify({ name, state: 'online', hardware_id: hardwareId }));
      const ack = await waitForMessage(socket);
      console.log('收到上线回执:', JSON.stringify(ack));
    } finally {
      socket.close();
    }
  });

  test('批量工作机并发上线', async () => {
    const count = Number(process.env.DISTRIBUTED_WORKER_COUNT || 20);
    const sockets = [];
    const hardwareIds = [];

    try {
      for (let index = 0; index < count; index++) {
        const hardwareId = crypto.randomUUID();
        hardwareIds.push(hardwareId);
        const socket = await openComputerSocket({
          name: `DESKTOP-5JAD0MB-${index}`,
          state: 'free',
          hardware_id: hardwareId,
        });
        sockets.push(socket);
      }

      console.log(`已建立 ${sockets.length} 个 WebSocket 连接`);

      const pending = sockets.map((socket, index) =>
        waitForMessage(socket).then((message) => {
          console.log(`工作机 ${index} 收到下发`, message?.id ?? message);
          socket.send(
            JSON.stringify({
              name: `DESKTOP-5JAD0MB-${index}`,
              state: 'online',
              hardware_id: hardwareIds[index],
            })
          );
          return message;
        })
      );

      const messages = await Promise.all(pending);
      pwExpect(messages.length).toBe(count);
    } finally {
      for (const socket of sockets) socket.close();
    }
  });
});
