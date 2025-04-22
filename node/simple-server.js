const net = require('net');

const PORT = 2864;
const MAX_ROOMS = 20;
const ROOM_ID_LENGTH = 6;

// Store rooms and clients
const rooms = new Map();
const clients = new Map();

// Generate a timestamp in the format "HH:MM AM/PM"
function getTimestamp() {
  const now = new Date();
  return now.toLocaleTimeString('en-US', { hour: '2-digit', minute: '2-digit' });
}

// Create TCP server for C clients
const server = net.createServer((socket) => {
  console.log('New client connected');
  
  let clientId = null;
  let connectionEstablished = false;
  let username = '';
  let roomId = '';
  
  // Handle data from client
  socket.on('data', (data) => {
    const message = data.toString().trim();
    console.log(`Received: ${message}`);
    
    if (!connectionEstablished) {
      // First message is connection data (username|roomID|action)
      console.log(`Received connection data: ${message}`);
      
      const parts = message.split('|');
      if (parts.length !== 3) {
        socket.write('ERROR|Invalid connection data format');
        socket.end();
        return;
      }
      
      username = parts[0];
      roomId = parts[1];
      const action = parseInt(parts[2]);
      
      // Generate a unique ID for this client
      clientId = `tcp-${Date.now()}-${Math.random().toString(36).substr(2, 9)}`;
      
      // Handle room creation or joining
      if (action === 0) { // Create room
        if (rooms.has(roomId)) {
          // Room already exists
          socket.write(`ERROR|Room with ID ${roomId} already exists`);
          socket.end();
          return;
        }
        
        // Create new room
        if (rooms.size < MAX_ROOMS) {
          rooms.set(roomId, {
            id: roomId,
            name: `Room ${roomId}`,
            clientCount: 0
          });
        } else {
          socket.write('ERROR|Maximum number of rooms reached');
          socket.end();
          return;
        }
      } else { // Join room
        if (!rooms.has(roomId)) {
          // Room doesn't exist
          socket.write(`ERROR|Room with ID ${roomId} doesn't exist`);
          socket.end();
          return;
        }
      }
      
      // Add client to room
      clients.set(clientId, {
        username,
        roomId,
        socket
      });
      
      const room = rooms.get(roomId);
      room.clientCount++;
      
      // Send success response with room name
      socket.write(`SUCCESS|${room.name}`);
      connectionEstablished = true;
      
      const time = getTimestamp();
      console.log(`[${time}] ${username} joined room ${roomId}`);
      
      // Notify users in the same room
      const joinMessage = `\x1b[1m[${time}]\x1b[0m \x1b[1;32m${username}\x1b[0m joined the chat!`;
      
      for (const [id, client] of clients.entries()) {
        if (id !== clientId && client.roomId === roomId) {
          client.socket.write(joinMessage);
        }
      }
    } else {
      // Regular message, broadcast to room
      const time = getTimestamp();
      
      const formattedMsg = `\x1b[1m[${time}]\x1b[0m \x1b[1;36m${username}\x1b[0m: ${message}`;
      console.log(`${formattedMsg} (Room: ${roomId})`);
      
      // Send message to all clients in the same room
      for (const [id, client] of clients.entries()) {
        if (id !== clientId && client.roomId === roomId) {
          client.socket.write(formattedMsg);
        }
      }
    }
  });
  
  // Handle client disconnection
  socket.on('end', () => {
    console.log('Client disconnected');
    if (clientId && clients.has(clientId)) {
      const client = clients.get(clientId);
      const { username, roomId } = client;
      
      const time = getTimestamp();
      console.log(`[${time}] ${username} left room ${roomId}`);
      
      // Notify users in the same room
      const leftMessage = `\x1b[1m[${time}]\x1b[0m 👋 \x1b[1;31m${username}\x1b[0m left the chat!`;
      
      for (const [id, client] of clients.entries()) {
        if (id !== clientId && client.roomId === roomId) {
          client.socket.write(leftMessage);
        }
      }
      
      // Update room client count
      if (rooms.has(roomId)) {
        const room = rooms.get(roomId);
        room.clientCount--;
        
        // If room is empty, remove it
        if (room.clientCount <= 0) {
          console.log(`Room ${roomId} is now empty and will be removed`);
          rooms.delete(roomId);
        }
      }
      
      // Remove client from clients map
      clients.delete(clientId);
    }
  });
  
  // Handle errors
  socket.on('error', (err) => {
    console.error('Socket error:', err);
    if (clientId && clients.has(clientId)) {
      clients.delete(clientId);
    }
  });
});

// Start the server
server.listen(PORT, () => {
  console.log(`\n\x1b[1mSimple TCP Chat Server Running on port ${PORT}\x1b[0m\n`);
});
