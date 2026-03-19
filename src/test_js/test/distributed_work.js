

const URL = "ws://localhost:50025";


const l_list = [];
const l_list_2 = [];
for (let index = 0; index < 300; index++) {
  // 连接到 ws://192.168.20.89:50025/api/data/computers
  const socket = new WebSocket(`${URL}/api/data/computers`);
  l_list.push(socket);
  l_list_2.push(crypto.randomUUID());
  socket.addEventListener("open", (event) => {
    console.log("连接已打开", event);
    // 发送消息
    socket.send(
      JSON.stringify({
        name: `DESKTOP-5JAD0MB-${index}`,// 计算机名称
        state: "free",
        hardware_id: `${l_list_2[index]}` // 硬件ID, 随机生成
      })
    )
  });
  socket.addEventListener("message", (event) => {
    console.log("收到消息", event.data);
    // 等待1 - 10秒后 发送成功消息
    setTimeout(() => {
      console.log("发送成功消息", event.data.id);
      socket.send(
        JSON.stringify({
          name: `DESKTOP-5JAD0MB-${index}`,
          state: "online",
          hardware_id: `${l_list_2[index]}`
        })
      );
    }, Math.random() * 9000 + 1000);

  });
}