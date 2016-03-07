#include "../include/global.h"
#include "../include/logger.h"


/**
 * Prints and logs the successful execution of a command
 * @param Command name
 */
void success(char* command){

    cse4589_print_and_log("[%s:SUCCESS]\n",command);

}

/**
 * Prints and logs an error message in execution of a command
 * @param Command name
 */
void error_(char* command){

    cse4589_print_and_log("[%s:ERROR]\n",command);

}

/**
 * Prints and logs the end of execution of a command
 * @param Command name
 */
void end_(char* command){

    cse4589_print_and_log("[%s:END]\n",command);

}

/**
 * Gets the IP address of the host process
 * @return Integer value of the IP address.<br> -1 in event of an error
 */
uint32_t getPublicIP()
{
	struct addrinfo hints, *res;
	struct sockaddr_in current;

	socklen_t len = sizeof current;
	void *addr;

	char ipstr[INET_ADDRSTRLEN];
	my_ip=(char *) calloc(INET_ADDRSTRLEN, sizeof(char));

	char myHostname[1024];
	char service[50];


	memset(&hints, 0, sizeof hints);    //Set all values of variable 'hints' to zero
	hints.ai_family = AF_INET;          //Set IP address type as IPv4
	hints.ai_socktype = SOCK_DGRAM;     //Set socket type as Datagram

	if(getaddrinfo("8.8.8.8", "53", &hints, &res) !=0)
	{
		//error("\nFailed to get public IP");
		return(-1);
	}

	int my_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

	if(my_fd == -1)
		return(-1);     //exit_fn("\nsocket");

	if(connect(my_fd, res->ai_addr, res->ai_addrlen) == -1)
		return(-1);     //exit_fn("\nError connecting the host process's socket");

	if(getsockname(my_fd, (struct sockaddr*)&current, &len) == -1)
		return(-1);     //exit_fn("\nError getting the host process's socket name");

	addr = &(current.sin_addr);

	if(inet_ntop(res->ai_family, addr, ipstr, sizeof(ipstr)) == NULL)
		return(-1);     //exit_fn("\nError on inet_ntop converstion");

	ipstr[strlen(ipstr)] = '\0';

	strcpy(my_ip,ipstr);

	close(my_fd);

	return(current.sin_addr.s_addr);
	//return(atoi(ipstr));    //printf("\nIP read is : %s %s",my_details.ip,ipstr);
}

/**
 * Determines the command to be executed by parsing the user input
 * @param String entered by the user
 */
