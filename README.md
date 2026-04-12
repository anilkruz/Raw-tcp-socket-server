# Raw TCP Socket Server — C++

High performance epoll-based TCP echo server in C++.
Handles 10,000 concurrent clients on a single thread.

## Benchmark Results
- 10,000 clients simultaneously
- 1,000,000 total requests
- ~61,000 requests/second
- Single thread — no context switching

## Files
- `server.cc` — Simple single client server
- `epoll_server.cc` — Production epoll server (ET mode)
- `client.cc` — Interactive persistent client
- `stress_client.cc` — 10k clients benchmark tool

## Concepts Used
- epoll (Edge Triggered)
- Non-blocking sockets
- TCP state machine
- Kernel send/recv buffers
- Accept loop + Read loop

## Build
```bash
g++ epoll_server.cc -o epoll_server
g++ client.cc -o client
g++ stress_client.cc -o stress_client -lpthread
```

## Run
```bash
# Terminal 1
./epoll_server

# Terminal 2
./client

# Stress test
./stress_client
```
