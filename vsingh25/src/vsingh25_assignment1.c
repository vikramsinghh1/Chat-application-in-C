/**
 * @vsingh25_assignment1
 * @author  Vikram Singh <vsingh25@buffalo.edu>
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


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <strings.h>
#include <string.h>
#include <ifaddrs.h>

#include "../include/global.h"
#include "../include/logger.h"

//-------------------------------------------------------------------------------------------------------------
//------------------------------------Client Constants and declarations-----------------------------------------
//-------------------------------------------------------------------------------------------------------------


char *getIPAddress();  // common
int connectToServer(char *connectIP, int connectPort);
void readStandardInput(char *temp[]);
void processCommand(char **argVV, int count, char *msgCpy);
void printError(char *command,char *mssg);
void printSuccess(char *command,char *mssg);
int sendMssg(char *,int); 
int callClient(int);
char* getHostName(char *ipaddress); //common
void processServerResponse(char *response);
int checkAndSend(char *msgToClient,char *peerIp);
int getUserNumber();
bool checkClientBlock(char *peerIp);
bool validIP(char *pIp);

bool ipChecker(char *ipaddress);

bool hasLoggedIn = false;
int SERVER_SOCKET;
bool isConnected;
int C_PORT;
char *CLIENT_IP;
int CNOC;
struct ClientList
{

    int list_id;
    char *hostname;
    char *ip_addr;
    int port_num;
    int status;
    char *blockUsers[4];
    int NOBU;
};
struct ClientList clientList[4];

//-------------------------------------------------------------------------------------------------------------
//------------------------------------Server Constants and declarations-----------------------------------------
//-------------------------------------------------------------------------------------------------------------
#define STDIN 0
#define TRUE 1
#define CMD_SIZE 100
#define BUFFER_SIZE 1000

//char* getIPAddress();
void processServerCommand(char * command,char *ipp,int count);
void processClientOnServer(int clientSocket, char * command);
int callServer(int argc);
void sortList();
void sendList(int clientSocket);



int PORT;
int NOC=0;
char *SERVER_IP;

struct List
{
    int fd_id;
    char *hostname;
    char *ip_addr;
    int port_num;
    int num_msg_sent;
    int num_msg_rcv;
    int status;
    char *blockUsers[4];
    int NOBU;
    char *buffMsg[100];
    int NOBM;
};

struct List cList[4];

//-------------------------------------------------------------------------------------------------------------
//------------------------------------END : Server Constants and declarations----------------------------------
//-------------------------------------------------------------------------------------------------------------

//////////commons
int BLOCK_ARRAY[6][6];
int ipToNumber(char * ipaddress);

////////////

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

	if(argc != 3)
	{
		printf("INVALID \n");
		exit(0);
	}
	else
	{
		if(strcmp(argv[1],"c") == 0)
			callClient(atoi(argv[2]));
		else if(strcmp(argv[1],"s")==0)
			callServer(atoi(argv[2]));
	}
    return 0;
}



//-------------------------------------------------------------------------------------------------------------
//----------------------------------------Start : Server functions---------------------------------------------
//-------------------------------------------------------------------------------------------------------------

int callServer(int argv)
{
    PORT = argv;
    SERVER_IP = getIPAddress();
    int server_socket, head_socket, selret, sock_index, fdaccept=0, caddr_len;
    struct sockaddr_in server_addr, client_addr;
    fd_set master_list, watch_list;
    /* Socket */
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(server_socket < 0)
        perror("Cannot create socket");
    /* Fill up sockaddr_in struct */
    //bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);
    /* Bind */
    if(bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0 )
        perror("Bind failed");
    /* Listen */
    if(listen(server_socket, 5) < 0)
        perror("Unable to listen on port");
    /* ---------------------------------------------------------------------------- */
    /* Zero select FD sets */
    FD_ZERO(&master_list);
    FD_ZERO(&watch_list);
    /* Register the listening socket */
    FD_SET(server_socket, &master_list);
    /* Register STDIN */
    FD_SET(STDIN, &master_list);
    head_socket = server_socket;
    while(TRUE){
        memcpy(&watch_list, &master_list, sizeof(master_list));  // why memcpy and not normal copy
        selret = select(head_socket + 1, &watch_list, NULL, NULL, NULL);
        if(selret < 0)
            perror("select failed.");
        if(selret > 0){
            for(sock_index=0; sock_index<=head_socket; sock_index+=1){
                if(FD_ISSET(sock_index, &watch_list)){
                   /* Check if new command on STDIN */
                    if (sock_index == STDIN){
                        char mssg[256];
                        char * pch;
                        char *argVV[30];
                        int count;
                        memset(mssg, '\0',sizeof(char) *256);
                        if(fgets(mssg, 256, stdin) == NULL)
                        {
                            printf("fgets faile d\n");
                            exit(0);
                        }
                        int len = strlen(mssg);
                        if( mssg[len-1] == '\n')
                           mssg[len-1] = '\0';
                        count =0;
                        pch = strtok(mssg," ");
                        while (pch != NULL)
                        {      
                            argVV[count] = pch;
                            pch = strtok(NULL, " ");
                            count++;
                        }
                        char *command = argVV[0];
                        char *ip_new = argVV[1];
                        processServerCommand(command,ip_new,count);
                    }
                    /* Check if new client is requesting connection */
                    else if(sock_index == server_socket){
                        caddr_len = sizeof(client_addr);
                        fdaccept = accept(server_socket, (struct sockaddr *)&client_addr, &caddr_len);
                        if(fdaccept < 0)
                            perror("Accept failed.");
                        FD_SET(fdaccept, &master_list);
                        int client_port;
                        recv(fdaccept,&client_port,sizeof(client_port),0);
                        char *listip = inet_ntoa(client_addr.sin_addr);
                        int listport = ntohl(client_port);
                        char *ippp = strdup(listip);
                        cList[NOC].fd_id=fdaccept;
                        cList[NOC].ip_addr = ippp;
                        cList[NOC].hostname = getHostName(listip);
                        cList[NOC].port_num =  listport;
                        cList[NOC].status = 1;
                        cList[NOC].NOBU = 0;
                        NOC++;
                        sortList();
                        sendList(fdaccept);
                        
                        //send(SERVER_SOCKET,&portt,sizeof(portt),0);
                        if(fdaccept > head_socket) head_socket = fdaccept;
                    }
                    /* Read from existing clients */
                    else{
                        /* Initialize buffer to receieve response */                   
                        char *buffer = (char*) malloc(sizeof(char)*BUFFER_SIZE);
                        memset(buffer, '\0',sizeof(char)* BUFFER_SIZE);

                        if(recv(sock_index, buffer, BUFFER_SIZE, 0) <= 0){
                            close(sock_index);
                            printf("Remote Host terminated connection!\n");

                            /* Remove from watched list */
                            FD_CLR(sock_index, &master_list);
                        }
                        else {
                            //Process incoming data from existing clients here ...
                         //   printf("\nClient sent me: %s\n", buffer);
                         //.   printf("ECHOing it back to the remote host ... ");
                          //  if(send(fdaccept, buffer, strlen(buffer), 0) == strlen(buffer))
                          //      printf("Done!\n");
                            
                                char * cmd = buffer;
                                printf("Execute this : %s\n",cmd);
                                processClientOnServer(sock_index, cmd);


                            fflush(stdout);
                        }

                        free(buffer);
                    }
                }
            }
        }
    }

    return 0;
}


