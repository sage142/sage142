# Discord-like ImGui Chat Prototype (C++)

This project is a minimal desktop app using:
- **Dear ImGui** + **GLFW/OpenGL** for window/UI
- **Asio** for TCP networking

## Features
- Host your own local chat server on a port.
- Join a server by IP + port.
- Choose a username and chat in a simple `#general` feed.

## Build

```bash
cmake -S . -B build
cmake --build build -j
```

## Run

```bash
./build/discord_like_app
```

Open a second instance to simulate two users connecting to the same IP/port.

## Notes
- This is a demo architecture, not production Discord clone logic.
- No encryption, auth, persistence, or advanced channel/role permissions are included.
