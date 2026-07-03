export function connectTelemetry({ url, onOpen, onClose, onSnapshot }) {
  let socket;

  try {
    socket = new WebSocket(url);
  } catch {
    return { close() {} };
  }

  socket.addEventListener('open', onOpen);
  socket.addEventListener('close', onClose);
  socket.addEventListener('message', (event) => {
    try {
      onSnapshot(JSON.parse(event.data));
    } catch {
      // Ignore malformed telemetry frames.
    }
  });

  return {
    close() {
      if (socket && socket.readyState < 2) {
        socket.close();
      }
    }
  };
}