void processClientOnServer(int clientSocket, char * command)
{   
    
    //client socket contains the sender's fd_id
    char *senderIp;
    for(int i =0; i<NOC; i++)
    {
        if(clientSocket ==cList[i].fd_id)
        { 
           senderIp = cList[i].ip_addr;
           break;
        }
    }

    // easy approach

    //printf("request from client is %s\n",command );
    char *cmd,*cIp,*cMsg,*tok,*ccMsg;
    cmd = (char*)malloc(sizeof(char)*100);
    memset(cmd,'\0',sizeof(char)*100);

    cIp = (char*)malloc(sizeof(char)*256);
    memset(cIp,'\0',sizeof(char)*256);

    tok = strtok(command," ");
    if(tok != NULL)
    {      
        cmd = tok;
        tok = strtok(NULL, " ");
    }


    if(strcmp("SEND",cmd) == 0 || strcmp("BROADCAST",cmd) == 0)
    {   
        bool isBroadcast = false;
        if(strcmp("BROADCAST",cmd) == 0)
            isBroadcast = true;
        cMsg = (char*)malloc(sizeof(char)*263);  //change it to 257
        memset(cMsg,'\0',sizeof(char)*263);
        ccMsg = (char*)malloc(sizeof(char)*258);
        memset(ccMsg,'\0',sizeof(char)*258);
        strcat(cMsg,"MSSG ");
        strcat(cMsg,senderIp);
        strcat(cMsg," ");
        
        if(isBroadcast)
        {
            cIp = "255.255.255.255";
        }
        else
        {
            if(tok != NULL)
            {
                cIp = tok;
                tok = strtok(NULL," ");
            }
        }

        while(tok != NULL)
        {                   
            strcat(cMsg,tok);
            strcat(cMsg," ");
            strcat(ccMsg,tok);
            strcat(ccMsg," ");
            tok = strtok(NULL," ");     
        }
        cMsg[strlen(cMsg)-1] = '\0';
        ccMsg[strlen(ccMsg)-1] = '\0'; //trailing removal
       /* printf("message has to be sent to %s\n",cIp);
        printf("message is %s.\n",cMsg);*/
        printf("this is the shizz %s\n",cMsg);
        char *relayedMsg;
        relayedMsg = (char*)malloc(sizeof(char)*400);
        memset(relayedMsg,'\0',sizeof(char)*400);
      //  printf("msg is %s.\n",cMsg);
        bool isBroadcastSuccesful = true;
        for(int i =0;i<NOC;i++)
        {
            if(cList[i].status == 1)
            { 
                if(isBroadcast)
                {
                    
                    if(BLOCK_ARRAY[ipToNumber(cList[i].ip_addr)][ipToNumber(senderIp)]==1)
                    {
                        for(int j =0;j<NOC;j++)
                        {
                            if(strcmp(cList[j].ip_addr,senderIp) == 0)
                                {
                                    cList[j].num_msg_sent++;
                                    break;
                                }
                        }
                    }
                    else   
                    {
                        int n = sendMssg(cMsg,cList[i].fd_id);
                        for(int j =0;j<NOC;j++)
                        {
                            if(strcmp(cList[j].ip_addr,senderIp) == 0)
                                {
                                    cList[j].num_msg_sent++;
                                    cList[i].num_msg_rcv++;
                                    break;
                                }
                        }
                        if(n==-1)
                        {
                            printError("RELAYED",NULL);
                            isBroadcastSuccesful = false;
                        }
                    }    
                }
                else if(strcmp(cIp,cList[i].ip_addr)==0)   ///******can add difference here for broadcast and single
                {
                    int fd = cList[i].fd_id;
                    if(BLOCK_ARRAY[ipToNumber(cIp)][ipToNumber(senderIp)]==1)
                    {
                        for(int j =0;j<NOC;j++)
                        {
                            if(strcmp(cList[j].ip_addr,senderIp) == 0)
                                {
                                    cList[j].num_msg_sent++;
                                    break;
                                }
                        }
                    }
                    else
                    {
                        int n = sendMssg(cMsg,fd);
                        for(int j =0;j<NOC;j++)
                        {
                            if(strcmp(cList[j].ip_addr,senderIp) == 0)
                                {
                                    cList[j].num_msg_sent++;
                                    cList[i].num_msg_rcv++;
                                    break;
                                }
                        }
                        if(n!=-1)
                        {
                            sprintf(relayedMsg,"msg from:%s, to:%s\n[msg]:%s\n", senderIp, cIp, ccMsg);
                            printSuccess("RELAYED",relayedMsg);
                        }
                        else
                            printError("RELAYED",NULL);
                    }
                    break;
                }
            }
            else
            {
                if(isBroadcast)
                {
                    if(BLOCK_ARRAY[ipToNumber(cList[i].ip_addr)][ipToNumber(senderIp)]==1) //BLOCKED BROADCAST LOGGED OUT
                    {
                        for(int j =0;j<NOC;j++)
                        {
                            if(strcmp(cList[j].ip_addr,senderIp) == 0)
                                {
                                    cList[j].num_msg_sent++;
                                    break;
                                }
                        }
                    }
                    else   /// BROADCAST LOGGED OUT  ********
                    {
                        for(int j =0;j<NOC;j++)
                        {
                            if(strcmp(cList[j].ip_addr,senderIp) == 0)
                                {
                                    cList[j].num_msg_sent++;
                                    //*******************IMplement buffering here
                                    break;
                                }
                        }
                    }
                }
                else if(strcmp(cIp,cList[i].ip_addr)==0)
                {
                    if(BLOCK_ARRAY[ipToNumber(cIp)][ipToNumber(senderIp)]==1)  //BLOCKED UNICAST LOGGED OUT
                    {
                        for(int j =0;j<NOC;j++)
                        {
                            if(strcmp(cList[j].ip_addr,senderIp) == 0)
                                {
                                    cList[j].num_msg_sent++;
                                    break;
                                }
                        }
                        break;
                    }
                    else     //  UNICAST LOG OUT ****************
                    {
                        for(int j =0;j<NOC;j++)
                        {
                            if(strcmp(cList[j].ip_addr,senderIp) == 0)
                                {
                                    cList[j].num_msg_sent++;
                                    char *formattedMsg;
                                    formattedMsg = (char*)malloc(sizeof(char)*400);
                                    memset(formattedMsg,'\0',sizeof(char)*400);
                                    sprintf(formattedMsg,"msg from:%s\n[msg]:%s\n", senderIp, ccMsg);
                                    cList[i].buffMsg[cList[i].NOBM] = formattedMsg;
                                    cList[i].NOBM++;
                                    break;
                                }
                        }

                    }
                }
                // **************IMPLEMENT BUFFERED MESSAGES HERE***************8
            }

        }
        if(isBroadcastSuccesful && isBroadcast)
        {
            sprintf(relayedMsg,"msg from:%s, to:%s\n[msg]:%s\n", senderIp, cIp, ccMsg);
            printSuccess("RELAYED",relayedMsg);
        }
       // recv(clientSocket, &ip_response, sizeof(ip_response),0);
       // printf("Msg has to be sent to %s\n",ip_response);
    }
    else if(strcmp("LOGOUT",cmd)==0)
    {
        for(int i = 0;i<NOC;i++)
        {
            if(clientSocket == cList[i].fd_id)
            {
                cList[i].status =0;
                break;
            }
        }
    }
    else if(strcmp("REFRESH",cmd)==0)
    {
        for(int i = 0;i<NOC;i++)
        {
            if(clientSocket == cList[i].fd_id)
            {
                sortList();
                sendList(clientSocket);
                break;
            }
        }
    }
    else if(strcmp("BLOCK",cmd)==0)  //clientSocket
    {
        if(tok != NULL)
        {      
            cIp = tok;
            tok = strtok(NULL, " ");
        }

        for(int user=0;user<NOC;user++)
        {
            if(cList[user].fd_id == clientSocket)   // blocking  and i tells the ith users info
            {
                char * tmpIP = cList[user].ip_addr;
                BLOCK_ARRAY[ipToNumber(tmpIP)][ipToNumber(cIp)]= 1;
                break;
                /*printf("It came till here ");
                printf("user ip is %s \n",cIp);
                int userNo = cList[user].NOBU;
                printf("user number : %d\n",userNo);
                cList[user].blockUsers[userNo] = (char *)malloc(sizeof(char)*256);
                memset(cList[user].blockUsers[userNo],'\0',sizeof(char)*256);
                cList[user].blockUsers[userNo] = strdup(cIp);
                cList[user].NOBU++;
                break;*/
            }

        }
       // printf("%s has to block %s",senderIp,cIp);
    }
    else if(strcmp("UNBLOCK",cmd)==0)  //clientSocket
    {
        if(tok != NULL)
        {      
            cIp = tok;
            tok = strtok(NULL, " ");
        }

        for(int user=0;user<NOC;user++)
        {
            if(cList[user].fd_id == clientSocket)   // blocking  and i tells the ith users info
            {
                char * tmpIP = cList[user].ip_addr;
                BLOCK_ARRAY[ipToNumber(tmpIP)][ipToNumber(cIp)]= 0;
                break;
                /*printf("It came till here ");
                printf("user ip is %s \n",cIp);
                int userNo = cList[user].NOBU;
                printf("user number : %d\n",userNo);
                cList[user].blockUsers[userNo] = (char *)malloc(sizeof(char)*256);
                memset(cList[user].blockUsers[userNo],'\0',sizeof(char)*256);
                cList[user].blockUsers[userNo] = strdup(cIp);
                cList[user].NOBU++;
                break;*/
            }

        }
       // printf("%s has to block %s",senderIp,cIp);
    }
    else if(strcmp("LOGIN",command) == 0)
    {
        int user;
        for(int i = 0; i<NOC;i++)
        {
            if(cList[i].fd_id == clientSocket)
            {
               cList[i].status = 1; 
               user =i;
               break;
            }
        }

        char*buffermssg;
        buffermssg = (char *)malloc(sizeof(char *)*10000);
        memset(buffermssg,'\0',sizeof(char *)*10000);

        strcat(buffermssg,"BMM#$$#");
        for(int i = 0;i<cList[user].NOBM;i++)
        {
            strcat(buffermssg,cList[user].buffMsg[i]);
            strcat(buffermssg,"#$$#");
        }
        sendMssg(buffermssg,clientSocket);
    }
}

