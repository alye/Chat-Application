/**
 *  This source file contains server-specific function implementations
 *  @author Alizishaan Khatri
 */

#include "../include/global.h"
#include "../include/logger.h"

//Function declarations
void add_node_to_list(uint32_t ip,uint32_t port,int fd);
void serv_init();
int struct_cmp(const void *a, const void *b);
void serv_process_command(uint32_t command, int fd);
void disable_node(uint32_t ip, uint32_t port, char isDeleted);
void handle_client_exit(int fd);
void send_refresh_list(int fd);
char isBlocked(uint32_t sender_ip,uint32_t receiver_ip);
void send_messages();

//Stores the records of all clients present in the system
struct client_node client_list[4];

/* Initialize the various server variables */
void serv_init(){

    struct login_packet *block_init;

    //Initialize all client list values to 0
    bzero(client_list,4*sizeof(struct client_node));

    //Populate server details
    server_details.ip=getPublicIP();
    server_details.port=my_port;

    //Initialize the client list
    int i,j;
    for(i=0;i<=3;i++)
    {
        client_list[i].isOnline=FALSE;
        client_list[i].nodeinfo.port=NOT_CONNECTED;
        client_list[i].pending_index=0;
        client_list[i].last_printed=0;
        //client_list[i].block_list=(struct login_packet *)calloc(4,sizeof(struct login_packet));

        //Initialize the port numbers of all block list nodes
        for(j=0;j<=3;j++)
        {
            block_init=(struct login_packet *)calloc(1,sizeof(struct login_packet));
            block_init->port=NOT_CONNECTED;
            block_init->ip=0;
            client_list[i].block_list[j]=*block_init;
            //client_list[i].block_list[j].port=NOT_CONNECTED;
        }
    }
}


/*  Compares two custom-structs as desired
 *  To be used by the built-in sorting function in ascending order
 */
int struct_cmp(const void *a, const void *b)
{
    uint32_t ia = ((const struct client_node *)a)->nodeinfo.port;
    uint32_t ib = ((const struct client_node *)b)->nodeinfo.port;
    //return (ia-ib);
    if(ia>ib)
        return(1);
    else
        return(-1);
}

