export function enableDevReload() {
  if (location.hostname !== "localhost") return;

  const proto = location.protocol === "https:" ? "wss://" : "ws://";
  const ws = new WebSocket(proto + location.host);

  ws.onopen = () => {
    console.log("[dev] websocket connected");
  };

  ws.onmessage = (e) => {
    if (e.data === "reload") {
      console.log("[dev] reload");
      location.reload();
    }
  };

  ws.onerror = () => {
    console.warn("[dev] websocket error");
  };

  ws.onclose = () => {
    console.warn("[dev] websocket closed");
  };
}