int checkServerBlock(int user,char* cIp)
{

    for(int i =0;i < cList[user].NOBU;i++)
    {
        if(strcmp(cList[user].blockUsers[i],cIp)==0)
            return -1;
    }
    return 0;
}

void processServerCommand(char * command,char *ipp,int count)
{
		char mssg[256];
        if(strcmp("AUTHOR",command) == 0)
        {
            char *msg = "I, vsingh25, have read and understood the course academic integrity policy.\n";   
        	printSuccess("AUTHOR",msg);
        }
        else if(strcmp("PORT",command) == 0)
        {	
        	sprintf(mssg,"PORT:%d\n",PORT);
        	printSuccess("PORT",mssg);
        }
        else if(strcmp("IP", command) == 0)
        {
            sprintf(mssg,"IP:%s\n",SERVER_IP);
            printSuccess("IP",mssg);
        }
        else if(strcmp("LIST",command)==0)
        {
            cse4589_print_and_log("[LIST:SUCCESS]\n");
            int inn =1;
            for(int i =0;i<NOC;i++)
            {
                if(cList[i].status== 1)
                {
                    cse4589_print_and_log("%-5d%-35s%-20s%-8d\n",inn,cList[i].hostname,cList[i].ip_addr,cList[i].port_num);
                    inn++;
                }
            }
            cse4589_print_and_log("[LIST:END]\n");


        }
        else if(strcmp("BLOCKED",command)==0)
        {
            if(count!=2)
            {
                printError("BLOCKED",NULL);
            }
            else
            {
                bool ipExists =false;
                int user;
                for(int i =0;i<NOC;i++)
                {
                    if(strcmp(cList[i].ip_addr,ipp) == 0)
                    {
                        printf("found it atleast\n");
                        ipExists = true;
                        user =i;
                        break;
                    }
                }
                //if ip exists
                if(ipExists)
                {
                    int x=1;
                    cse4589_print_and_log("[BLOCKED:SUCCESS]\n");
                    for(int i = 0;i<NOC;i++)
                    {
                        if(BLOCK_ARRAY[ipToNumber(ipp)][ipToNumber(cList[i].ip_addr)] == 1)
                        {
                            cse4589_print_and_log("%-5d%-35s%-20s%-8d\n",x,cList[i].hostname,cList[i].ip_addr,cList[i].port_num);
                            x++;
                        }
                    }
                    cse4589_print_and_log("[BLOCKED:END]\n");

                    /*int x =0;
                    cse4589_print_and_log("[BLOCKED:SUCCESS]\n");
                    for(int i = 0;i<NOC;i++)
                    {
                        for(int j =0;j<cList[user].NOBU;j++)
                        {
                            if(strcmp(cList[i].ip_addr, cList[user].blockUsers[j])==0)
                            {
                                x++;
                                cse4589_print_and_log("%-5d%-35s%-20s%-8d\n",x,cList[i].hostname,cList[i].ip_addr,cList[i].port_num);
                            }
                        }
                    }
                    cse4589_print_and_log("[BLOCKED:END]\n");*/
                }
                else
                {
                    printError("BLOCKED",NULL);
                }

            }

        }
        else if(strcmp("STATISTICS",command)==0)
        {
            int x=1;
            cse4589_print_and_log("[STATISTICS:SUCCESS]\n");
            for(int i = 0;i<NOC;i++)
            {
                char *status_msg;
                if(cList[i].status == 1)
                    status_msg = "logged-in";
                else
                    status_msg = "logged-out";
                cse4589_print_and_log("%-5d%-35s%-8d%-8d%-8s\n",x,cList[i].hostname, cList[i].num_msg_sent , cList[i].num_msg_rcv , status_msg);
                x++;
                
            }
            cse4589_print_and_log("[STATISTICS:END]\n");
        }
}


