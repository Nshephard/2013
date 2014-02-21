//Nick Shephard SID#10872355
//CPTS 455, Fall 2013
//Prof: Hauser
//Project #2: Router Simulator 

//Libraries
#include "readrouters.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>

//Globals
routerInfo *rTable; //Router Info Table
linkInfo *lInfo; //Link Info Table
neighborSocket *nSockets; //Holds neighbor's socket fds
fd_set readFDs; //Used by select() for polling fds
int ready_Socks, number_sock_fds, recvSocket, fdcount; //Socket fd globals
char thisRouter[]; //Holds Current Router

typedef struct RouterTable 
{
	char router[10];
	char dest[10];
	char nexthop[10];
	int   cost;
} routerTable; 
routerTable localRouterTable[10]; //Local RouterTable struct
int routerTableCount = 0; //Local routerTable count

//Prototypes
int set_FDS();
int get_DestIndex(char *router);
char *get_RouterName(int fd);
int get_Port(char *r);
void print_RouterTable(char *router);
int get_Cost(char *router);
int table_Check(char *router);
void update_Neighbors();


//Main Loop
int main(int argc, char **argv)
{
	int i = 0, port, tempCount;
	int index, newCost, tempCost, temp;
	char buf[128] = {'\0'}, *neighbor, *sentFrom;
	struct hostent *hp;
	struct sockaddr_in sin;
	struct timeval timeout; //Needed for Select timeouts
	
	//Usage Check
	if(argc < 3)
	{
		printf("\nUsage: Router Testdir Routername\n");
		return -1; //Exit with Error
	}


	//Initial Setup from supplied functions
	printf("This is Router: %s\n", argv[2]);
	strcpy(thisRouter,argv[2]);
	rTable = readrouters(argv[1]); //Setup initial routerTable
	lInfo = readlinks(argv[1], argv[2]); //Setup initial Links
	nSockets = createConnections(argv[2]); //Socket Connections per router
	if((port = get_Port(argv[2])) == -1)//Hook up correct port matching this router name
	{
		printf("ERROR: Could Not Find a Port for this Router!\n");
		return -1; //Exit with Error
	} 
	hp = gethostbyname("127.0.0.1"); //localhost

	//Establish port for recvSocket
	bzero((char*)&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	bcopy( hp->h_addr, (char*)&(sin.sin_addr), hp->h_length );
	sin.sin_port = htons(port);

	//Create recvSocket
    	if( (recvSocket = socket(PF_INET, SOCK_DGRAM, 0) ) <  0)
    	{
    	    perror("ERROR: Could Not Create recvSocket!");
    	    return -1; //Exit with error
   	 }

	//Bind recvSocket to its respective port
	if ((bind(recvSocket, (struct sockaddr *)&sin, sizeof(struct sockaddr_in))) < 0 )
    	{
        	perror("ERROR: Could Not Bind recvSocket!");
        	return -1;//Exit with error
    	}

	//Formatting for Select(), zero out the readFDs, add fd's for each socket and return max # fd's
	FD_ZERO(&readFDs);
	number_sock_fds = set_FDS();
	FD_SET(recvSocket, &readFDs);
	number_sock_fds = recvSocket;
	//printf("Total Links (Including Listenting Socket): %d\n", number_sock_fds);

	//For each link, initialize local routerTable with the data
	tempCount = get_LinkCount(); 
	for(i=0; i < tempCount; i++){
		strcpy(localRouterTable[i].router, argv[2]);
		strcpy(localRouterTable[i].nexthop, lInfo[i].router);
		strcpy(localRouterTable[i].dest, lInfo[i].router);
		localRouterTable[i].cost = lInfo[i].cost;
		routerTableCount++;
	}

	//Print initial table
	printf("Routing Table %s Initialized:\n", argv[2]);
	print_RouterTable(thisRouter);
	printf("**********************************************\n");

	//Main loop
	while(1)
	{	
		timeout.tv_sec = 30; //Set timeout time of 30secs
		timeout.tv_usec = 0;

		//ready_Socks holds select's returned fd, PARAM1: nfds = highest# fd + 1, PARAM2:readfds, PARAM5: timeout
		ready_Socks = select(number_sock_fds+1, &readFDs, NULL, NULL, &timeout);

		if(ready_Socks == 0) //If a timeout occured, call update_Neighbors
		{ 
			//printf("Timout Occured\n");
			update_Neighbors();
		}

		for(i = 0; i <= number_sock_fds; i++) //For all the fds, check if select marked them set (incoming)
		{
			if(FD_ISSET(i, &readFDs)) //If fd is set, handle it
			{
				bzero(buf, 128); //Clear out temp buffer
				recv(i, buf, sizeof(buf), 0); //Put incoming message into buffer
				printf("Received: %s\n", buf); //Tracing to see the incoming message

				if(strncmp(buf, "P", 1) == 0) //If P = Print Router's Table
				{
				printf("**********************************************\n");
					print_RouterTable(thisRouter);
				printf("**********************************************\n");
				}
				

				else if( strncmp(buf, "U", 1) == 0) //If U = Update Check
				{
					sentFrom = get_RouterName(i); //Get Neighbor's FD
					strtok(buf, " ");
					neighbor = strtok(NULL, " "); //Get Neighbor's Name
					newCost = atoi(strtok(NULL, " ")); //Get Cost 
			
					//printf("table_Check = %d\n", table_Check(sentFrom));
					//printf("Neighbor Index = %d, Name = %s\n", index, sentFrom);
					tempCost = get_Cost(sentFrom) + newCost; //Compute tempCost incase update results in shorter path
					//printf("TEMP COST: %d\n", tempCost);
					if (((table_Check(neighbor)) == -1) && (strcmp(thisRouter,neighbor) != 0))//if neighbor is NOT currently in table add it to table + not sending to itself
					{
						strcpy(localRouterTable[routerTableCount].router, thisRouter);
						strcpy(localRouterTable[routerTableCount].dest, neighbor);
						strcpy(localRouterTable[routerTableCount].nexthop, sentFrom);
						localRouterTable[routerTableCount].cost = newCost + get_Cost(sentFrom);
						
						printf("NEW ENTRY (%s - dest: %s cost: %d nexthop: %s)\n",thisRouter, localRouterTable[routerTableCount].dest, localRouterTable[routerTableCount].cost, localRouterTable[routerTableCount].nexthop);	
						routerTableCount++;
						//printf("New Routing Table Count: %d\n", routerTableCount);
						temp = get_Count(); //Temp = number of links (original neighbors) for sending
						sprintf(buf, "U %s %d", neighbor, (newCost + get_Cost(sentFrom)));		
						for(i = 0; i < temp; i++) //Send update messages to each neighbor
						{
							//printf("Sent new entry Msg: %s\n", buf);
							if(send(nSockets[i].socket, buf, strlen(buf),0 ) < 1)
							{
							printf("ERROR: Could Not Send Message to: %s\n", nSockets[i].neighbor);
							}			
						}
					}
					//Exists in Table Check Cost + Not a distance to itself
					else if (tempCost < get_Cost(neighbor) && (strcmp(thisRouter,neighbor) != 0))
					{ 
						index = get_DestIndex(neighbor);
						strcpy(localRouterTable[index].dest, neighbor);
						strcpy(localRouterTable[index].nexthop, sentFrom);
						localRouterTable[index].cost = tempCost;
						printf("FOUND CHEAPER PATH (%s - dest: %s cost: %d nexthop: %s)\n", thisRouter, localRouterTable[index].dest, localRouterTable[index].cost, localRouterTable[index].nexthop);
						
						//Same loop as above, update occured -> Send to all neighbors
						temp = get_Count();
						sprintf(buf, "U %s %d", neighbor, localRouterTable[index].cost);		
						for(i = 0; i < temp; i++)
						{
							//printf("Sent update Msg: %s\n", buf);
							if(send(nSockets[i].socket, buf, strlen(buf),0 ) < 1)
							{
								printf("ERROR: Could Not Send Message to: %s\n", nSockets[i].neighbor);
							}			
						}
					}
			}

			else if(strncmp(buf, "L", 1) == 0) //Link Update Message
			{
				//Get Message Params
				strtok(buf, " ");
				neighbor = strtok(NULL, " ");
				newCost = atoi(strtok(NULL, " "));
				index = get_DestIndex(neighbor);

				//printf("Index of neighbor updated: %d\n", index);
				localRouterTable[index].cost = newCost; //Update the link to new cost
				strcpy(localRouterTable[index].nexthop, neighbor);
				printf("LINK COST CHANGE (%s - dest: %s cost: %d nexthop: %s)\n", thisRouter, localRouterTable[index].dest,localRouterTable[index].cost, localRouterTable[index].nexthop);

				//Send update to neighbors
				temp = get_Count();		
				sprintf(buf, "U %s %d", neighbor, localRouterTable[index].cost);		
				for(i = 0; i < temp; i++)
				{
					//printf("Sent update Msg: %s\n", buf);
					if(send(nSockets[i].socket, buf, strlen(buf),0 ) < 1)
					{
						printf("ERROR: Could Not Send Message to: %s\n", nSockets[i].neighbor);
						}			
					}
				}		
			}
		}
	
		//Clear out the FDs, and reset for next wave of incoming msgs on sockets
		FD_ZERO(&readFDs);
		number_sock_fds = set_FDS();
		FD_SET(recvSocket, &readFDs);
		number_sock_fds = recvSocket;
	}
	return 0;
}

//Function: set_FDS - Uses FD_SET to set up each neighbor socket and add to the fd_set, returns the MAX_FD for select usage
int set_FDS()
{
	int max_FD = -1, i = 0, temp = get_Count();

	for(i = 0; i < temp; i++)
	{
		FD_SET(nSockets[i].socket, &readFDs); //Add to fd_set
		if(nSockets[i].socket > max_FD) //max_FD check
		{
			max_FD = nSockets[i].socket;
		}
	}

	return max_FD; //Return our Max_FD
}

//Function: get_DestIndex - Helper function to return the index of neighbor in local table
int get_DestIndex(char *router)
{
	int i;

	for(i=0; i < routerTableCount; i++)
	{
		if( strcmp(router, localRouterTable[i].dest) == 0)
		{
			return i;
		}
	}
	return -1;	
}

//Function: get_RouterName - Helper function to convert a file descriptor to a neighbor's name
char *get_RouterName(int fd)
{
	int i, temp = get_Count();

	for(i=0; i < temp; i++)
	{
		if(nSockets[i].socket == fd)
		{
			return nSockets[i].neighbor;
		}
	}
	return NULL;
}

//Function: get_Port - Helper function for setting up correct ports to input_file: routers in testdir
int get_Port(char *r)
{
	int i = 0, temp = get_RouterCount();

	for(i = 0; i < temp; i++)
	{
		if(strncmp(r, rTable[i].router,1) == 0)
		{
			return rTable[i].baseport;
		}
	}
	return -1;
}

//Function: print_RouterTable - simply prints the local router table
void print_RouterTable(char *router)
{
	int i = 0;

	for(i=0; i < routerTableCount; i++)
	{
		//Matches requirements in the form: <r> – dest: <d> cost: <c> nexthop: <n>
		printf("%s - dest: %s cost: %d nexthop: %s\n", router, localRouterTable[i].dest,localRouterTable[i].cost, localRouterTable[i].nexthop);
	}
}

/* gets the cost to a neighbor */
int get_Cost(char *router){
	int i;

	for(i = 0; i < routerTableCount; i++){
		if( strncmp(localRouterTable[i].dest, router, 1 ) == 0){
			return localRouterTable[i].cost;
		}
	}

	return 0;

}
//Function: table_Check - Helper functions for update messages, simply returns the index of destination in table, or -1 if the destination is not in the table
int table_Check(char *router)
{
 	int i;

 	for(i=0; i < routerTableCount; i++)
 	{
		//printf("Comparing %s with %s\n", router, localRouterTable[i].dest);
 		if( strncmp(router, localRouterTable[i].dest, 1) == 0)
 		{
 			return i; //Return dest's index
 		}
 	}

 	return -1; //return -1 if not in local table
 }

//Function: update_Neighbors - Timeout function to send the localRouterTable's entries to its neighbors
void update_Neighbors()
{
	int i, j, temp = get_Count();
	char buf[128] = {'\0'};
	
	for (i=0; i < temp; i++)
	{
		for (j = 0; j < routerTableCount; j++)
		{
			//printf("I: %d\n", i);
			sprintf(buf, "U %s %d", localRouterTable[j].dest, localRouterTable[j].cost);					
			//printf("Sent: %s to router %s\n", buf, get_RouterName(nSockets[i].socket));
			//Sent out messages to each neighbor's socket
			if(send(nSockets[i].socket, buf, strlen(buf),0 ) < 1)
			{
				printf("ERROR: Could Not Send Message to: %s\n", nSockets[i].neighbor);
			}
		}
			
	}
}
