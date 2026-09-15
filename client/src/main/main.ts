
type HTMLElementCreateInfo = {
  id: number;
  type: string;
  classes: string[];
  initalizer: (element: HTMLElement) => void;
  html_id?: string;
  parent?: HTMLElement;
}

class Scene {
  div: HTMLDivElement = document.createElement("div");
  children: Map<number, HTMLElement> = new Map<number, HTMLElement>();
  constructor(html_create_info_arr?: HTMLElementCreateInfo[]) {
    if (html_create_info_arr !== undefined) {
      for (const info of html_create_info_arr) {
        this.appendChild(info);
      }
    }
    document.body.appendChild(this.div);
  }

  appendChild(info: HTMLElementCreateInfo): void {
    if (this.getChild(info.id) !== undefined) return;
    const element: HTMLElement = document.createElement(info.type);
    for (const cls of info.classes) {
      element.classList.add(cls);
    }
    if (info.html_id) {
      element.id = info.html_id;
    }
    if (info.parent) {
      info.parent.appendChild(element);
    } else {
      this.div.appendChild(element);
    }
    info.initalizer(element);
    this.children.set(info.id, element);
  }

  appendChildren(info_arr: HTMLElementCreateInfo[]): void {
    for (const info of info_arr) {
      this.appendChild(info);
    }
  }

  getChild(id: number): HTMLElement | undefined {
    return this.children.get(id);
  }

}



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

  const arr: HTMLElementCreateInfo[] = [
    {
      id: 0,
      type: "button",
      classes: [],
      initalizer: (element: HTMLElement) => {
        element.textContent = "Click me!";
      },
    },
  ];

  const scene = new Scene(arr);
  scene.appendChild({
    id: 1,
    type: "button",
    classes: [],
    initalizer: (element: HTMLElement) => {
      element.textContent = "Dont click me...";
    },
  });

  return 0;
}

main();