void sortList()
{
    struct List temp;
    for(int i = 0; i<NOC; i++)
    {
        for(int j =0; j<NOC;j++)
        {
            if(cList[i].port_num < cList[j].port_num)
            {
                temp = cList[i];
                cList[i] = cList[j];
                cList[j] = temp;
            }

        }
    }

}

void sendList(int clientSocket)
{
    char listMssg[4000];
    memset(listMssg, '\0',sizeof(char)*4000);
    //listMssg[0] ='\0';
    char portbuffer[256];
    int count = 0;
    for(int j = 0; j<NOC; j++)
    {
        if(cList[j].status==1)
        count++;
    }
    strcat(listMssg,"clist ");
    sprintf(portbuffer,"%d",count);
    strcat(listMssg,portbuffer);
    for(int j = 0; j<NOC; j++)
        {
            if(cList[j].status == 1)
            {   
                strcat(listMssg," ");
                strcat(listMssg,cList[j].hostname);
                strcat(listMssg," ");
                strcat(listMssg,cList[j].ip_addr);
                strcat(listMssg," ");
                sprintf(portbuffer,"%d",cList[j].port_num);
                strcat(listMssg,portbuffer);
            }
        }
     //   printf("message to be sent is %s\n",listMssg);
    send(clientSocket,&listMssg,sizeof(listMssg),0);
}



