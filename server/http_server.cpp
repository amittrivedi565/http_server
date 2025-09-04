#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#define MAX_EVENTS 10
#define PORT 8080

/**
 * `fcntl` File Control Flags
 * By default our socket has blocking behaivour
 * which means if no data comes in from an client, we cannot handle others
 * 
 * To make non-blocking socket we use file control flags
 * file control flags explicitly mentions that if no data is coming
 * then switch to next client
 * 
 * `fcntl` requires a file descriptor, Operation Flag, mode
 * 
 * We are providing the `listen()` to `fcntl` because 
 * it is the entry point of our server
 * 
 * it cannot be non blocking.
 */
int make_socket_non_blocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main()
{
    /**
     * Create a listening socket for client
     * it will return a new file descriptor for socket
     */
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0)
    {
        perror("socket");
        return 1;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    /**
     * Bind the socket to our specific port
     */
    if (bind(listen_fd, (sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("bind");
        return 1;
    }

    /**
     * Listen for incoming clients on our socket
     */
    if (listen(listen_fd, SOMAXCONN) < 0)
    {
        perror("listen");
        return 1;
    }

    /**
     * `make_socket_blocking` has the implementation of `fctl()`
     * Make our listening socket non-blocking
    */
    make_socket_non_blocking(listen_fd);

    /**
     * Create a epoll instance using `epoll_create1`
     * it is a special object that can monitor multiple sockets for event
     * 
     * It returns a file descriptor, flat `0` describes normal epoll_instance
     */
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1)
    {
        perror("epoll_create1");
        return 1;
    }

    /**
     * Adding Listener on `listen` socket
     * specifically listen on reading
     * so basically whenever someone trys to connect to the server event is generated
     * EPOLLIN is generated
     */
    epoll_event event{};
    event.data.fd = listen_fd;
    event.events = EPOLLIN;
    
    /**
     * `epoll_ctl` is defines a definition for add sockets to the epoll instance
     * `epoll_ctl(epoll_instance_created, Operation name add or remove, for which fd, event defined earlier)`
    */
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &event) == -1)
    {
        perror("epoll_ctl");
        return 1;
    }

    /**
     * This events will store all the epoll events
     * With the capacity of `MAX_EVENTS`
     * lets suppose 15 connection simultaneously, it will first handle
     * 10 clients, remaining 5 clients will go to `epoll_wait()`
     */
    std::vector<epoll_event> events(MAX_EVENTS);

    std::cout << "HTTP server running on port " << PORT << std::endl;

    while (true)
    {
        /**
         * epoll_wait stores the socker which are ready for an event
         * these sockets are stored in event[]
         * 
         * Methodology:
         *  When client connects to the listening first it talks to the kernal
         *  Kernal places this connection request in pending backlog
         *  This pending backlog is cleared by `accept()`
         * 
         *  EPOllIN tells the epoll that `alert me when a client tries to connect`
         *  as soon as client connection comes in our listening socket becomes active
         * 
         * 
         */
        int n = epoll_wait(epoll_fd, events.data(), MAX_EVENTS, -1);
        for (int i = 0; i < n; ++i)
        {
            if (events[i].data.fd == listen_fd)
            {
                /* Client address details*/
                sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);

                /**
                 * Whenever a new client connects, a new file descriptor is created
                 */
                int client_fd = accept(listen_fd, (sockaddr *)&client_addr, &client_len);
                if (client_fd == -1)
                {
                    perror("accept");
                    continue;
                }

                /**
                 * By default all sockets are blocking
                 * we are making the new client socket non-blocking as well
                 */
                make_socket_non_blocking(client_fd);

                /**
                 * Again creating a specific watcher for this socket
                 * With specified operations such as EPOLLN -> client send data, EPOLLET -> new data arrives
                */
                epoll_event client_event{};
                client_event.data.fd = client_fd;
                client_event.events = EPOLLIN | EPOLLET; 
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &client_event);
            }
            else
            {
                /* This stores the client fd which are ready*/
                int client_fd = events[i].data.fd;
                char buf[4096];
                ssize_t count = read(client_fd, buf, sizeof(buf));
                if (count <= 0)
                {
                    close(client_fd);

                    /**
                     * This tells the epoll to stop monitoring this socket, no longer need to watch
                     */
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, nullptr);
                }
                else
                {
                    std::string response =
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Type: text/plain\r\n"
                        "Content-Length: 13\r\n"
                        "Connection: close\r\n"
                        "\r\n"
                        "Hello, world!";
                    write(client_fd, response.c_str(), response.size());
                    close(client_fd);
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, nullptr);
                }
            }
        }
    }

    close(listen_fd);
    return 0;
}
