## TCP Server

simple `tcp` server with use of `poll()`

### Technologies
---
&nbsp;
- `C++`

&nbsp;
### Requirements
---
- Handle multiple clients
- Non-blocking
- Fast response time


### Internal Working of `poll()`

```internal_working_of_poll

Time 0: No clients connected
fds[0].revents = 0
fds[1..N].revents = 0
poll() blocks

Time 1: Client1 connects
kernel sets fds[0].revents = POLLIN
poll() returns
Server sees fds[0].revents & POLLIN
accept() → client_fd1 added to fds[1]
fds[0].revents cleared

Time 2: Client1 sends data
kernel sets fds[1].revents = POLLIN
poll() returns
Server reads data from client_fd1
fds[1].revents cleared

Time 3: Client2 connects
kernel sets fds[0].revents = POLLIN
poll() returns
accept() → client_fd2 added to fds[2]

```