void serv_process_command(uint32_t command, int fd)
{
    int x,y,i,j,cache_index;
    struct login_packet t,source,dest;
    struct command_packet response;


    uint32_t source_ip, source_port,dest_ip,dest_port;
    char isIPvalid=FALSE,isRelayed=FALSE;
    uint32_t ip, port;
    char msg[257];
    char *from_ip,*to_ip;
    char from_ip1[INET_ADDRSTRLEN];
    char to_ip1[INET_ADDRSTRLEN];
    struct in_addr sender, target;
    struct message *to_be_cached;

    switch(command)
    {
        case LOGIN:

            x=recv(fd,(void *)&t,sizeof t,0);

            response.commandID=htonl(FAILURE);

            if (x<=0)
            {
                //Handle Connection loss

            }else
            {
                ip=ntohl(t.ip);
                port=ntohl(t.port);
                add_node_to_list(ip,port,fd);
                send_refresh_list(fd);
                send_cached_messages(fd);
                response.commandID=htonl(SUCCESS);
                //printf("\nDEBUG: Sent list!");
                //fflush(stdout);
            }

            x=send(fd,(void *)&response,sizeof(struct command_packet),0);
            break;

        case LOGOUT:

            x=recv(fd,(void *)&t,sizeof t,0);

            if (x<=0)
            {
                //Handle Connection loss

            }else
            {
                ip=ntohl(t.ip);
                port=ntohl(t.port);
                disable_node(ip,port,FALSE);
            }
            break;


        case REFRESH:
            send_refresh_list(fd);
            break;

        case BLOCK:

            //Read source and destination info from client
            x=recv(fd,(void *)&source,sizeof source,0);
            y=recv(fd,(void *)&dest,sizeof dest,0);

            source_ip=ntohl(source.ip);
            source_port=ntohl(source.port);

            dest_ip=ntohl(dest.ip);


            //Loop through server list to find given IP
            for(i=0,isIPvalid=FALSE;i<=3;i++)
            {
                if(client_list[i].nodeinfo.ip==dest_ip && client_list[i].nodeinfo.port!=NOT_CONNECTED)
                {
                    isIPvalid=TRUE;
                    dest_port=client_list[i].nodeinfo.port;
                    break;
                }
            }

            //Add dest_ip to the source node's block_list
            for(i=0;i<=3;i++)
            {
                if(client_list[i].nodeinfo.ip==source_ip && isIPvalid==TRUE)
                {

                    //add dest_ip to end block list
                    client_list[i].block_list[0].ip=dest_ip;
                    client_list[i].block_list[0].port=dest_port;
                    //Sort the blocked clients in ascending order of listening port numbers
                    //struct login_packet *sorter;
                    //sorter=client_list[i].block_list;
                    qsort((void *)&client_list[i].block_list[0],4,sizeof(struct login_packet),node_cmp);

                    /*
                    for(j=0;j<=3;j++)
                    if(client_list[i].block_list[j].ip==0)
                        {
                            client_list[i].block_list[j].ip=dest_ip;
                            client_list[i].block_list[j].ip=dest_port;
                            break;
                        }
                    */

                    break;
                }
            }

            //Respond back to the calling host
            if(isIPvalid==TRUE){
                response.commandID=htonl(SUCCESS);
            }else{
                response.commandID=htonl(FAILURE);
            }

            //Write final response to socket
            x=send(fd,(void *)&response,sizeof(struct command_packet),0);
            //Check 'x' for communication failure!

            break;

        case UNBLOCK:

                //Read source and destination info from client
            x=recv(fd,(void *)&source,sizeof source,0);
            y=recv(fd,(void *)&dest,sizeof dest,0);

            source_ip=ntohl(source.ip);
            source_port=ntohl(source.port);

            dest_ip=ntohl(dest.ip);


            for(i=0,isIPvalid=FALSE;i<=3;i++)
            {
                //printf("\n i : %d",i);
                //fflush(stdout);

                if(client_list[i].nodeinfo.ip==source_ip)
                {
                    for(j=0;j<=3;j++){
                    //printf("\n j : %d",j);
                    //fflush(stdout);
                        if(client_list[i].block_list[j].ip==dest_ip)
                        {
                            isIPvalid=TRUE;
                            //Disable the current entry
                            client_list[i].block_list[j].ip=0;
                            client_list[i].block_list[j].port=NOT_CONNECTED;
                            //Sort the blocked list in ascending order of listening port numbers
                            //qsort((void *)&client_list[i].block_list,4,sizeof(client_list[i].block_list),node_cmp);
                            qsort((void *)&(client_list[i].block_list[0]),4,
                                sizeof(struct login_packet),node_cmp);
                            break;

                        }}
                }
                }

            if(isIPvalid==TRUE)
                response.commandID=htonl(SUCCESS);
            else
                response.commandID=htonl(FAILURE);

            //Send back final response to the client
            x=send(fd,(void *)&response,sizeof(struct command_packet),0);
            //Check 'x' for communication failure!
            break;

        case MESSAGE:
            x=recv(fd,(void *)&source,sizeof source,0);
            y=recv(fd,(void *)&dest,sizeof dest,0);
            //Check x and y for network errors

            bzero(msg,257);
            x=recv(fd,msg,256*sizeof(char),0);
            //Check x status

            source_ip=ntohl(source.ip);
            source_port=ntohl(source.port);

            dest_ip=ntohl(dest.ip);


            int dest_index,send_idx;
            response.commandID=htonl(FAILURE);

            for(i=0,dest_index=-1,send_idx=-1;i<=3;i++)
                if(client_list[i].nodeinfo.ip==dest_ip && client_list[i].nodeinfo.port!=NOT_CONNECTED)
                {
                    dest_index=i;
                    response.commandID=htonl(SUCCESS);
                    //continue;
                }else if(client_list[i].nodeinfo.ip==source_ip)
                {
                    send_idx=i;
                }

            if(ntohl(response.commandID)==SUCCESS)
                client_list[send_idx].mes_sent++;

            x=send(fd,(void *)&response,sizeof(struct command_packet),0);
            //Check 'x' for communication failure!


            //Dispatch the message along the correct channel
            if(dest_index>=0 && isBlocked(source_ip,dest_ip)!=TRUE)
            {
                if(client_list[dest_index].isOnline==TRUE)
                {
                    //printf("\nDest-index is:%d",dest_index);
                    //fflush(stdout);
                    if(send_message(source_ip,client_list[dest_index].nodeinfo.fd,msg)==TRUE)
                    {
                        success("RELAYED");

                        printf("\nA");
                        fflush(stdout);

                        memset((void *)&sender,0,sizeof(struct in_addr));
                        printf("\nb");
                        fflush(stdout);


                        sender.s_addr=source_ip;

                        inet_ntop(AF_INET,&(sender),from_ip1,INET_ADDRSTRLEN);


                        memset((void *)&target,0,sizeof(struct in_addr));

                        target.s_addr=dest_ip;
                        inet_ntop(AF_INET,&(target),to_ip1,INET_ADDRSTRLEN);


                        cse4589_print_and_log("msg from:%s, to:%s\n[msg]:%s\n", from_ip1, to_ip1, msg);

                    }
                    else
                        error_("RELAYED");

                    //cse4589_print_and_log("msg from:%s, to:%s\n[msg]:%s\n", from-client-ip, to-client-ip, msg);
                }
                else //Store for later transmission
                {
                    //int msg_index=client_list[dest_index].pending_index;
                    //strcpy(&client_list[dest_index].msg_cache[msg_index],msg);
                    //client_list[dest_index].pending_index++;

                    success("RELAYED");
                    memset((void *)&sender,0,sizeof(struct in_addr));
                    sender.s_addr=source_ip;
                    //from_ip=inet_ntoa(sender);

                    memset((void *)&target,0,sizeof(struct in_addr));
                    target.s_addr=dest_ip;
                    //to_ip=inet_ntoa(target);

                    cse4589_print_and_log("msg from:%s, to:%s\n[msg]:%s\n", inet_ntoa(sender), inet_ntoa(target), msg);
                    //client_list[dest_index].mes_recv++;

                    to_be_cached=calloc(1,sizeof(struct message));
                    to_be_cached->ip=source_ip;
                    //to_be_cached.msg=msg;

                    for(i=0;i<=256;i++)
                        to_be_cached->msg[i]=msg[i];

                    //Add to cache and increment cache index
                    cache_index=client_list[dest_index].pending_index;
                    client_list[dest_index].msg_cache[cache_index]=*to_be_cached;
                    client_list[dest_index].pending_index=cache_index+1;

                    printf("\nDEBUG: Message successfully saved to cache!\nNext cache index is: %d",cache_index);
                    fflush(stdout);
                }
                end_("RELAYED");
            }

            break;

        case BROADCAST:

            x=recv(fd,(void *)&source,sizeof(struct login_packet),0);
            //Check x and y for network errors
            bzero(msg,257);
            y=recv(fd,msg,256*sizeof(char),0);

            memset((void *)&sender,0,sizeof(struct in_addr));
            sender.s_addr=(long)ntohl(source.ip);
            from_ip=inet_ntoa(sender);

            memset((void *)&response,0,sizeof(struct command_packet));
            response.commandID=htonl(SUCCESS);

            for(i=0;i<=3;i++)
            {
                printf("\ni: %d",i);
                fflush(stdout);

                if(client_list[i].nodeinfo.ip!=(uint32_t)sender.s_addr && client_list[i].nodeinfo.port!=NOT_CONNECTED
                    && isBlocked((uint32_t)sender.s_addr,client_list[i].nodeinfo.ip)==FALSE)
                {
                    isRelayed=TRUE;
                    //Send message to respective server if online or cache otherwise
                    if(client_list[i].isOnline==TRUE)
                    {
                        if(send_message((uint32_t)sender.s_addr, client_list[i].nodeinfo.fd,msg)==FALSE)
                            response.commandID=htonl(FAILURE);
                    }else
                    {
                        to_be_cached=calloc(1,sizeof(struct message));
                        to_be_cached->ip=source_ip;
                        //to_be_cached.msg=msg;

                        for(i=0;i<=256;i++)
                            to_be_cached->msg[i]=msg[i];

                        //Add to cache and increment cache index
                        cache_index=client_list[dest_index].pending_index;
                        client_list[dest_index].msg_cache[cache_index]=*to_be_cached;
                        client_list[dest_index].pending_index=cache_index+1;

                        printf("\nDEBUG: Message successfully saved to cache!\nNext cache index is: %d",cache_index);
                        fflush(stdout);
                    }

                }
                else if(client_list[i].nodeinfo.ip==sender.s_addr)
                {
                    client_list[i].mes_sent++;
                }
            }

            if(isRelayed==TRUE)
            {
                success("RELAYED");
                to_ip="255.255.255.255";
                cse4589_print_and_log("msg from:%s, to:%s\n[msg]:%s\n", inet_ntoa(sender),to_ip, msg);
                end_("RELAYED");

            }

            //Send back response to client
            x=send(fd,(void *)&response,sizeof(struct command_packet),0);
            //Check x for errors, if any
    }
}

