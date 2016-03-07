/**
 *  This source file contains client-specific function implementations
 *  @author Alizishaan Khatri
 */

#include "../include/global.h"
#include "../include/logger.h"

struct node clients[4];
struct login_packet block_list[4];
char isLoggedOut;

void client_init()
{
    //Initialize client and blocked node lists with zero values
    memset((void *)&clients,0,sizeof(clients));
    memset((void *)&block_list,0,sizeof(block_list));

    isLoggedOut=FALSE;

    //Set all listening port numbers to NOT_CONNECTED
    int i;
    for(i=0;i<=3;i++)
    {
        clients[i].port=NOT_CONNECTED;
        block_list[i].port=NOT_CONNECTED;
    }

    //Record own IP address
    my_ip_long=getPublicIP();
}

char client_block(char *ip_str)
{

    //Extract IP address in integer form  
    int i,send_status;
   
    char s[INET_ADDRSTRLEN];
    strcpy(s,ip_str);
    uint32_t ip;
    ip=ntohl(conv(s));
  
    //Check if client is trying to block self
    if(ip==my_ip_long)
    {
        printf("\nDEBUG: 2");
        fflush(stdout);
        return FALSE;
    }

    //Check if client is already in block_list
    for(i=0;i<=3;i++)
    {
        if(block_list[i].ip==ip && block_list[i].port!=NOT_CONNECTED)
        {
            printf("\ni = %d",i);
            printf("\nDEBUG: 3");
            fflush(stdout);
            return FALSE;
        }

    }

    //Send command to server
    struct command_packet p_block;
    p_block.commandID=htonl(BLOCK);

    send_status=send(client_sock,(void *)&p_block,sizeof(struct command_packet),0);

    if(send_status==0)
    {
        //Connection termination event
        client_exit();
        printf("\nDEBUG: 5");
        fflush(stdout);

        return FALSE;
    }else if(send_status<0)
    {
        //Send error
        printf("\nDEBUG: 6");
        fflush(stdout);
        return FALSE;
        return FALSE;
    }

    //Build source and destination detail packets
    struct login_packet self,dest;
    memset((void *)&self,0,sizeof(struct login_packet));
    memset((void *)&dest,0,sizeof(struct login_packet));

    self.ip=htonl(my_ip_long);
    self.port=htonl(my_port);

    dest.ip=htonl(ip);
    dest.port=htonl(NOT_CONNECTED);

    //Send source details to server
    send_status=send(client_sock,(void *)&self,sizeof(struct login_packet),0);

    if(send_status==0)
    {
        //Connection termination event
        client_exit();
        printf("\nDEBUG: 7");
        fflush(stdout);

        return FALSE;
    }else if(send_status<0)
    {
        //Send error
        printf("\nDEBUG: 8");
        fflush(stdout);

        return FALSE;
    }

    //Send destination details to server
    send_status=send(client_sock,(void *)&dest,sizeof(struct login_packet),0);

    if(send_status==0)
    {
        //Connection termination event
        client_exit();
        printf("\nDEBUG: 9");
        fflush(stdout);
        return FALSE;
    }else if(send_status<0)
    {
        //Send error
        printf("\nDEBUG: 10");
        fflush(stdout);
        return FALSE;
    }

    struct command_packet serv_response;
    uint32_t response_code;

    int recv_status;
    recv_status=recv(client_sock,(void *)&serv_response,sizeof serv_response,0);

    if(recv_status==0)
    {
        //Connection termination event
        client_exit();       
        return FALSE;
    }else if(recv_status<0)
    {
        //Send error
        return FALSE;
    }

    response_code=ntohl(serv_response.commandID);

    if(response_code==SUCCESS)
    {
        block_list[0].ip=ip;
        block_list[0].port=0;
        qsort((void *)&block_list,4,sizeof(struct login_packet),node_cmp);
        return TRUE;
    }else
    {
        printf("\nDEBUG: 13");
        fflush(stdout);
        return FALSE;
    }
}

