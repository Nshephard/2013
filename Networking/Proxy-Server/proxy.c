//Nick Shephard
//SID #10872355
//CPTS 455, Fall 2013
//Prof: Hauser
//Project #3: LAN Proxy Server

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <netdb.h>

#define PORT 1234
#define BUF_SIZE 1024*1024

//Prototypes
int get_ProxySock(int port);
int get_serverSock(char *hostname, int port);
void get_HeaderInfo(char *uri, char *hostname, char *pathname, int *port);

int main ()
{
	int ProxySock, clientSock, serverSock, serverPort, pid = 0, bytes_read = 0, length;
	char client_buf[BUF_SIZE], server_buf[BUF_SIZE], uri[1024], version[1024], request[1024], hostname[1024], pathname[1024];
	char clientIP[INET_ADDRSTRLEN]; //New Client sock data
	struct sockaddr_in clientADDR; 
	socklen_t clientADDR_length;
	char *temp2, *temp3;
	char *temp; //Temp helps parsing headers for Connection: keep-alive and Content-Length modification
	int childCount = 0; //Required for Hauser's waitpid setup

	//Create the Proxy Server
	ProxySock = get_ProxySock(PORT);
	//printf("Proxy Sock: %d\n", ProxySock);
	
	if (ProxySock < 0)
	{
		exit(1);
	}
	printf("Nick Shephard's Proxy Server Has been created on Port:%d\n\n", PORT);
	while(1)
	{
		//Step 1: Get Request From Client
		clientADDR_length = sizeof(clientADDR);
		clientSock = accept(ProxySock, (struct sockaddr*) &clientADDR, &clientADDR_length);
		
		if (clientSock < 0)
		{
			printf("\nERROR: Socket error accepting client!\n");
			break;
		} 
		
		//Convert numeric address for client into text string for parsing into header
		if (inet_ntop(AF_INET, &clientADDR.sin_addr.s_addr, clientIP, sizeof(clientIP)) != 0)
		{
			//printf("New Client: Socket = %d, IP = %s, Port = %d\n", clientSock, clientIP, clientADDR.sin_port);
			
			if ((pid = fork()) == 0) //If sucessfully forked a child
			{
				//bzero(&client_buf, BUF_SIZE);
				//bzero(&server_buf, BUF_SIZE);
				close(ProxySock); //Close proxysocket as we have a request
				//Handle the Request
				while(1)
				{
					bytes_read = recv(clientSock, client_buf, BUF_SIZE, 0);
					if (bytes_read < 0)
					{
						printf("ERROR: Bad receive from client!\n");
						break;
					}
					else
					{
						printf("Received from Client:\n%s\n", client_buf);
					}
			
					sscanf(client_buf, "%s %s %s", request, uri, version);
					//INSERT FPRINTF TO OUTPUT HERE
					//printf("REQUEST TYPE: %s\n", request);
					//printf("URI: %s\n", uri);
					//printf("VERSION: %s\n", version);
					//printf("Hostname: %s\n", hostname);
					//printf("Pathname: %s\n", pathname);
					
					//Modify the Connection: keep-alive to Connection: close
					strcpy(server_buf, client_buf);
					temp3 = strstr(server_buf, "Connection: ");
					temp = strstr(client_buf, "keep-alive");
					temp2 = strchr(temp, '\n');
					memcpy(temp3, "Connection: close\r\n", strlen(temp3));
					strcat(server_buf, temp2);
					
					printf("\nSending to Server:\n%s", server_buf);
					get_HeaderInfo(uri, hostname, pathname, &serverPort);
			
					//Send new request to server
					serverSock = get_serverSock(hostname, serverPort);
					//printf("The Server Sock is: %d\n\n", serverSock);
					bytes_read = send(serverSock, server_buf, BUF_SIZE, 0);
					printf("The Server Sent Back: \n%s", client_buf);
					
					bzero(client_buf, BUF_SIZE);
					//GET Request, handle all 
					if (strncmp(request, "GET", 3) == 0)
					{
						while ((bytes_read = recv(serverSock, client_buf, BUF_SIZE, 0)) > 0)
						{
							printf("%s", client_buf);
							bytes_read = send(clientSock, client_buf, BUF_SIZE, 0);
							bzero(client_buf, BUF_SIZE);
						}
					}
			
					//Handle POST requests, use the content length as data loop limit
					else if (strncmp(request, "POST", 4) == 0)
					{
						temp = strstr(client_buf, "Content-Length:");
						temp = strstr(temp, " ");
						length = atoi(temp);
						printf("\nThe Content Length of the POST: %d\n", length);
						while (length > 0)
					
						{
							bytes_read = recv(serverSock, client_buf, BUF_SIZE, 0);
							printf("%s", client_buf);
							bytes_read = send(clientSock, client_buf, BUF_SIZE, 0);
							bzero(client_buf, BUF_SIZE);
							length = length - bytes_read;
						}
					}
					//Clear buffers and close client socket fd
					bzero(client_buf, BUF_SIZE);
					bzero(server_buf, BUF_SIZE);
					close(clientSock);
					break;
			}
		}
		else //if parent
		{
			close(clientSock);
			close(serverSock);
		}
		
		//wait3(NULL, WNOHANG, (struct rusage *)client_buf);
		childCount++;
		//Code here from Professor Hauser, forking server example
		while(childCount)
		{
			pid = waitpid(-1, NULL, WNOHANG);
			if(pid == 0)
				break;
			else
				childCount--;
		}
	}
}
//Should never get here
return 0;
}