//-------------------------------------------------------------------------------------------------------------
//----------------------------------------Start : Common Functions---------------------------------------------
//-------------------------------------------------------------------------------------------------------------

char* getIPAddress()       // this function is from stack overflow
{
    struct ifaddrs *ifap, *ifa;
    struct sockaddr_in *sa;
    char *addr;

    getifaddrs (&ifap);
    for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr->sa_family==AF_INET) {
            sa = (struct sockaddr_in *) ifa->ifa_addr;
            addr = inet_ntoa(sa->sin_addr);
            if(strcmp("eth0",ifa->ifa_name)==0)
                         break;
            //printf("%s",addr);
        }
    }
    return addr;
    freeifaddrs(ifap);
}



//-------------------------------------------------------------------------------------------------------------
//----------------------------------------Start : Client Functions---------------------------------------------
//-------------------------------------------------------------------------------------------------------------



int callClient(int pport)  
{
    C_PORT = pport;
    CLIENT_IP = getIPAddress();
    isConnected = false;
    fd_set master_list, watch_list;
    FD_ZERO(&master_list);
    FD_ZERO(&watch_list);
    FD_SET(STDIN, &master_list);
    int head_socket = STDIN;
    int selret,sock_index;
    while(1)
    {

        memcpy(&watch_list, &master_list, sizeof(master_list));
        selret = select(head_socket + 1, &watch_list, NULL, NULL, NULL);
        if(selret < 0)
            perror("select failed.");
        if(selret > 0){
            for(sock_index=0; sock_index<=head_socket; sock_index+=1)
            {
                if(FD_ISSET(sock_index, &watch_list))
                {
                    if (sock_index == STDIN)
                    {
                        fflush(stdout); 
                        char mssg[2000];
                        char * pch;
                        char *argVV[30];
                        int count;
                        memset(mssg, '\0',sizeof(char)*2000);
                        if(fgets(mssg, 2000, stdin) == NULL)
                        {
                            printf("fgets faile d\n");
                            exit(0);
                        }
                        int len = strlen(mssg);
                        if( mssg[len-1] == '\n' )
                        mssg[len-1] = '\0';
                        char *msgCpy;
                        msgCpy = (char*)malloc(sizeof(char)*2000);
                        memset(msgCpy,'\0',sizeof(char)*2000);
                        msgCpy = strdup(mssg);
                        count =0;
                        pch = strtok(mssg," ");
                        while (pch != NULL)
                        {   
                            argVV[count] = pch;
                            pch = strtok(NULL, " ");
                            count++;
                        }
                        processCommand(argVV,count,msgCpy);
                        if(isConnected)
                            {
                                FD_SET(SERVER_SOCKET,&master_list);
                                head_socket = SERVER_SOCKET;
                            }
                    }
                    else if(isConnected && head_socket == SERVER_SOCKET)
                    {
                        char server_response[4000];
                        memset(server_response,'\0',sizeof(char)*4000);
                        int arg = 0;
                        int serv_recv = recv(SERVER_SOCKET, &server_response, sizeof(server_response),0);
                        if(serv_recv!=-1)
                        {
                            processServerResponse(server_response);
                        }  
                    }    
                }
            }
        }       
 
        //send(sock,"i am back", sizeof("i am back"),0);  
    } 
return 0;
}


