# Tic-Tac-Toe over TCP

A two-player networked tic-tac-toe game written in C++ with Winsock. Two computers (or two windows on the same computer) connect over TCP and take turns in the terminal.

```
O|2|3
-+-+-
4|X|6
-+-+-
7|8|O

[X]>>
```

## Features

- TCP socket connection (Winsock2, port `5550`)
- The server waits for players on a background `std::thread`, plays one player at a time, and waits for the next player when a game ends
- Rejects invalid moves (outside 1–9, an occupied square, or input that isn't a number)
- Detects a win (3 rows, 3 columns, 2 diagonals) or a draw

## Project structure

| Folder | Description |
|---|---|
| `server/` | Server side (`#define SERVER_MODE`), moves first, plays **O** |
| `client/` | Client side, connects to the server's IP, plays **X** |

Both sides use exactly the same code. The only difference is whether `#define SERVER_MODE` on line 14 is commented out.

## How to run

Requires Windows and Visual Studio (with the "Desktop development with C++" workload).

1. Open `server/HW10-server.sln` in Visual Studio, build and run it. It shows `Waiting for new clients...`
2. Open `client/HW10-client3.sln`, build and run it
3. In the client, enter the server's IP (`127.0.0.1` if both run on the same computer)
4. Take turns entering a number from 1 to 9 to place your mark

## Notes

This was an assignment (HW10) for a university Software Design course.
