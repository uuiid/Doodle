import { io } from "socket.io-client";

const URL = "http://localhost:50025";

const l_list = [];

for (let index = 0; index < 30; index++) {
  const socket = io(`${URL}/events`, {
    transports: ["websocket"]
  });
  socket.on("doodle:task_info:update", (...args) => console.log("message", ...args));
  // socket.onAny((event, ...args) => {
  //   console.log(`got ${event}`);
  // });
  l_list.push(socket);
}