/**
 * Integration of `poll()` with HTTP Server
 * `poll()` is a system call that allows a program to monitor multiple file descriptors
 * to see if they are ready for reading or writing.
 * This allows us to handle multiple client connections and requests.
 * 
 * Whenever a client requests a connection, the server accepts the connection and adds it to the poll list.
 * Use `poll()` to check whether any data is to be read from any connected clients.
 */

#include <poll.h>      // For poll() and struct pollfd
#include <unistd.h>    // For close() and read()
#include <arpa/inet.h> // For sockaddr_in and socket functions
#include <iostream>    // For standard I/O
#include <cstring>     // For memset()
#include <vector>      // For using std::vector

#define PORT 8080
#define MAX_CLIENTS 10

int main() {

    /**
     * Creates a socket
     * It will return a file descriptor
     */
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Socket creation failed");
        return 1;
    }

    /**
     * Bind to port 8080
     * `bind()` @param file descriptor of socket, pointer to a sockaddr struct that address info, size of socketaddr
     */
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);
    address.sin_addr.s_addr = INADDR_ANY; // Listen on all interfaces

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        return 1;
    }

    /**
     * This puts the server socket onto listen mode
    */
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        return 1;
    }

    std::cout << "TCP Server started on port " << PORT << std::endl;

    /**
     * `poll()` configuration
     * pollfd `fds` is predefined struct for poll
     * We have set the size of pull to MAX_CLIENT+1 since socket will also occupy
     * First element of poll be the server socket file descriptor
     * Event in which we are interested is POLLIN, read operation
     * 
     * fds[0].fd tells which file descriptor we are monitoring in this case server_fd
     * fds.events constantly listens on the event occurence definitely
     * 
     * Internal Working:
     * 
     * so basically kernal maintains a backlog queue of pending connections
     * whenever there is a pending connection, set POLL.REVENTS = POLLIN
     * if no revents = 0
     */
    struct pollfd fds[MAX_CLIENTS + 1];
    fds[0].fd = server_fd;               
    fds[0].events = POLLIN;              
    fds[0].revents = 0;                 

    /**
     * Intialize all the empty slots to -1
     * To indicate slot emptyness
     */
    for (int i = 1; i <= MAX_CLIENTS; ++i) {
        fds[i].fd = -1;                 
    }

    /**
     * Used to connect to the server
     * When client connects it returns a new file descriptor
     * Upon every new connection new file descriptor is created
     * This client file descriptor resides on server side
     * Client uses this fd to communicate through
     */
    while (true) {
        /**
         * `poll(struct pollfd *fds, nfds_t nfds, int timeout)`
         * @args pollfd take pointer to an pollfd type struct
         * @args nfds number of entries in the fds array
         * @args int timout, indicates for how much time to wait for an event to occur
         * 
         * -1 means wait indefinitely
         * 
         * here we go in the kernal mode 
        */
        int ready = poll(fds, MAX_CLIENTS + 1, -1); // -1 means wait indefinitely
        if (ready < 0) {
            perror("Poll failed");
            break;
        }

        /**
         * Since our fd is listening on event
         * We only care about POLLIN event that has been placed
         * Meaning client sockets which are only interested in read operation
         */
        if (fds[0].revents & POLLIN) {
            sockaddr_in client_addr{};
            socklen_t addr_len = sizeof(client_addr);
            int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
            if (client_fd >= 0) {
                std::cout << "New connection accepted" << std::endl;
                // Add new client to the poll array
                for (int i = 1; i <= MAX_CLIENTS; ++i) {
                    if (fds[i].fd == -1) { // Find an empty slot
                        fds[i].fd = client_fd; // Assign client fd to the poll array
                        fds[i].events = POLLIN; // Monitor for incoming data
                        break;
                    }
                }
            } else {
                perror("Accept failed");
            }
        }

        // Step 7: Check for data from clients
        for (int i = 1; i <= MAX_CLIENTS; ++i) {
            if (fds[i].fd != -1 && (fds[i].revents & POLLIN)) {
                char buffer[1024] = {0};
                int bytes_read = read(fds[i].fd, buffer, sizeof(buffer) - 1);
                if (bytes_read > 0) {
                    std::cout << "Received data from client: " << buffer << std::endl;
                } else {
                    // Handle client disconnection
                    std::cout << "Client disconnected" << std::endl;
                    close(fds[i].fd);
                    fds[i].fd = -1; // Mark as available
                }
            }
        }
    }

    close(server_fd);
    return 0;
}