int connectToServer(char *connectIP, int connectPort)
{
    SERVER_SOCKET = socket(AF_INET,SOCK_STREAM,0);
    struct sockaddr_in clientaddr;
    clientaddr.sin_family = AF_INET;
    clientaddr.sin_addr.s_addr = inet_addr(connectIP);
    clientaddr.sin_port = htons(connectPort);
    int conn = connect(SERVER_SOCKET, (struct sockaddr*) &clientaddr, sizeof(clientaddr)); //-1 if fails
    //printf("reached here\n");
    int portt = htonl(C_PORT);  // this is just for testing --> to change later and accept from command line
    send(SERVER_SOCKET,&portt,sizeof(portt),0);
    return conn;
}


/*void readStandardInput(char *temp[])   TO implement later in order to improve on code readability
{
        char mssg[256];
        char * pch;
        char *argVV[30];
        int count;
        if(fgets(mssg, 256, stdin) == NULL)
        {
            printf("fgets faile d\n");
            exit(0);
        }
        printf("Execute this command %s \n",mssg);
        count =0;
        pch = strtok(mssg," ");
        while (pch != NULL)
        {   
            argVV[count] = pch;
            pch = strtok(NULL, " ");
            count++;
        }
        //return argVV;
}
*/ 
void processServerResponse(char *response)
{
 //   printf("server response : %s ",response);
    char *res = strtok(response," ");
    char *cmd;
    int count=0;
    if(res != NULL)
    {
        cmd = res;
        res = strtok(NULL," ");
    }
    
   if(strcmp(cmd,"clist") == 0)
   {
       // printf("LIST\n");
        if(res != NULL)
        {
            count = atoi(res);
            res = strtok(NULL," ");
        }
        //printf("count = 11%d\n",count);
        CNOC=0;
       // printf("here \n");
        while(res != NULL && CNOC<count)
        {   
         //   printf("entered \n");
            CNOC++;
           // printf("%s\n",res);
            clientList[CNOC].hostname =(char*)malloc(sizeof(char)*256);
            memset(clientList[CNOC].hostname,'\0',sizeof(char)*256);
            clientList[CNOC].hostname= strdup(res);
            res = strtok(NULL, " ");
           // printf("%s \n",res);
            clientList[CNOC].ip_addr =(char*)malloc(sizeof(char)*256);
            memset(clientList[CNOC].ip_addr,'\0',sizeof(char)*256);
            clientList[CNOC].ip_addr= strdup(res);
            res = strtok(NULL, " ");
            clientList[CNOC].port_num = atoi(res);
            clientList[CNOC].NOBU = 0;
            // printf("%d\n",atoi(res));
            res = strtok(NULL, " ");
        }
    }
    else if(strcmp(cmd,"MSSG")==0)
    {
       //printf("Response : ");

        char *finalIp;
        finalIp = (char*)malloc(sizeof(char)*256);
        memset(finalIp,'\0',sizeof(char)*256);

        if(res!=NULL)
        {
            finalIp = strdup(res);
            res = strtok(NULL," ");
        }

        char *finalMsg,*printIt;
        finalMsg = (char*)malloc(sizeof(char)*258);
        memset(finalMsg,'\0',sizeof(char)*258);
        while(res != NULL)
        {
            strcat(finalMsg,res);
            strcat(finalMsg," ");
            res = strtok(NULL, " ");
        }
        finalMsg[strlen(finalMsg)-1]='\0';
        printIt = (char*)malloc(sizeof(char)*300);
        memset(printIt,'\0',sizeof(char)*300);
        sprintf(printIt,"msg from:%s\n[msg]:%s\n", finalIp, finalMsg);
        printSuccess("RECEIVED",printIt);
    }
}