/*  Add a new node to the list
 *
 *  @param ip The IP address of the node to be added
 *  @param port The listening port # of the node to be added
 */
void add_node_to_list(uint32_t ip,uint32_t port,int fd)
{
    int i,j,found_int=4;

    for(i=0;i<=3;i++)
    {
        if(client_list[i].nodeinfo.ip==ip && client_list[i].nodeinfo.port==port)
        {
            found_int=i;
            client_list[i].isOnline=TRUE;
            //client_list[i].fd=fd;
            break;
        }
    }

    if(found_int==4)
    {
        //Clear the last node of its previous value (if any)
        //memset((void*)&client_list[3],0,sizeof(struct client_node));
        client_list[3].mes_recv=0;
        client_list[3].mes_sent=0;

        for(j=0;j<=3;j++)
            client_list[3].block_list[j].port=NOT_CONNECTED;

        //Initially add next value to the last position
        client_list[3].isOnline=TRUE;
        client_list[3].nodeinfo.ip=ip;
        client_list[3].nodeinfo.port=port;
        client_list[3].nodeinfo.fd=fd;

    }
    //Sort the array in ascending order of listening port numbers
    qsort(client_list,4,sizeof(struct client_node),struct_cmp);
}

/*  Removes/disables a node in the list
 *
 *  @param ip The IP address of the node to be deleted
 *  @param port The listening port # of the node to be deleted
 *  @param isDeleted Deletes the entire record from list if set to TRUE
 */
