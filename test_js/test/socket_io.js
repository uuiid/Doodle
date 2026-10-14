import { test, expect as pwExpect } from '@playwright/test';
import { io } from 'socket.io-client';

import { URL } from './config.js';

const SOCKET_EVENT = 'doodle:task_info:update';

function waitForEvent(socket, eventName, timeoutMs = 10000) {
  return new Promise((resolve, reject) => {
    const timer = setTimeout(() => {
      socket.off(eventName, onEvent);
      reject(new Error(`等待事件 ${eventName} 超时（${timeoutMs}ms）`));
    }, timeoutMs);

    function onEvent(...args) {
      clearTimeout(timer);
      socket.off(eventName, onEvent);
      resolve(args);
    }

    socket.on(eventName, onEvent);
  });
}

test.describe('socket.io 事件推送', () => {
  test.describe.configure({ timeout: 60000 });

  test('连接事件测试', async () => {
    const socket = io(`${URL}/events`, { transports: ['websocket'] });
    try {
      await new Promise((resolve, reject) => {
        const timer = setTimeout(() => reject(new Error('socket.io 连接超时')), 10000);
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

  test('并发连接压力测试', async () => {
    const CONNECTION_COUNT = Number(process.env.SOCKET_IO_CONNECTION_COUNT || 20);
    const sockets = [];
    try {
      const connectOne = (index) =>
        new Promise((resolve, reject) => {
          const socket = io(`${URL}/events`, { transports: ['websocket'] });
          sockets.push(socket);
          const timer = setTimeout(
            () => reject(new Error(`第 ${index} 个连接超时`)),
            30000
          );
          socket.on('connect', () => {
            clearTimeout(timer);
            resolve(socket);
          });
          socket.io.on('connect_error', (error) => {
            clearTimeout(timer);
            reject(new Error(`第 ${index} 个连接失败: ${error?.message ?? error}`));
          });
        });

      await Promise.all(Array.from({ length: CONNECTION_COUNT }, (_, i) => connectOne(i)));

      const connected = sockets.filter((s) => s.connected).length;
      console.log(`已建立 ${connected}/${CONNECTION_COUNT} 个 socket.io 连接`);
      pwExpect(connected).toBe(CONNECTION_COUNT);

      const received = [];
      for (const socket of sockets) {
        socket.on(SOCKET_EVENT, (...args) => received.push(args));
      }
      console.log(`监听事件 ${SOCKET_EVENT}，等待服务端推送…`);
    } finally {
      for (const socket of sockets) socket.close();
    }
  });
});