void processCommand(char **argVV,int count,char *fullMsg)
{
        char mssg[256];
        char *command = argVV[0];


        if(strcmp("LOGIN",command)==0)
        {
            if(count!=3)
            printError("LOGIN",NULL);
            else if(isConnected)
                printError("LOGIN",NULL);
            else
            {
                if(!hasLoggedIn)
                { 
                    char *serverIp = argVV[1];
                    if(ipChecker(serverIp))
                    {
                        char *tmp = argVV[2];
                        //int serverPort = atoi(argVV[2]);
                        bool isValid = true;
                        for(int i =0;tmp[i]!='\0';i++)
                        {
                            if(tmp[i]<48 || tmp[i]>57)
                            {
                                isValid = false;
                                break;
                            }
                            printf("1\n");
                        }
                        if(isValid)
                        {
                            printf("2\n");
                            int serverPort = atoi(argVV[2]);    
                            int conn;
                            conn = connectToServer(serverIp,serverPort);  // ip and port need to get from command instead
                            if(conn == -1)
                                printError("LOGIN",NULL);
                            else
                            {
                                isConnected = true;
                                hasLoggedIn = true;
                                printSuccess("LOGIN",NULL);
                            }
                        }
                        else
                            printError("LOGIN",NULL);    
                    }
                    else
                        printError("LOGIN",NULL);
                }    
                else
                {
                    isConnected = true;
                    sendMssg("LOGIN",SERVER_SOCKET);
                    printSuccess("LOGIN",NULL);
                    char *buffmessage;
                    recv(SERVER_SOCKET,&buffmessage,sizeof(buffmessage),0);
                    printf("%s",buffmessage);
                    
                }
            }
        }
        else if(strcmp("AUTHOR",command) == 0)
        {
            char msg[] = "I, vsingh25, have read and understood the course academic integrity policy.\n";
            printSuccess("AUTHOR",msg);         
        }
        else if(strcmp("EXIT",command) == 0)
        {
		exit(1);
        }
        else if(strcmp("PORT",command) == 0)
        {
            sprintf(mssg,"PORT:%d\n",C_PORT);
            printSuccess("PORT",mssg);
        }
        else if(strcmp("IP", command) == 0)
        {
            sprintf(mssg,"IP:%s\n",CLIENT_IP);
            printSuccess("IP",mssg);
        }
        else if(strcmp("LOGOUT", command) == 0)
        {
            if(!isConnected)
                printError("LOGOUT",NULL);
            else
            {
                printSuccess("LOGOUT",NULL);
                isConnected = false;
                int n = sendMssg("LOGOUT",SERVER_SOCKET);
                if(n == -1)
                    isConnected = true;
            }
        }
        else if (strcmp("REFRESH",command)==0)
        {
                if(!isConnected)
                    printError("REFRESH",NULL);
                else
                {
                    sendMssg("REFRESH",SERVER_SOCKET);
                    printSuccess("REFRESH",NULL);
                } 
                
        }
        else if(strcmp("SEND", command) == 0)    // partition between connected to server and not connected to server commands
         {
            if(count<3)
            {
                printError("SEND",NULL);
            }
            else
            {
                if(isConnected)
                {
                    char *msgBuffer;
                    msgBuffer = (char *)malloc(sizeof(char)*1000);
                    memset(msgBuffer, '\0', sizeof(char)*1000);
                    char *peerIp;
                    peerIp = (char*)malloc(256*sizeof(char));
                    memset(peerIp, '\0', sizeof(char)*256);
                    peerIp = strdup(argVV[1]);

                    for(int i =0;i<count; i++)
                        {
                        //    char *sbuf = argVV[i];
                            strcat(msgBuffer,argVV[i]);
                            if(i<count-1)
                            strcat(msgBuffer," ");
                        }
                       // printf("msg to be sent %s and ip to be sent to %s\n",msgBuffer,peerIp);
                    int n = checkAndSend(msgBuffer,peerIp);
                    if(n==1)
                        printSuccess("SEND",NULL);
                    else
                        printError("SEND",NULL);
                }
                else
                {
                    printError("SEND",NULL);
                }
            }
        }
        else if(strcmp("BROADCAST",command) == 0)
        {
            if(isConnected)
            {
                if(count > 1)
                {
                    int n = sendMssg(fullMsg,SERVER_SOCKET);
                    if(n == -1)
                        printSuccess("BROADCAST",NULL);
                    else
                        printError("BROADCAST",NULL);

                }
                else
                    printError("BROADCAST",NULL);
            }
            else
                printError("BROADCAST",NULL);

        }
        else if(strcmp("LIST", command) == 0)
        {
            if(isConnected)
            { 
                cse4589_print_and_log("[LIST:SUCCESS]\n");
                for(int i =1;i<=CNOC;i++)
                {
                 // if(clientList[i].status == 1)
                  cse4589_print_and_log("%-5d%-35s%-20s%-8d\n",i,clientList[i].hostname,clientList[i].ip_addr,clientList[i].port_num);
                }
                cse4589_print_and_log("[LIST:END]\n");
            }
            else
                printError("LIST",NULL);

        }
        else if(strcmp("BLOCK", command) == 0)
        {
            if(isConnected)
            {
                char *peerIp;
                peerIp = (char*)malloc(256*sizeof(char));
                memset(peerIp, '\0', sizeof(char)*256);
                peerIp = strdup(argVV[1]);
                if(validIP(peerIp))  
                {  
                  //  bool isBlocked = checkClientBlock(peerIp);
                    /*if(isBlocked)
                    {
                        printError("BLOCK",NULL);
                    }*/
                    if(BLOCK_ARRAY[ipToNumber(CLIENT_IP)][ipToNumber(peerIp)] == 1)
                        printError("BLOCK",NULL);
                    else
                    {
                    int n = sendMssg(fullMsg,SERVER_SOCKET);
                    printSuccess("BLOCK",NULL);
                    BLOCK_ARRAY[ipToNumber(CLIENT_IP)][ipToNumber(peerIp)] = 1; 
                    /*clientList[getUserNumber()].NOBU++;    // have to go though this for unblocking
                    clientList[getUserNumber()].blockUsers[clientList[getUserNumber()].NOBU] = (char *)malloc(sizeof(char)*256);
                    memset(clientList[getUserNumber()].blockUsers[clientList[getUserNumber()].NOBU],'\0',sizeof(char)*256);
                    clientList[getUserNumber()].blockUsers[clientList[getUserNumber()].NOBU] = strdup(peerIp);
                    */

                    
                    }
                }
                else
                    printError("BLOCK",NULL);
            }
            else
                printError("BLOCK",NULL);
        }
        else if(strcmp("UNBLOCK",command) == 0)
        {
            if(isConnected)
            {
                char *peerIp;
                peerIp = (char*)malloc(256*sizeof(char));
                memset(peerIp, '\0', sizeof(char)*256);
                peerIp = strdup(argVV[1]);
                if(validIP(peerIp))  
                {  
                    if(BLOCK_ARRAY[ipToNumber(CLIENT_IP)][ipToNumber(peerIp)] == 0)
                        printError("UNBLOCK",NULL);
                    else
                    {
                    int n = sendMssg(fullMsg,SERVER_SOCKET);
                    printSuccess("UNBLOCK",NULL);
                    BLOCK_ARRAY[ipToNumber(CLIENT_IP)][ipToNumber(peerIp)] = 0; 
                    }
                }
                else
                    printError("UNBLOCK",NULL);
            }
            else
                printError("UNBLOCK",NULL);
        }
}

