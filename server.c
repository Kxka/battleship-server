#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>

#include "ship.h"


user *users = NULL; 

// Global variables 
fd_set set;    
int max_fd;

int main(int argc, char *argv[]) {

    // Ensure proper format ./server PORT
    if (argc != 2) {
        fprintf(stderr, "Usage: server PORT\n");
        exit(1);
    }

    // Ignore SIGPIPE
    signal(SIGPIPE, SIG_IGN);

    int port = atoi(argv[1]);

    // create socket
    int sfd = socket(AF_INET, SOCK_STREAM, 0);


    // Fill in address struct
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));

    // IPV4
    address.sin_family = AF_INET;   
    // set port            
    address.sin_port = htons(port);       
    // bind to all interfaces
    address.sin_addr.s_addr = INADDR_ANY;

    // call bind
    bind(sfd, (struct sockaddr *)&address, sizeof(address));

    //listen
    listen(sfd, 1000);

    // Initialize fd sets. set = original set, readset = temp set for select()

    fd_set readset;
    FD_ZERO(&set);
    FD_ZERO(&readset);
    FD_SET(sfd, &set);
    max_fd = sfd;
    
    while (true) {
        // copy set to readset before select
        readset = set;

        // find number of fds ready
        int ready = select(max_fd + 1, &readset, NULL, NULL, NULL);
        if (ready == -1) {
            continue;
        }

        // check if new connection
        if (FD_ISSET(sfd, &readset)) {

            // New Connection
            user *new_user = new_connection(sfd);
            
            if (new_user != NULL) {
                // Add the new player's socket FD
                FD_SET(new_user->fd, &set);
                
                // Update max_fd 
                if (new_user->fd > max_fd) {
                    max_fd = new_user->fd;
                }
            }
            
            // Decrement the count of ready descriptors
            ready--;
            // continue to next loop if no more ready fds
            if (ready <= 0) {
                continue;
            }
        }

        // Check for input, iterate through all players
        user *p = users;
        user *next;
        while (p != NULL) {
            next = p->next; 

            // If fd is ready to read
            if (FD_ISSET(p->fd, &readset)) {
                
                // Backup fd for removal
                int fd_backup = p->fd; 
                // Read data 
                bool keep_connection = client(p);
                
                // Disconnect, remove player
                if (!keep_connection) {
                    FD_CLR(fd_backup, &set);
                    
                    // Update max_fd if needed
                    if (fd_backup == max_fd) {
                        max_fd = sfd;
                        for (user *u = users; u; u = u->next) {
                            if (u->fd > max_fd) {
                                max_fd = u->fd;
                            }
                        }
                    }
                }
            }
            
            p = next; 
        }
    }

    // Free memory and close socket
    user *p = users;
    while (p) {
        user *next = p->next;
        close(p->fd);
        free(p);
        p = next;
    }
    users = NULL;
    close(sfd);
    return 0;
}

void remove_user(user *u) {
    user **curr = &users;
    while (*curr) {
        if (*curr == u) {
            *curr = u->next;
            close(u->fd);
            free(u);
            return;
        }
        curr = &(*curr)->next;
    }
}

user *new_connection(int sfd) {

    // Empty address struct
    struct sockaddr_in addr;
    socklen_t cli_len = sizeof(addr);

    // Accept new connection
    int clientfd = accept(sfd, (struct sockaddr *)&addr, &cli_len);

    // New user
    user *u = calloc(1, sizeof(user));

    u->fd = clientfd;
    u->buffer_len = 0;

    // Add to front of linked list
    u->next = users;
    users = u;
    return u;
}

bool client(user *u) {
    char temp_buffer[BUFFER_SIZE + 1];

    ssize_t bytes_read = recv(u->fd, temp_buffer, BUFFER_SIZE, 0);
    // Connection closed 
    if (bytes_read <= 0) {
        // Only registered users 
        if (u->ship.remaining > 0) {  
            char msg[30];
            snprintf(msg, sizeof(msg), "GG %s\n", u->name);
            allUsers_print(msg); 
        } 
        
        disconnect_user(u);  
        return false;
    }

    // Process data byte by byte
    for (int i = 0; i < bytes_read; i++) {
        
        // Add to buffer
        u->buffer[u->buffer_len++] = temp_buffer[i];

        // Buffer overflowed, remove user
        if (u->buffer_len >= BUFFER_SIZE) {
            if (u->ship.remaining > 0) {  
                char msg[30];
                snprintf(msg, sizeof(msg), "GG %s\n", u->name);
                allUsers_print(msg);
            }
            remove_user(u);
            return false;
        }


        // Check for newline character, process line
        if (temp_buffer[i] == '\n') {

            // Null-terminate the buffer
            u->buffer[u->buffer_len - 1] = '\0'; 
            process_command(u, u->buffer);

            // Reset buffer 
            u->buffer_len = 0; 
        }
    }

    return true;
}