void disable_node(uint32_t ip, uint32_t port, char isDeleted)
{
    int i,j;

    /*

    printf("\nOn entering disable node!");
    fflush(stdout);
    for(i=0,j=1;i<=3;i++,j++)
        printf("\n%d\tIP: %d\tPort: %d",j,client_list[i].nodeinfo.ip,client_list[i].nodeinfo.port);
    */

    for(i=0;i<=3;i++)
    {
        if(client_list[i].nodeinfo.ip==ip && client_list[i].nodeinfo.port==port)
        {
            //printf("\nFrom disable node:\ti: %d\tIP: %d\tPort: %d",i,client_list[i].nodeinfo.ip,client_list[i].nodeinfo.port);

            if(isDeleted==TRUE)
            {
                client_list[i].nodeinfo.port=NOT_CONNECTED;
                for(j=0;j<=3;j++)
                    client_list[i].block_list[j].port=NOT_CONNECTED;
            }

            client_list[i].isOnline=FALSE;

            //Set port value to NOT_CONNECTED
            //client_list[i].nodeinfo.port=NOT_CONNECTED;
            break;
        }
    }

    /*

    for(i=0,j=1;i<=3;i++,j++)
        printf("\n%d\tIP: %d\tPort: %d",j,client_list[i].nodeinfo.ip,client_list[i].nodeinfo.port);

    */

    //Sort the array in ascending order of listening port numbers
    qsort(client_list,4,sizeof(struct client_node),struct_cmp);

    /*
    printf("\nAfter Sorting");
    for(i=0,j=1;i<=3;i++,j++)
        printf("\n%d\tIP: %d\tPort: %d",j,client_list[i].nodeinfo.ip,client_list[i].nodeinfo.port);
    fflush(stdout);
    */

}


/* Sends the latest copy of the list to the specified client
 * @param1 fd Specify file descriptor of the destination node
 */