/*int getUserNumber()
{
    for(int i =1;i<=CNOC;i++)
    {
        if(strcmp(clientList[i].ip_addr,CLIENT_IP)== 0)  // add CLIENT_IP
        {
            printf("this is current user %d",i);
            return i;
        }
    }
return -1;
}
*/
bool validIP(char *pIp)
{
    for(int i =1;i<=CNOC;i++)
    {
        if(strcmp(pIp,clientList[i].ip_addr) == 0)
            {
                printf("%s\n","valid");
                return true;
            }
    }
    return false;
}

/*bool checkClientBlock(char *peerIp)
{
    if(getUserNumber() != -1)
    {
        for(int j =1; j<=clientList[getUserNumber()].NOBU;j++)
        {
       //  check for self blocking
           if(strcmp(clientList[getUserNumber()].ip_addr,peerIp) == 0)
             return true;
            if(strcmp(clientList[getUserNumber()].blockUsers[j],peerIp) == 0)
            {   
                printf("is already blocked\n");
                return true;
            }

        }
        printf("elligible for blocking\n");
        return false;
    }
    else
    {
        printf("THIS SHOULD NOT HAPPEN\n");
        return true;
    }
    
}
*/
int checkAndSend(char *msgToClient,char *peerIp)
{

    bool isPresent = false;
          //          printf("CNOC is %d",CNOC);
                    for(int j =1; j <= CNOC; j++)
                    {
                        //printf("stored ip=%sclient ip =%sclient\n", clientList[j].ip_addr,peerIp);
                        if(strcmp(clientList[j].ip_addr,peerIp)==0)
                        {
                            isPresent = true;
                        }
                    }
                    if(isPresent)    //checking if it is present in the client's list or not
                    {
                    //    printf("EUREKA!\n");
                        int n =sendMssg(msgToClient,SERVER_SOCKET);  
                        if(n!=-1)
                            return 1;
                        else
                            return 0;
                    }
                    else
                    {
              //          printf("not present in the list\n");
                        return 0;
                    }
}


//-------------------------------------------------------------------------------------------------------------
//----------------------------------------Start : Common functions---------------------------------------------
//-------------------------------------------------------------------------------------------------------------


int sendMssg(char *mssg,int sockeT)
{
   // printf("WELL WELL WELL %s",mssg);
    int n = send(sockeT,mssg, sizeof(char)*strlen(mssg),0);
    return n;
}

void printSuccess(char *command,char *mssg)
{
    cse4589_print_and_log("[%s:SUCCESS]\n",command);
    if(mssg != NULL)
        cse4589_print_and_log("%s",mssg);
    cse4589_print_and_log("[%s:END]\n",command);
}   

void printError(char *command,char *mssg)
{
    cse4589_print_and_log("[%s:ERROR]\n",command);
    if(mssg != NULL)
        cse4589_print_and_log("%s\n",mssg);
    cse4589_print_and_log("[%s:END]\n",command);
}

char* getHostName(char *ipaddress)
{
    char *hostname;

    if(strcmp(ipaddress,"128.205.36.46")==0)
        hostname = "stones.cse.buffalo.edu";
    else if(strcmp(ipaddress,"128.205.36.36") == 0)
        hostname = "underground.cse.buffalo.edu";
    else if(strcmp(ipaddress,"128.205.36.35") == 0)
        hostname = "embankment.cse.buffalo.edu";
    else if(strcmp(ipaddress,"128.205.36.34") == 0)
        hostname = "euston.cse.buffalo.edu";
    else if(strcmp(ipaddress,"128.205.36.33") == 0)
        hostname = "highgate.cse.buffalo.edu";
    else 
        hostname = "localhost";

    char *host = hostname;
    return host;
}

bool ipChecker(char *ipaddress)
{
    if(strcmp(ipaddress,"128.205.36.46")==0)
        return true;
    else if(strcmp(ipaddress,"128.205.36.36") == 0)
        return true;
    else if(strcmp(ipaddress,"128.205.36.35") == 0)
        return true;
    else if(strcmp(ipaddress,"128.205.36.34") == 0)
        return true;
    else if(strcmp(ipaddress,"128.205.36.33") == 0)
        return true;
    else if(strcmp(ipaddress,"127.0.0.1") == 0)
        return true;
    else
    return false;
}

int ipToNumber(char * ipaddress)
{
    if(strcmp(ipaddress,"128.205.36.46")==0)
        return 1;
    else if(strcmp(ipaddress,"128.205.36.36") == 0)
        return 2;
    else if(strcmp(ipaddress,"128.205.36.35") == 0)
        return 3;
    else if(strcmp(ipaddress,"128.205.36.34") == 0)
        return 4;
    else if(strcmp(ipaddress,"128.205.36.33") == 0)
        return 5;
    else
        return 0;
}
