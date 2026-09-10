"use strict";
function main() {
    const socket = new WebSocket("ws://localhost:8080");
    socket.onopen = () => {
        console.log("connected");
    };
    socket.onmessage = (event) => {
        console.log("received:", event.data);
    };
    socket.onclose = () => {
        console.log("disconnected");
    };
    return 0;
}
main();
//# sourceMappingURL=main.js.map