//Function: get_ProxySock()
//Establishes the listening socket on the Proxy for clients to connect to
//Returns the fd of socket or -1 if error
int get_ProxySock(int port)
{
	int ProxySock;
	struct sockaddr_in proxyADDR;
	
	if ((ProxySock = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
	{
		printf("ERROR: Could not create ProxyServer socket!\n");
		return -1;

	}

	bzero((char *) &proxyADDR, sizeof(proxyADDR));
	proxyADDR.sin_family = AF_INET;
	proxyADDR.sin_addr.s_addr = htonl(INADDR_ANY);
	proxyADDR.sin_port = htons((unsigned short) PORT);
	
	//Bind a socket to Proxy's Address
	if (bind(ProxySock, (struct sockaddr*) &proxyADDR, sizeof(proxyADDR)) < 0)
	{
		printf("ERROR: Could not bind a ProxyServer socket!\n");
		return -1;
	}
	
	//Set newly established socket to listen for incoming connections
	//Set max fd's to listen to many requests ie- for CNN or bing
	if (listen(ProxySock, 100) < 0)
	{
		printf("ERROR: ProxyServer could not listen!\n");
		return -1;
	}
	return ProxySock;	
}

//Function: get_serverSock()
//Based on input hostname and port creates a socket fd + connection to server
//Return the fd of new server socket or -1 for error
int get_serverSock(char *hostname, int port)
{
	int ServerSock;
    struct hostent *hp;
    struct sockaddr_in serverADDR;

	//Setup a new socket for the server connection
    if ((ServerSock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
		return -1;
	}

    //Get hostent ptr via function gethostbyname()
    if ((hp = gethostbyname(hostname)) == NULL)
    {
		return -1;
	}
	
	//printf("Hostname: %s\n", hostname);
	//Fill in server ADDR entries
    bzero((char *) &serverADDR, sizeof(serverADDR));
    serverADDR.sin_family = AF_INET;
	bcopy((char *) hp->h_addr_list[0], (char *) &serverADDR.sin_addr.s_addr, hp->h_length);
    serverADDR.sin_port = htons(port);

    //Connect with server to ServerSock fd
    if (connect(ServerSock, (struct sockaddr *) &serverADDR, sizeof(serverADDR)) < 0)
    {
		return -1;
	}
    return ServerSock;
} 
 
//Function: get_HeaderInfo()
//Gets hostname, pathname and port from the URI of client's request to aid in server setup/printing
void get_HeaderInfo(char *uri, char *hostname, char *pathname, int *port)
{
    char *host_front, *host_back, *path;
    int length;

    if (strncasecmp(uri, "http://", 7) != 0) 
    {
		hostname[0] = '\0';
    }
       
    //Get Hostname (offset 7 from uri)
    host_front = uri + 7;
    host_back = strpbrk(host_front, " :/\r\n\0");
    length = host_back - host_front;
    strncpy(hostname, host_front, length);
    hostname[length] = '\0';
    
	//Use default outgoing network port 80
    *port = 80;
    
    //Get pathname
    path = strchr(host_front, '/');
    if (path == NULL) 
    {
		pathname[0] = '\0';
    }
    else 
    {
		path++;	
		strcpy(pathname, path);
    }
}
