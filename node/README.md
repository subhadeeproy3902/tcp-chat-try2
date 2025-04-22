# Node.js Chat Server

This is a Node.js implementation of the multi-room chat server that provides the same functionality as the C server.

## Features

- Create and join chat rooms with random or custom room IDs
- Real-time messaging within rooms
- Support for multiple concurrent rooms
- Compatible with the C client through TCP adapter

## Installation

```bash
# Install dependencies
npm install
```

## Usage

There are two ways to run the server:

### 1. Socket.IO Server (for web clients)

```bash
npm start
```

This starts the Socket.IO server on port 2864.

### 2. TCP Adapter (for C clients)

```bash
npm run start-tcp
```

This starts a TCP server that acts as an adapter between the C client and the Socket.IO server.

## Connecting with C Client

The C client can connect to this server by selecting the "online" option and providing the server's IP address.

## Protocol

The server uses the same protocol as the C server:

- Connection format: `username|roomID|action`
  - action: 0 = create room, 1 = join room
- Response format: `SUCCESS|roomName` or `ERROR|errorMessage`
- Messages are sent as plain text
