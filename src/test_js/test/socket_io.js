import {io} from "socket.io-client";

const URL = "http://localhost:50025";

const l_list = [];

for (let index = 0; index < 500; index++) {
  const socket = io(`${URL}/events`);
  socket.on("doodle:task_info:update", (...args) => console.log("message", ...args));
  l_list.push(socket);
}