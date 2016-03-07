#ifndef GLOBAL_H_
#define GLOBAL_H_

#define HOSTNAME_LEN 128
#define PATH_LEN 256
#define MAX_MSG_SIZE 257
#define MAX_MESSAGES 1000
#define SERVER 1
#define CLIENT 2
#define TRUE 't'
#define FALSE 'f'
#define BLOCK 1
#define UNBLOCK 2
#define SEND 3                      //Process must check destination IP to distinguish between send and Broadcast
#define REFRESH 4
#define LOGIN 5
#define LOGOUT 6
#define SUCCESS 100
#define BROADCAST 7
#define MESSAGE 11
#define FAILURE 404
#define PORT argv[2]
#define NOT_CONNECTED 4294967295

#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/ip.h>

#include <errno.h>
#include <sys/wait.h>
#include <signal.h>

/* Structures */
struct command_packet
{
    uint32_t commandID;
    //uint32_t sizeOfNext;
}__attribute__((packed));

/* Used for LIST and REFRESH commands */
struct list_io
{
    uint32_t ip1;
    uint32_t port1;
    uint32_t ip2;
    uint32_t port2;
    uint32_t ip3;
    uint32_t port3;
    uint32_t ip4;
    uint32_t port4;

}__attribute__((packed));

struct login_packet
{
    uint32_t ip;
    uint32_t port;

}__attribute__((packed));

/* Stores basic details of each node */
struct node{

	uint32_t ip;
	uint32_t port;
	int fd;
};

struct message{

    uint32_t ip;
    char msg[MAX_MSG_SIZE];

};

/* Used on Server side to store client details */
struct client_node{

    struct node nodeinfo;
    struct message msg_cache[1000];
    int mes_sent;
    int mes_recv;
    int last_printed;
    int pending_index;
    struct login_packet block_list[4];
    char isOnline;
};

/* Give receiver an idea of the data it is gonna get by sending a command and size info first.
The receiver acknowledges this and then the transmitter sends the payload */


/* Extern Variables */
extern int mode;				//Set to 's' if program is operating as server and 'c' otherwise
extern int my_port;				//The port no on which the current process is listening for incoming connections
extern char *my_ip;
extern char isLoggedIn;
extern char isLoggedOut;
extern struct node server_details;
extern uint32_t my_ip_long;
extern fd_set master;                          // master file descriptor list
extern fd_set read_fds;                        // temp file descriptor list for select()
extern int fdmax;                                     // maximum file descriptor number

//Client-specific
extern int client_sock;
extern struct node clients[4];

//Server-specific
extern struct client_node client_list[4];


/* Functions */
void command_process(char *argument);

void list_();
uint32_t getPublicIP();

void success(char *command);
void error_(char *command);
void end_(char *command);
void get_name(char * name,uint32_t ip);

/* Functions in client.c */
char client_login(char *ip,int serv_port);
void client_init();
int node_cmp(const void *a, const void *b);
void client_exit();
char send_message_client(char *dest_ip,char *message);
char send_broadcast(char *mesg);

/* Functions in server.c */
void serv_process_command(uint32_t command, int fd);
void handle_client_exit(int fd);
void serv_stats();
void command_blocked(char *ip_str);
char recv_message(int fd);
char send_message(uint32_t dest_ip, int fd, char* mesg);
char send_cached_messages(int fd);


/* ip2no */
uint32_t conv(char ipadr[]);