char client_unblock(char *ip)
{
    //Convert IP to numeric form
    char ip_arr[INET_ADDRSTRLEN];
    strcpy(ip_arr,ip);
    uint32_t ip_val=ntohl(conv(ip_arr));

    int send_status,i,found_index=4;

    //Check if entered IP exists in local block list
    for(i=0;i<=3;i++)
        if(block_list[i].ip==ip_val)
        {
            found_index=i;
            break;
        }

    if(found_index==4)
    {
        return FALSE;
    }

    //Remove IP from local block list

    //Update server about unblocking
    struct command_packet ublock;
    ublock.commandID=htonl(UNBLOCK);

    struct login_packet my_inf, target_info;

    my_inf.ip=htonl(getPublicIP());
    my_inf.port=htonl((uint32_t)my_port);

    target_info.ip=htonl(ip_val);
    target_info.port=htonl(0);              //Dummy value. We don't really care about this field

    //Send out structs
    send_status=send(client_sock,(void *)&ublock,sizeof(struct command_packet),0);
    if(send_status<=0)
    {
        printf("\nDEBUG 1");
        fflush(stdout);
        return FALSE;
    }

    send_status=send(client_sock,(void *)&my_inf,sizeof(struct login_packet),0);
    if(send_status<=0)
    {
        printf("\nDEBUG 2");
        fflush(stdout);
        return FALSE;
    }

    send_status=send(client_sock,(void *)&target_info,sizeof(struct login_packet),0);
    if(send_status<=0)
    {
        printf("\nDEBUG 3");
        fflush(stdout);
        return FALSE;
    }

    send_status=recv(client_sock,(void *)&ublock,sizeof(struct command_packet),0);
    if(send_status<=0)
    {
        printf("\nDEBUG 4");
        fflush(stdout);
        return FALSE;
    }

    uint32_t response=ntohl(ublock.commandID);

    if(response!=SUCCESS)
    {
        printf("\nDEBUG 5");
        fflush(stdout);
        return FALSE;
    }

    //Remove node from local block list
    block_list[found_index].ip=0;
    block_list[found_index].port=NOT_CONNECTED;
    qsort((void *)&block_list,4,sizeof(struct login_packet),node_cmp);
    return TRUE;
}

    
char client_login(char *ip,int serv_port){

    //Set-up server struct
    struct sockaddr_in server;
    memset((struct sockaddr_in *)&server,0,sizeof(struct sockaddr_in));

    server.sin_port=htons(serv_port);
    server.sin_family=AF_INET;  //server.sin_addr.s_addr=

    inet_pton(AF_INET,ip,(void *)&server.sin_addr.s_addr);

    if(isLoggedOut==FALSE)
    {
        //Create client socket
        client_sock=socket(AF_INET, SOCK_STREAM, 0);

        if(client_sock<3)
        {
            printf("DEBUG: Error creating client socket!");
            return FALSE;
        }
        if (connect(client_sock,(struct sockaddr *)&server,sizeof(struct sockaddr)) < 0)
        {
            printf("\nDEBUG: ERROR connecting!");
            return FALSE;
        }

    }   

    struct command_packet abc;


    abc.commandID=htonl(LOGIN);
    //abc.sizeOfNext=htonl(32);

    if(send(client_sock,(void *)&abc,sizeof(struct command_packet),0)<0)
    {
        printf("\nDEBUG: ERROR writing command to socket!");
        return FALSE;
    }

    //Send IP address and port information to server
    struct login_packet login_info;
    login_info.ip=htonl(getPublicIP());
    login_info.port=htonl((uint32_t)my_port);

    if(send(client_sock,(void *)&login_info,sizeof(struct login_packet),0)<=0)
    {
        printf("\nDEBUG: ERROR writing login_info to socket!");
        return FALSE;
    }

    //Update local list as per response from server
    struct list_io recv_list;

    if(recv(client_sock,(void *)&abc,sizeof abc,0)<=0)
    {
        printf("\nDEBUG: ERROR reading command packet!");
        return FALSE;
    }

    if(recv(client_sock,(void *)&recv_list,sizeof recv_list,0)<=0)
    {
        printf("\nDEBUG: ERROR reading list!");
        return FALSE;
    }

    clients[0].ip=ntohl(recv_list.ip1);
    clients[1].ip=ntohl(recv_list.ip2);
    clients[2].ip=ntohl(recv_list.ip3);
    clients[3].ip=ntohl(recv_list.ip4);

    clients[0].port=ntohl(recv_list.port1);
    clients[1].port=ntohl(recv_list.port2);
    clients[2].port=ntohl(recv_list.port3);
    clients[3].port=ntohl(recv_list.port4);

    memset((void *)&abc,0,sizeof(struct command_packet));

    if(recv(client_sock,(void *)&abc,sizeof(struct command_packet),0)<=0)
    {
        return FALSE;
    }

    while(abc.commandID==htonl(MESSAGE))
    {
        //Receive updated message from server
        if(recv_message(client_sock)==FALSE)
            error_("RECEIVED");
        end_("RECEIVED");

        //Receive next message or end result of login
        if(recv(client_sock,(void *)&abc,sizeof(struct command_packet),0)<=0)
        {
            return FALSE;
        }
    }

    if(abc.commandID==htonl(SUCCESS))
        return TRUE;
    else
        return FALSE;
}

