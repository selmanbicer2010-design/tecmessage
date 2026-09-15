
function main(): number
{
  const socket = new WebSocket("ws://localhost:8080");

  socket.onopen = (): void => {
      console.log("connected");
  };

  socket.onmessage = (event): void => {
      console.log("received:", event.data);
  };

  socket.onclose = (): void => {
      console.log("disconnected");
  };

  window.addEventListener("keydown", (event: KeyboardEvent) => {
    event;
    socket.send("Hello");
  });

  return 0;
}

main();