void send_refresh_list(int fd)
{
    struct list_io toBeSent;
    int i,online_state;

    toBeSent.ip1=htonl(client_list[0].nodeinfo.ip);
    if(client_list[0].isOnline==TRUE)
        toBeSent.port1=htonl(client_list[0].nodeinfo.port);
    else
        toBeSent.port1=htonl(NOT_CONNECTED);

    toBeSent.ip2=htonl(client_list[1].nodeinfo.ip);
    if(client_list[1].isOnline==TRUE)
        toBeSent.port2=htonl(client_list[1].nodeinfo.port);
    else
        toBeSent.port2=htonl(NOT_CONNECTED);

    toBeSent.ip3=htonl(client_list[2].nodeinfo.ip);
    if(client_list[2].isOnline==TRUE)
        toBeSent.port3=htonl(client_list[2].nodeinfo.port);
    else
        toBeSent.port3=htonl(NOT_CONNECTED);

    toBeSent.ip4=htonl(client_list[3].nodeinfo.ip);
    if(client_list[3].isOnline==TRUE)
        toBeSent.port4=htonl(client_list[3].nodeinfo.port);
    else
        toBeSent.port4=htonl(NOT_CONNECTED);



    /* Debug outputs

    int i;
    for(i=0;i<=3;i++)
        printf("\nList : %d,\tip : %d\tPort: %d",i,client_list[i].nodeinfo.ip,client_list[i].nodeinfo.port);

    printf("\nIP1 : %d\tPort1 : %d",toBeSent.ip1,toBeSent.port1);
    printf("\nIP1 : %d\tPort1 : %d",toBeSent.ip2,toBeSent.port2);
    printf("\nIP1 : %d\tPort1 : %d",toBeSent.ip3,toBeSent.port3);
    printf("\nIP1 : %d\tPort1 : %d",toBeSent.ip4,toBeSent.port4);

    */

    struct command_packet p;
    p.commandID=htonl(REFRESH);

    //Send updated list to this node
    send(fd,(void *)&p,sizeof(struct command_packet),0);
    send(fd,(void *)&toBeSent,sizeof(struct list_io),0);

}

/*
 *  Function to check if sender is blocked by receiver
 *
 *  @param sender_ip The IP address of sending node in uint32 form
 *  @param receiver_ip The IP address of target node in uint32 form
 *
 *  @return TRUE or FALSE
 */
char isBlocked(uint32_t sender_ip,uint32_t receiver_ip)
{
    int i=0;
    for(i=0;i<=3;i++)
    {
        if(client_list[i].nodeinfo.ip==receiver_ip)
        {
            int j;
            for(j=0;j<=3;j++)
            {
                if(client_list[i].block_list[j].ip==sender_ip)
                {
                    return TRUE;
                }
            }
            return FALSE;
        }
    }
    printf("\nDEBUG: Receiver not found!");
    fflush(stdout);
    return FALSE;
}

void send_messages()
{
    int i;
    //Iterate through all nodes in list
    for(i=0;i<=3;i++)
    {
        if(client_list[i].isOnline==TRUE)
        {
            //Loop through message bank and send all stored messages!
        }
    }

}

char send_message(uint32_t dest_ip, int fd, char* mesg)
{
    struct command_packet cmd_header;
    struct login_packet details;
    int status_code,i;

    cmd_header.commandID=htonl(MESSAGE);

    //printf("\nDEBUG 1");
    //fflush(stdout);

    status_code=send(fd,(void *)&cmd_header, sizeof(struct command_packet), 0);
    if(status_code<=0)
    {
        printf("\nDEBUG: Error sending message command_packet");
        printf("\nDEBUg: fd is : %d",fd);
        fflush(stdout);
        return(FALSE);
    }

    cmd_header.commandID=0;

    //Wait for client to start!
    status_code=recv(fd,(void *)&cmd_header, sizeof(struct command_packet), 0);
    if(status_code<=0)
    {
        printf("\nDEBUG: Error reading message command_packet");
        fflush(stdout);
        return(FALSE);
    }


    //printf("\nDEBUG 2. Status code: %d",status_code);
    //fflush(stdout);

    //Start communication with client
    details.ip=htonl(dest_ip);
    details.port=htonl(NOT_CONNECTED); //Dummy Value

    status_code=send(fd, (void *)&details , sizeof(struct login_packet), 0);

    if(status_code<0)
    {
        printf("\nDEBUG: Error sending message login_packet");
        fflush(stdout);
        return(FALSE);
    }

    //printf("\nDEBUG 3. Status code: %d",status_code);
    //fflush(stdout);

    //Send Payload
    char messages[256];
    //messages[256]='\0';
    strncpy(messages,mesg,256);

    status_code=send(fd,messages,256*sizeof(char),0);
    if(status_code<0)
    {
        printf("\nDEBUG: Error sending message payload");
        fflush(stdout);
        return(FALSE);
    }

    //printf("\nDEBUG 4. Status code: %d",status_code);
    //fflush(stdout);

    //Since we've reached here, everything went on smoothly

    //Increment receive messages
    for(i=0;i<=3;i++)
        if(client_list[i].nodeinfo.fd==fd)
            client_list[i].mes_recv=client_list[i].mes_recv+1;


    return(TRUE);

}

