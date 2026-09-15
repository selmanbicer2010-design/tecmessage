"use strict";
class Scene {
    constructor(html_create_info_arr) {
        this.div = document.createElement("div");
        this.children = new Map();
        if (html_create_info_arr !== undefined) {
            for (const info of html_create_info_arr) {
                this.appendChild(info);
            }
        }
        document.body.appendChild(this.div);
    }
    appendChild(info) {
        if (info.id === undefined)
            return;
        if (this.getChild(info.id) !== undefined)
            return;
        const element = document.createElement(info.type);
        for (const cls of info.classes) {
            element.classList.add(cls);
        }
        if (info.name) {
            element.id = info.name;
        }
        if (info.parent) {
            info.parent.appendChild(element);
        }
        else {
            this.div.appendChild(element);
        }
        info.initalizer(element);
        this.children.set(info.id, element);
    }
    appendChildren(info_arr) {
        for (const info of info_arr) {
            this.appendChild(info);
        }
    }
    getChild(id) {
        return this.children.get(id);
    }
}
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
    const arr = [
        {
            id: 0,
            type: "button",
            classes: [],
            initalizer: (element) => {
                element.textContent = "Click me!";
            },
        },
    ];
    const scene = new Scene(arr);
    scene.appendChild({
        id: 1,
        type: "button",
        classes: [],
        initalizer: (element) => {
            element.textContent = "Dont click me...";
        },
    });
    return 0;
}
main();
//# sourceMappingURL=main.js.map