char client_logout()
{
    struct command_packet logout;
    struct login_packet my_info;
    int status;

    logout.commandID=htonl(LOGOUT);

    status=send(client_sock,(void *)&logout,sizeof(struct command_packet),0);
    if(status<=0)
    {
        if(status==0)
        {
            //Connection ended!
        }
        printf("\nSending failed!");
        return FALSE;
    }

    //Send IP address and port information to server
    my_info.ip=htonl(getPublicIP());
    my_info.port=htonl((uint32_t)my_port);

    if(send(client_sock,(void *)&my_info,sizeof(struct login_packet),0)<0)
    {
        printf("\nDEBUG: ERROR writing login_info to socket!");
        return FALSE;
    }


    return(TRUE);
}

char client_refresh()
{
    int status,i;
    struct command_packet refresh;
    struct list_io recv_list;
    refresh.commandID=htonl(REFRESH);
    uint32_t online_state;

    status=send(client_sock,(void *)&refresh,sizeof(struct command_packet),0);
    if(status<=0)
    {
        if(status==0)
        {
            //Connection ended!
        }
        printf("\nSending failed!");
        return FALSE;
    }

    //Server acknowledges by sending a similar packet back
    if(recv(client_sock,(void *)&refresh,sizeof refresh,0)<0)
    {
        printf("\nDEBUG: ERROR reading command packet!");
        return FALSE;
    }

    //Get updated list from server
    status=recv(client_sock,(void *)&recv_list,sizeof(recv_list),0);
    if(status<=0)
    {
        if(status==0)
        {
            //Connection ended!
        }
        printf("\nReceive error!");
        return FALSE;
    }

    clients[0].ip=ntohl(recv_list.ip1);
    clients[1].ip=ntohl(recv_list.ip2);
    clients[2].ip=ntohl(recv_list.ip3);
    clients[3].ip=ntohl(recv_list.ip4);

    clients[0].port=ntohl(recv_list.port1);
    clients[1].port=ntohl(recv_list.port2);
    clients[2].port=ntohl(recv_list.port3);
    clients[3].port=ntohl(recv_list.port4);

    //fflush(client_sock);
    return(TRUE);
}

void client_exit()
{
    close(client_sock);
}