void comm_process(char *comm_line){

    char *comm_args[257];                    //Stores tokenized arguments of the input command
    char temp[999];                         //Create a copy of entered command line value
    int i=0,j;                                //Counter
    unsigned int arguments=0;               //Stores no of command line arguments


    //Set all elements of temp array to '\0'
    memset((char *)&temp,'\0',sizeof(temp));

    //Set all elements of char array to '\0'
    memset((char *)&comm_args,'\0',sizeof(comm_args));

    //Copy input string
    strcpy(temp,comm_line);

    //Extract command from the user entered string
    comm_args[0]=strtok(comm_line," ");

    //printf("\nDEBUG: comm_args[0] is : %s",comm_args[0]);

    //Extract arguments
    while(comm_args[i]){

        comm_args[++i]=strtok(NULL," ");

        //printf("\nDEBUG: comm_args[%D] : %s",i,comm_args[i]);

        if(i==5 || comm_args[i]==NULL)
        {
            arguments=i-1;
            break;
        }
        //vprintf("\nDEBUG: comm_args[%d] is :%s",i,comm_args[i]);
    }

	comm_line=temp;

    //Common commands
    if (strncmp("AUTHOR",comm_args[0],sizeof("AUTHOR")-1)==0 && arguments==0)
    {

        success(comm_args[0]);
        cse4589_print_and_log("I, alizisha, have read and understood the course academic integrity policy.\n");
        end_(comm_args[0]);

    }
    else if( strncmp("PORT",comm_args[0],sizeof("PORT")-1)==0 && arguments==0 && isLoggedIn==TRUE)
    {

        if(my_port>=0 && my_port<=65536){
            success(comm_args[0]);
            cse4589_print_and_log("PORT:%d\n", my_port);
        }else{
            error_(comm_args[0]);
        }

        end_(comm_args[0]);
    }
    else if( strncmp("IP",comm_args[0],sizeof("IP")-1)==0 && arguments==0 && isLoggedIn==TRUE)
    {

        if(getPublicIP()>=0){
           success(comm_args[0]);
           cse4589_print_and_log("IP:%s\n", my_ip);
        }else{
            error_(comm_args[0]);
        }
        end_(comm_args[0]);
    }
    else if( strncmp("LIST",comm_args[0],sizeof("LIST")-1)==0 && arguments==0 && isLoggedIn==TRUE)
    {
        char *ip_addr=(char *)calloc(INET_ADDRSTRLEN+1,sizeof(char));
        char *host_name=(char *)calloc(50,sizeof(char));

        int list_id;

        if(mode==CLIENT)
        {
            success(comm_args[0]);
            for(i=0,list_id=1;i<=3;i++)
            {
                if(clients[i].port!=NOT_CONNECTED)
                {
                    //printf("\nDEBUG: 1");
                    memset(host_name,'\0',50);
                    memset(ip_addr,'\0',INET_ADDRSTRLEN+1);
                    //printf("\nDEBUG: 2");
                    //printf("\nIP before ntop is: %d",clients[i].ip);
                    inet_ntop(AF_INET,&(clients[i].ip),ip_addr,INET_ADDRSTRLEN);
                    //printf("\nDEBUG: 3");
                    get_name(host_name,clients[i].ip);
                    //printf("\nDEBUG: 4");
                    cse4589_print_and_log("%-5d%-35s%-20s%-8d\n", list_id++, host_name, ip_addr, clients[i].port);
                    //printf("\nDEBUG: 5");
                }
            }
        }else if(mode==SERVER)
        {
            success(comm_args[0]);
            //memset(host_name,'\0',50);
            //memset(ip_addr,'\0',INET_ADDRSTRLEN+1);

            /*

            for(i=0,j=1;i<=3;i++,j++)
                printf("\n%d\tIP: %d\tPort: %d",j,client_list[i].nodeinfo.ip,client_list[i].nodeinfo.port);

            fflush(stdout);*/

            for(i=0,list_id=1;i<=3;i++)
            {
                if(client_list[i].isOnline==TRUE)
                {
                    memset(host_name,'\0',50);
                    memset(ip_addr,'\0',INET_ADDRSTRLEN+1);
                    //printf("\nDEBUG: 2");
                    inet_ntop(AF_INET,&(client_list[i].nodeinfo.ip),ip_addr,INET_ADDRSTRLEN);
                    //printf("\nDEBUG: 3");
                    get_name(host_name,client_list[i].nodeinfo.ip);
                    //printf("\nDEBUG: 4");
                    cse4589_print_and_log("%-5d%-35s%-20s%-8d\n", list_id++, host_name, ip_addr, client_list[i].nodeinfo.port);
                }
            }

            /*

            printf("\nAfter listing!");
            for(i=0,j=1;i<=3;i++,j++)
                printf("\n%d\tIP: %d\tPort: %d",j,client_list[i].nodeinfo.ip,client_list[i].nodeinfo.port);

            fflush(stdout);
            */
        }else
        {
            error_(comm_args[0]);
        }

        free(ip_addr);
        free(host_name);

        end_(comm_args[0]);

    }
    else if( strncmp("EXIT",comm_args[0],sizeof("EXIT")-1)==0 && arguments==0)
    {
        //Insert method to logout

        success(comm_args[0]);
        end_(comm_args[0]);

        exit(0);
    }
    //Server Commands
    else if( strncmp("STATISTICS",comm_args[0],sizeof("STATISTICS")-1)==0 && arguments==0 && mode==SERVER)
    {
        success(comm_args[0]);
        serv_stats();
        end_(comm_args[0]);

    }
    else if( strncmp("BLOCKED",comm_args[0],sizeof("BLOCKED")-1)==0 && arguments==1 && mode==SERVER)
    {
        command_blocked(comm_args[1]);
        end_(comm_args[0]);
    }
    //Client Commands
    else if( strncmp("BLOCK",comm_args[0],sizeof("BLOCK")-1)==0 && arguments==1 && mode==CLIENT && isLoggedIn==TRUE
        && strncmp("BLOCKED",comm_args[0],sizeof("BLOCKED")-1)!=0)
    {
        //printf("\nDEBUG: Extracted IP is : %s",comm_args[1]);
        //fflush(stdout);

        if(client_block(comm_args[1])==TRUE)
        {
            success(comm_args[0]);
        }else
        {
            error_(comm_args[0]);
        }
        end_(comm_args[0]);
    }
    else if( strncmp("LOGIN",comm_args[0],sizeof("LOGIN")-1)==0 && arguments==2 && mode==CLIENT &&isLoggedIn==FALSE )
    {
        if(client_login(comm_args[1],atoi(comm_args[2]))==TRUE)
        {
            success(comm_args[0]);
            isLoggedIn=TRUE;
            isLoggedOut=FALSE;
            FD_SET(client_sock, &master);                   //Add client_socket to client's master FD list
            //FD_SET(client_sock, &read_fds);
            if(client_sock>fdmax)
                fdmax=client_sock;
            //printf("\nDebug: Client_socket successfully added to list!");
            //fflush(stdout);
        }else
        {
            error_(comm_args[0]);
        }

        end_(comm_args[0]);

    }
    else if( strncmp("REFRESH",comm_args[0],sizeof("REFRESH")-1)==0 && arguments==0 && mode==CLIENT && isLoggedIn==TRUE)
    {
        int i,list_id;
        char *ip_addr=(char *)calloc(INET_ADDRSTRLEN+1,sizeof(char));
        char *host_name=(char *)calloc(50,sizeof(char));

        if(client_refresh()==TRUE)
        {
            success(comm_args[0]);
            for(i=0,list_id=1;i<=3;i++)
            {
                if(clients[i].port!=NOT_CONNECTED)
                {
                    //printf("\nDEBUG: 1");
                    memset(host_name,'\0',50);
                    memset(ip_addr,'\0',INET_ADDRSTRLEN+1);
                    //printf("\nDEBUG: 2");
                    //printf("\nIP before ntop is: %d",clients[i].ip);
                    inet_ntop(AF_INET,&(clients[i].ip),ip_addr,INET_ADDRSTRLEN);
                    //printf("\nDEBUG: 3");
                    get_name(host_name,clients[i].ip);
                    //printf("\nDEBUG: 4");
                    cse4589_print_and_log("%-5d%-35s%-20s%-8d\n", list_id++, host_name, ip_addr, clients[i].port);
                    //printf("\nDEBUG: 5");
                }
            }

        }
        else{
            error_(comm_args[0]);
        }

        free(ip_addr);
        free(host_name);

        end_(comm_args[0]);

    }
    else if( strncmp("SEND",comm_args[0],sizeof("SEND")-1)==0 && isLoggedIn==TRUE&& mode==CLIENT && arguments>=2)
    {
        char *ip,*msg;//comm_args[1];
        strtok(comm_line," ");
        ip=strtok(NULL," ");
        msg=strtok(NULL,"\n");

        if(send_message_client(ip,msg)==TRUE)
        {
            success(comm_args[0]);
        }else
        {
            error_(comm_args[0]);
        }
        end_(comm_args[0]);
    }
    else if( strncmp("BROADCAST",comm_args[0],sizeof("BROADCAST")-1)==0  && isLoggedIn==TRUE && mode==CLIENT &&arguments>=1)
    {
        char *msg;

        /* Extract message from the entire command */
        strtok(comm_line," ");                          //Separate command string
        msg=strtok(NULL,"\n");

        if(send_broadcast(msg)==TRUE)
            success(comm_args[0]);
        else
            error_(comm_args[0]);

        end_(comm_args[0]);

    }
    else if( strncmp("UNBLOCK",comm_args[0],sizeof("UNBLOCK")-1)==0 && arguments==1 && mode==CLIENT && isLoggedIn==TRUE)
    {
        if(client_unblock(comm_args[1])==TRUE)
        {
            success(comm_args[0]);
        }else{
            error_(comm_args[0]);
        }

        end_(comm_args[0]);
    }
    else if( strncmp("LOGOUT",comm_args[0],sizeof("LOGOUT")-1)==0 && arguments==0 && mode==CLIENT && isLoggedIn==TRUE)
    {
        if(client_logout()==TRUE)
        {
            success(comm_args[0]);
            isLoggedIn=FALSE;
            isLoggedOut=TRUE;
            //Stop listening on client_sock
            FD_CLR(client_sock, &master);

        }else
        {
            error_(comm_args[0]);
        }

        end_(comm_args[0]);
    }
    else if( strncmp("SENDFILE",comm_args[0],sizeof("SENDFILE")-1)==0 && arguments==2 && mode==CLIENT && isLoggedIn==TRUE){
    }else{
    }
}

