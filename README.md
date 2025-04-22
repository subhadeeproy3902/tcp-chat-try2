# Multi-Room Chat Application

A cross-platform chat application with room-based functionality, supporting both local and online modes.

## Features

- Create and join chat rooms with random or custom room IDs
- Multi-group secure chatting
- Cross-platform compatibility (Windows and Linux)
- Support for both local and online modes
- Real-time messaging

## Project Structure

- `client.c` - C client application
- `server.c` - C server implementation
- `node/` - Node.js server implementation
  - `index.js` - Combined server (handles both web and TCP connections)

## Compilation

### Linux

```bash
# Compile the server
gcc server.c -o s.out

# Compile the client
gcc client.c -o c.out
```

### Windows

```bash
# Compile the server
gcc server.c -o s.exe -lws2_32

# Compile the client
gcc client.c -o c.exe -lws2_32
```

## Usage

### C Server

```bash
# Start the server
./s.out  # Linux
s.exe    # Windows
```

### Node.js Server

```bash
# Navigate to the node directory
cd node

# Install dependencies
npm install

# Start the server
node index.js
```

### Client

```bash
# Start the client
./c.out  # Linux
c.exe    # Windows
```

1. Enter your username
2. Choose connection mode:
   - Local: Connect to a server running on your computer
   - Online: Connect to a server running at localhost:8000
3. Choose to create a new room or join an existing one
4. Start chatting!

## Commands

- Type your message and press Enter to send
- Type `\EXIT` to quit the chat

## Cross-Platform Compatibility

The application is designed to work on both Windows and Linux systems. The C code uses conditional compilation to handle platform-specific differences.

## License

This project is open source and available under the MIT License.