char recv_message(int fd)
{
    struct login_packet source;
    char message[257];
    uint32_t sender_ip,sender_port;
    struct command_packet cmd_header;
    int status_code;

    //Acknowledge server by echoing command_packet
    cmd_header.commandID=htonl(MESSAGE);

    status_code=send(fd,(void *)&cmd_header, sizeof(struct command_packet), 0);
    if(status_code<0)
    {
        printf("\nDEBUG: Error sending message command_packet");
        fflush(stdout);
        return(FALSE);
    }

    //Read data from server

    if(recv(fd,(void *)&source,sizeof source,0)<=0)
    {
        printf("\nDEBUG: ERROR reading source IP packet!");
        return FALSE;
    }

    sender_ip=ntohl(source.ip);
    sender_port=ntohl(source.port);

    //recv(sockfd1,buf1,sizeof(buf1),0);
    if(recv(fd,(void *)&message,256 * sizeof(char),0)<=0)
    {
        printf("\nDEBUG: ERROR reading message packet!");
        return FALSE;
    }

    //Insert null terminator
    message[256]='\0';

    struct in_addr ip_conv;
    long long_ip=ntohl(source.ip);
    ip_conv.s_addr=long_ip;
    char *send_ip=inet_ntoa(ip_conv);

    success("RECEIVED");
    cse4589_print_and_log("msg from:%s\n[msg]:%s\n", send_ip, message);

    //free(send_ip);
    return TRUE;
}

char send_message_client(char *dest_ip,char *message)
{

    char ip[INET_ADDRSTRLEN];
    char msg[256];
    bzero(msg,sizeof(msg));

    strcpy(ip,dest_ip);
    strncpy(msg,message,256);

    int status;

    struct command_packet cmd;
    struct login_packet source,dest;

    cmd.commandID=htonl(MESSAGE);

    source.ip=htonl(getPublicIP());
    source.port=htonl(my_port);

    dest.ip=conv(ip);
    dest.port=htonl(NOT_CONNECTED);             //Dummy Value

    //Send command header
    status=send(client_sock,(void *)&cmd,sizeof(struct command_packet),0);
    if(status<=0)
    {
        return FALSE;
    }
    
    //Send sender details
    status=send(client_sock,(void *)&source,sizeof(struct login_packet),0);
    if(status<=0)
    {
        return FALSE;
    }

    //Send destination details
    status=send(client_sock,(void *)&dest,sizeof(struct login_packet),0);
    if(status<=0)
    {
        return FALSE;
    }

    //Send the payload
    status=send(client_sock,(void *)&msg,256*sizeof(char),0);
    if(status<=0)
    {
        printf("\nDEBUG: Error sending message payload");
        fflush(stdout);
        return(FALSE);
    }

    printf("\n5");
    fflush(stdout);

    status=recv(client_sock,(void *)&cmd,sizeof(struct command_packet),0);
    if(status<0)
    {
        printf("\nDEBUG: Error sending message payload");
        fflush(stdout);
        return(FALSE);
    }

    printf("\n6");
    fflush(stdout);

    uint32_t response=ntohl(cmd.commandID);

    if(response==SUCCESS)
        return(TRUE);
    else
        return FALSE;
}

/* Initiates a broadcast by sending the server a message 
 * @param mesg The message to be sent in string form
 * @return TRUE if successful <br> FALSE otherwise
 */
char send_broadcast(char *mesg)
{
    int status_code;
    char msg[256];
    bzero(msg,sizeof(msg));
    strcpy(msg,mesg);

    /* Build I/O packets */
    struct command_packet comm_pck;
    struct login_packet source_info;

    comm_pck.commandID=htonl(BROADCAST);
    source_info.ip=htonl(getPublicIP());
    source_info.port=htonl(my_port);

    status_code=send(client_sock,(void *)&comm_pck,sizeof(struct command_packet),0);
    if(status_code<=0)
        return(FALSE);

    status_code=send(client_sock,(void *)&source_info,sizeof(struct login_packet),0);
    if(status_code<=0)
        return(FALSE);

    //Send payload
    status_code=send(client_sock, msg,256,0);
    if(status_code<=0)
        return(FALSE);

    memset((void *)&comm_pck,0,sizeof(struct command_packet));

    //Receive response from server
    status_code=recv(client_sock,(void *)&comm_pck,sizeof(struct command_packet),0);
    if(status_code<0)
        return(FALSE);

    uint32_t response=ntohl(comm_pck.commandID);
    if(response==SUCCESS)
        return TRUE;
    else
        return FALSE;

}

/* Function used to compare two structs for the built-in qsort method*/
int node_cmp(const void *a, const void *b)
{
    int ia = ((const struct login_packet *)a)->port;
    int ib = ((const struct login_packet *)b)->port;
    return (ia-ib);
}