/*
 *  Updates client list when a particular connection ends
 *  @param1 The file descriptor of the connection
 */
void handle_client_exit(int fd)
{
    int i,j;
    uint32_t ip;
    uint32_t port;

    for(i=0;i<=3;i++)
    {
        if(client_list[i].nodeinfo.fd==fd)
        {
            ip=client_list[i].nodeinfo.ip;
            port=client_list[i].nodeinfo.port;
            //printf("\nFrom handle_client_exit:\tIP: %d\tPort: %d",ip,port);
            fflush(stdout);
            break;
        }
    }

    /*

    printf("\nHandle Client Exit: Full list");
    for(i=0,j=1;i<=3;i++,j++)
        printf("\n%d\tIP: %d\tPort: %d",j,client_list[i].nodeinfo.ip,client_list[i].nodeinfo.port);*/
    disable_node(ip,port,TRUE);
}

void serv_stats()
{
    int i,list_id,num_msg_sent, num_msg_rcv;
    char status[10];

    //char *host_name=(char *)calloc(50,sizeof(char));
    char host_name[50];

    for(i=0,list_id=1;i<=3;i++)
    {
        if(client_list[i].nodeinfo.port!=NOT_CONNECTED)
        {
            memset(host_name,'\0',50);
            memset(status,'\0',10);

            get_name(host_name,client_list[i].nodeinfo.ip);

            if(client_list[i].isOnline==TRUE){
                strcpy(status,"online");
            }else{
                strcpy(status,"offline");
            }

            cse4589_print_and_log("%-5d%-35s%-8d%-8d%-8s\n", list_id++, host_name,
                    client_list[i].mes_sent, client_list[i].mes_recv, status);
        }
    }

    //free(host_name);
}

void command_blocked(char *ip_str)
{
    //Extract IP address in integer form
    /*
    struct sockaddr_in server;
    memset((struct sockaddr_in *)&server,0,sizeof(struct sockaddr_in));

    if(inet_pton(AF_INET,ip_str,&(server.sin_addr))<0)
    {
        error_("BLOCKED");
        return;
    }*/

    uint32_t ip;
    char s[INET_ADDRSTRLEN];
    strcpy(s,ip_str);
    ip=ntohl(conv(s));
    //(uint32_t)server.sin_addr.s_addr;
    char isIPValid=FALSE;

    char host_name[50];
    char ip_addr[INET_ADDRSTRLEN];

    int i,j,list_id;

    //Iterate through all nodes
    for(i=0;i<=3;i++)
    {
        if(client_list[i].nodeinfo.ip==ip)
        {
            //The entered IP exists in our list and is valid
            isIPValid=TRUE;
            success("BLOCKED");
            //Print block list for this client
            for(j=0,list_id=1;j<=3;j++)
                if(client_list[i].block_list[j].port!=NOT_CONNECTED)
                {
                    memset(host_name,'\0',50);
                    memset(ip_addr,'\0',INET_ADDRSTRLEN);
                    //printf("\nDEBUG: 2");
                    inet_ntop(AF_INET,&(client_list[i].block_list[j].ip),ip_addr,INET_ADDRSTRLEN);
                    //printf("\nDEBUG: 3");
                    get_name(host_name,client_list[i].block_list[j].ip);
                    cse4589_print_and_log("%-5d%-35s%-20s%-8d\n", list_id++,
                    host_name, ip_addr, client_list[i].block_list[j].port);
                }
        //return;
        }
    }

    if(isIPValid==FALSE)
    {
        error_("BLOCKED");
        return;
    }
}

char send_cached_messages(int fd)
{
    int i,j,strt;
    uint32_t sender_ip;
    char *msg;

    char status=TRUE;

    for(i=0;i<=3;i++)
        if(client_list[i].nodeinfo.fd==fd)
        {
            strt=client_list[i].last_printed;
            for(j=strt;j<client_list[i].pending_index;j++)
            {
                sender_ip=client_list[i].msg_cache[j].ip;
                //strcpy(msg,client_list[i].msg_cache[j].msg);
                msg=client_list[i].msg_cache[j].msg;

                if(send_message(sender_ip,fd,msg)==FALSE)
                {   //If sending fails, break loop and return response
                    status=FALSE;
                    break;
                }
            }
            client_list[i].last_printed=j;
            break;
        }

    return(status);
}
