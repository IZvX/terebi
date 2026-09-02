const os = require("os");
const path = require("path");
const express = require("express");
const { WebSocketServer } = require("ws");

const PORT = Number(process.env.PORT || 3000);
const PAIRING_CODE = process.env.PAIRING_CODE || String(Math.floor(1000 + Math.random() * 900000));
const PAIRING_TOKEN = process.env.PAIRING_TOKEN || Math.random().toString(16).slice(2) + Date.now().toString(16);

const app = express();
const publicDir = path.join(__dirname, "public");
app.use(express.static(publicDir));
app.use("/public", express.static(publicDir));

const server = app.listen(PORT, "0.0.0.0", () => {
  console.log("Terebi remote server");
  console.log(`Pairing code: ${PAIRING_CODE}`);
  for (const address of lanAddresses()) {
    console.log(`Open: http://${address}:${PORT}`);
  }
});

const wss = new WebSocketServer({ server });

wss.on("connection", (ws, req) => {
  ws.paired = false;
  console.log(`Client connected: ${req.socket.remoteAddress}`);

  ws.on("message", (raw) => {
    let msg;
    try {
      msg = JSON.parse(raw);
    } catch {
      return reject(ws, "Invalid JSON");
    }

    if (!ws.paired) {
      if (msg.type !== "system" || msg.action !== "connect") {
        if (msg.type === "system" && msg.action === "request_pair") {
          ws.send(JSON.stringify({ type: "system", action: "pair_required" }));
          console.log(`Pairing requested. Code: ${PAIRING_CODE}`);
          return;
        }
        return reject(ws, "Pairing required");
      }

      if (msg.token !== PAIRING_TOKEN && String(msg.code || "") !== PAIRING_CODE) {
        ws.send(JSON.stringify({ type: "system", action: "rejected" }));
        return ws.close(1008, "Invalid pairing code");
      }

      ws.paired = true;
      ws.send(JSON.stringify({ type: "system", action: "paired", token: PAIRING_TOKEN }));
      console.log("Remote paired");
      return;
    }

    if (!isAllowedMessage(msg)) {
      return reject(ws, "Unsupported message");
    }

    // Terebi can consume these later through a native bridge, websocket client,
    // or by replacing this log with an IPC/event emitter.
    console.log("remote:", JSON.stringify(msg));
  });
});

function reject(ws, reason) {
  ws.send(JSON.stringify({ type: "system", action: "error", reason }));
}

function isAllowedMessage(msg) {
  if (!msg || typeof msg !== "object") return false;

  if (msg.type === "nav") {
    return ["up", "down", "left", "right", "select", "back", "home", "menu"].includes(msg.action);
  }

  if (msg.type === "pointer") {
    if (msg.action === "click") return true;
    return Number.isFinite(msg.dx) && Number.isFinite(msg.dy);
  }

  if (msg.type === "keyboard") {
    return typeof msg.text === "string" || ["backspace", "enter"].includes(msg.action);
  }

  return false;
}

function lanAddresses() {
  return Object.values(os.networkInterfaces())
    .flat()
    .filter((iface) => iface && iface.family === "IPv4" && !iface.internal)
    .map((iface) => iface.address);
}
