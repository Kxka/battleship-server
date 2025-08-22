#include "ship.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/socket.h>

extern user *users;


bool process_command(user *u, const char *cmd) {

    // REG
    if (strncmp(cmd, "REG ", 4) == 0) {
        char name[21], dir;
        int x, y;
        if (sscanf(cmd, "REG %20s %d %d %c", name, &x, &y, &dir) == 4)
            return handle_registration(u, name, x, y, dir);
        user_print(u, "INVALID\n");
        return true;
    }

    // BOMB
    if (strncmp(cmd, "BOMB ", 5) == 0) {

        // Not registered
        if (u->ship.remaining <= 0) {
            user_print(u, "INVALID\n");
            return true;
        }
        int x, y;
        if (sscanf(cmd, "BOMB %d %d", &x, &y) == 2) {
            bomb(u, x, y);
            return true;
        }
        user_print(u, "INVALID\n");
        return true;
    }
    user_print(u, "INVALID\n");
    return true;
}


bool handle_registration(user *u, const char *name, int x, int y, char dir) {
    
    // Check name
    int len = strlen(name);
    if (len == 0 || len > 20) {
        user_print(u, "INVALID\n");
        return false;
    }
    for (int i = 0; i < len; i++) {
        char c = name[i];
        if (!isalnum((unsigned char)c) && c != '-') {
            user_print(u, "INVALID\n");
            return false;
        }
    }

    // Check ship
    if (dir == '-') {
        if (x < 2 || x > 7 || y < 0 || y > 9) {
            user_print(u, "INVALID\n");
            return false;
        }
    } else if (dir == '|') {
        if (x < 0 || x > 9 || y < 2 || y > 7) {
            user_print(u, "INVALID\n");
            return false;
        }
    } else {
        user_print(u, "INVALID\n");
        return false;
    }
    
    // check name taken
    for (user *p = users; p; p = p->next) {
        if  (p->ship.remaining > 0 && strcmp(p->name, name) == 0)  {
            user_print(u, "TAKEN\n");
            return false;
        }
    }

    // Initialize ship
    strcpy(u->name, name);
    u->ship.remaining = 5;
    memset(u->ship.hit, 0, sizeof(u->ship.hit));
    
    if (dir == '-') {
        u->ship.x1 = x - 2; u->ship.y1 = y;
        u->ship.x2 = x - 1; u->ship.y2 = y;
        u->ship.x3 = x;     u->ship.y3 = y;
        u->ship.x4 = x + 1; u->ship.y4 = y;
        u->ship.x5 = x + 2; u->ship.y5 = y;
    } else {
        u->ship.x1 = x; u->ship.y1 = y - 2;
        u->ship.x2 = x; u->ship.y2 = y - 1;
        u->ship.x3 = x; u->ship.y3 = y;
        u->ship.x4 = x; u->ship.y4 = y + 1;
        u->ship.x5 = x; u->ship.y5 = y + 2;
    }
    
    user_print(u, "WELCOME\n");
    
    // Notify all users
    char msg[30];
    snprintf(msg, sizeof(msg), "JOIN %s\n", name);
    allUsers_print(msg);
    
    return true;
}

bool if_hit(Ship *ship, int x, int y) {
    int xs[5] = {ship->x1, ship->x2, ship->x3, ship->x4, ship->x5};
    int ys[5] = {ship->y1, ship->y2, ship->y3, ship->y4, ship->y5};
    for (int i = 0; i < 5; i++) {
        // Don't hit if already damaged
        if (xs[i] == x && ys[i] == y && !ship->hit[i]) {

            // Mark hit
            ship->hit[i] = true;
            ship->remaining--;
            return true;
        }
    }
    return false;
}


void bomb(user *u, int x, int y) {
    bool hit_any = false;
    char msg[80];
    
    // Store players to disconnect after loop
    user *disconnect[20];  //assuming max 20 disconnects 
    int count = 0;

    for (user *p = users; p; p = p->next) {
        //Check hit
        if (if_hit(&p->ship, x, y)) {
            snprintf(msg, sizeof(msg), "HIT %s %d %d %s\n", u->name, x, y, p->name);
            allUsers_print(msg);
            hit_any = true;
            
            if (p->ship.remaining == 0) {
                snprintf(msg, sizeof(msg), "GG %s\n", p->name);
                allUsers_print(msg);

                user_print(p, msg);

                
                if (count < 20) {
                    disconnect[count++] = p;
                }
            }
        }
    }

    if (!hit_any) {
        snprintf(msg, sizeof(msg), "MISS %s %d %d\n", u->name, x, y);
        allUsers_print(msg);
    }

    // Disconnect players 
    for (int i = 0; i < count; i++) {
        disconnect[i]->ship.remaining = 0;  
        remove_user(disconnect[i]);
    }
}

void user_print(user *u, const char *message) {
    ssize_t len = strlen(message);
    ssize_t sent = send(u->fd, message, len, 0);

    if (sent != len) {
        disconnect_user(u);
    }
}

void allUsers_print(const char *message) {
    user *current = users;
    user *next;

    // Store disconnects 
    user *disconnected[20]; 
    int count = 0;

    while (current) {
        next = current->next;

        if (current->ship.remaining > 0) {
            ssize_t len = strlen(message);
            ssize_t sent = send(current->fd, message, len, 0);

            if (sent != len) {
                disconnected[count++] = current;
            }
        }

        current = next;
    }

    for (int i = 0; i < count; i++) {
        disconnect_user(disconnected[i]);
    }
}

void disconnect_user(user *u) {
    u->ship.remaining = 0;
    remove_user(u);
}