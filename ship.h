
//scp -r "C:\Users\kwong\OneDrive\Uoft\2nd Year\Summer\b09\A4" kwongxue@mathlab.utsc.utoronto.ca:~  
//ssh kwongxue@mathlab.utsc.utoronto.ca  
// cd /courses/courses/cscb09s25/laialber/a4/sample  
  
#ifndef SHIP_H  
#define SHIP_H  
  
#include <stdbool.h>  
  
#define BUFFER_SIZE 100  
  
typedef struct {  
    int x1, y1;  
    int x2, y2;  
    int x3, y3;  
    int x4, y4;  
    int x5, y5;  
    bool hit[5];  
    int remaining;    
} Ship;  
  
typedef struct user {  
    int fd;             // socket   
    char name[21];      // Max 20 char  
    Ship ship;  
    char buffer[BUFFER_SIZE + 1];    
    int buffer_len;            
    struct user *next;    
} user;  
  
bool process_command(user *u, const char *cmd);  
bool handle_registration(user *u, const char *name, int x, int y, char dir);  
void bomb(user *u, int x, int y);  
void user_print(user *u, const char *message);  
void disconnect_user(user *u);  
  
bool client(user *u);  
void remove_user(user *u);  
bool process_command(user *u, const char *command);  
void allUsers_print(const char *message);  
void disconnect_user(user *u);  
user *new_connection(int sfd);  
  
#endif  