void get_name(char* name,uint32_t ip)
{
    //uint32_t ip;
    //ip=htonl(ip);

    //Get binary representation of known IP addresses
    //uint32_t ip_stones;
    struct sockaddr_in ip_stones;
    inet_pton(AF_INET,"128.205.36.46",(void *)&(ip_stones.sin_addr));

    struct sockaddr_in ip_euston;
    inet_pton(AF_INET,"128.205.36.34",(void *)&(ip_euston.sin_addr));

    struct sockaddr_in ip_embank;
    inet_pton(AF_INET,"128.205.36.35",(void *)&(ip_embank.sin_addr));

    struct sockaddr_in ip_highgate;
    inet_pton(AF_INET,"128.205.36.33",(void *)&(ip_highgate.sin_addr));

    struct sockaddr_in ip_underground;
    inet_pton(AF_INET,"128.205.36.36",(void *)&(ip_underground.sin_addr));

    if(ip==ip_stones.sin_addr.s_addr)
        strcpy(name,"stones.cse.buffalo.edu");
    else if(ip==ip_euston.sin_addr.s_addr)
        strcpy(name,"euston.cse.buffalo.edu");
    else if(ip==ip_embank.sin_addr.s_addr)
        strcpy(name,"embankment.cse.buffalo.edu");
    else if(ip==ip_highgate.sin_addr.s_addr)
        strcpy(name,"highgate.cse.buffalo.edu");
    else if(ip==ip_underground.sin_addr.s_addr)
        strcpy(name,"underground.cse.buffalo.edu");
    else
        strcpy(name,"Unknown server");

}


