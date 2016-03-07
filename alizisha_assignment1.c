/**
 * @alizisha_assignment1
 * @author  Alizishaan Khatri <alizisha@buffalo.edu>
 * @version 1.0
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * @section DESCRIPTION
 *
 * This contains the main function. Add further description here....
 */
#include "../include/global.h"
#include "../include/logger.h"

//Externs
int mode;
int my_port;
char *my_ip;
uint32_t my_ip_long;
char isLoggedIn;
fd_set master;                          // master file descriptor list
fd_set read_fds;                        // temp file descriptor list for select()
int fdmax;                              // maximum file descriptor number

int client_sock;
struct node server_details;


//Function
void *get_in_addr(struct sockaddr *sa);

/**
 * main function
 *
 * @param  argc Number of arguments
 * @param  argv The argument list
 * @return 0 EXIT_SUCCESS
 */
int main(int argc, char **argv)
{
	/*Init. Logger*/
	cse4589_init_log(argv[2]);

	/*Clear LOGFILE*/
	fclose(fopen(LOGFILE, "w"));

	/*Start Here*/

	/* Set port and mode information*/
	my_port=atoi(argv[2]);

	if(strcmp("s",argv[1])==0)
	{
		mode=SERVER;
		isLoggedIn=TRUE;
		//
		serv_init();
	}else if(strcmp("c",argv[1])==0)
	{
		mode=CLIENT;
		isLoggedIn=FALSE;
		client_init();
	}

    /**
     * Some of the following lines have been learnt from the programming example in Beej's guide
     */
    int listener;                           // listening socket descriptor
    int newfd;                              // newly accept()ed socket descriptor
    struct sockaddr_storage remoteaddr;     // client address
    socklen_t addrlen;

    //char buf[256];                           // buffer for client data
    struct command_packet buf;
    int nbytes;

    char remoteIP[INET6_ADDRSTRLEN];

    int yes=1;                                  // for setsockopt() SO_REUSEADDR, below
    int i, j, rv;

    struct addrinfo hints, *ai, *p;

    FD_ZERO(&master);                           // clear the master and temp sets
    FD_ZERO(&read_fds);

    // get us a socket and bind it
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0)
    {
        fprintf(stdout, "Unexpected error on selectserver: %s\n", gai_strerror(rv));
        //exit(1);
    }

    for(p = ai; p != NULL; p = p->ai_next)
    {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0)
        {
            continue;
        }

        // lose the "address already in use" error message
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0)
        {
            close(listener);
            continue;
        }

        break;
    }

    // if we got here, it means we didn't get bound
    if (p == NULL)
    {
        printf(stdout, "Unexpected error on selectserver: failed to bind\n");
        //exit(2);
    }

    freeaddrinfo(ai); // all done with this

    // listen
    if (listen(listener, 10) == -1)
    {
        fprintf(stdout,"Unexpected error on listen");
        //exit(3);
    }

    // add the listener to the master set
    FD_SET(listener, &master);
    FD_SET(0, &master);

    // keep track of the biggest file descriptor
    fdmax = listener; // so far, it's this one

    // main loop
    for(;;)
    {
        read_fds = master; // copy it
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1)
        {
            perror("select");
            exit(4);
        }

        // run through the existing connections looking for data to read
        for(i = 0; i <= fdmax; i++)
        {

            if (FD_ISSET(i, &read_fds))
            { // we got one!!

                if (i == listener)
                {
                    // handle new connections
                    addrlen = sizeof remoteaddr;
                    newfd = accept(listener,(struct sockaddr *)&remoteaddr,&addrlen);

                    if (newfd == -1)
                    {
                        printf("\nError on accept! Please retry");
                    }else
                    {
                        FD_SET(newfd, &master); // add to master set
                        if (newfd > fdmax)
                        {    // keep track of the max
                            fdmax = newfd;
                        }
                        
                    }
                }else if(i==0)
                {
                    //Handle input on stdin

                    char user_command[999];
                    fgets(user_command,998, stdin);
                    user_command[strlen(user_command)+1]='\0';
                    comm_process(user_command);

                }else
                {
                    // handle incoming data
                    if((nbytes = recv(i, (void *)&buf, sizeof(struct command_packet), 0)) <= 0)
                    {
                        // got error or connection closed by client
                        printf("Socket %d disconnected!\tnbytes = %d\n", i,nbytes);
                        if(mode==SERVER)
                        {
                            handle_client_exit(i);
                        }
                        else if(mode==CLIENT)
                        {//Modify this space for handling exiting clients during file-sharing
                            //Logout in event server terminates connection
                            if(i==client_sock)
                            {
                                printf("\nServer hung up! Please login again");
                                isLoggedIn=FALSE;
                            }

                        }

                    close(i); // bye!
                    FD_CLR(i, &master); // remove from master set
                    }else
                    {
                        // we've got incoming data
                        uint32_t command=ntohl(buf.commandID);
                      
                        if (mode==SERVER)
                        {

                            //Invoke server-side command handling
                            serv_process_command(command, i);
                            
                        }else if(mode==CLIENT)
                        {
                        //Functions for handling messages and files go here
                            //Case when we have received a message                           
                            if(command==MESSAGE)
                            {
                                //printf("\nIncoming message detected!");
                                //fflush(stdout);
                                if(recv_message(i)==FALSE)
                                    error_("RECEIVED");
                                end_("RECEIVED");
                            }
                        }
                    }
                } // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors
    } // END for(;;)--and you thought it would never end!

	return 0;
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

