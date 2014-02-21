//Nick Shephard
//SID# 10872355
//CPTS 455 Networking: Project 1
//Prof: Hauser
//SERVER.C

//Libraries
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>

#define  MAX_BUF 512

//Prototypes
int server_init(char *name, ushort port);
int verify_Credentials();
int verify_Client(int socket);
int verify_Password(int index, int pass_length);


//Globals
struct sockaddr_in  server_addr, client_addr, name_addr;
struct hostent *host_struct_ptr;

int  sock, newsock;                 
int  serverPort;     
int  r, length, n;                   

//2D Char arrays to hold clients info
char IDs[10][9] = {"12345678","10872355"};
char Names[10][20] = {"guest", "nick"};
char Passwords[10][20] = {"12345", "derp"};

//Main loop
main(int argc, char *argv[])
{
   char *hostname;
   char input[MAX_BUF];
   char welcome[40] = "Welcome to Nick Shephard's Server!\n"; //Welcome Msg
   ushort port;
   int id_num, pw_attempts = 0;
   ushort pass_length;

   if (argc == 1) //Make sure commandline supplied port
   {
	  printf("ERROR: Port Number Required!\n");
	  exit(1);
   }

   //using default localhost (only have 1 computer sorry), IP would go here
   hostname = "localhost";
   port = atoi(argv[1]);
   
   //Initialize server (Re-used 360 Networking code for setup)
   server_init(hostname, port); 

   //Polling Loop
   while(1)
   {
	 printf("Accepting New Clients...\n"); 

	 //Accept + Tracing
	 length = sizeof(client_addr);
	 if ((newsock = accept(sock, (struct sockaddr *)&client_addr, &length)) < 0)
	 {
		printf("ERROR: Accept Failed!\n");
		exit(1);
	 }
	 printf("I Have Connected to a New Client!\n");

	 n = write(newsock, welcome, MAX_BUF);    
	 
	 //ID_num holds the client's offset in the "database" (array)
	 id_num = verify_Client(newsock);

	 //Password checking, while bytes to read still exist
	 while(n!=0)
	 {
		 bzero(input, MAX_BUF); //clear buffer
		 n = read(newsock, input, MAX_BUF);
		 pass_length = atoi(input); //convert string read to int
		
		 //Check if PW valid via verify_password, if valid print specified output
		 if(verify_Password(id_num, ntohs(pass_length)) == 1)
		 {
			
			sprintf(input, "Congratulations %s; you have just revealed the password for %s to the world!\n", Names[id_num], IDs[id_num]);
			write(newsock,input, MAX_BUF);
			break;
		 }
		 //Allow up to 3 password tries (further handled on client-side)
		 write(newsock, "UhOh! Password Incorrect!\n", MAX_BUF);
		 pw_attempts++;
		 printf("Client has attempted %d passwords.\n", pw_attempts);
	 }

	 //If client dies, close its newsock created and wait for a new client
	 while(1)
	 {
	   if ((n = read(newsock, input, MAX_BUF))==0)
	   {
		   printf("My Client Died! Closing his socket.\n");
		   close(newsock);
		   pw_attempts = 0; //Reset pw attempts if subsequent clients connect
		   break;
	   }
 }
}
}

//Initialize Server
int server_init(char *name, ushort port)
{   
   if ((host_struct_ptr = gethostbyname(name)) == 0)
   {
	  exit(1);
   }
  
   //Create TCP Socket
   if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
   {
	  printf("ERROR: Could Not Create Socket!\n");
	  exit(1);
   }

   //Setup Server Struct
   server_addr.sin_family = AF_INET;                  //TCP
   server_addr.sin_addr.s_addr = htonl(INADDR_ANY);   //Host Addr
   server_addr.sin_port = htons(port);                //Assign server port (from client)

   //Bind
   if ((r = bind(sock,(struct sockaddr *)&server_addr, sizeof(server_addr))) < 0)
   {
	   printf("ERROR: Could Not Bind Socket!\n");
	   exit(1);
   }

   //Get socket's port #
   length = sizeof(name_addr);
   if ((r = getsockname(sock, (struct sockaddr *)&name_addr, &length)) < 0)
   {
	  printf("ERROR: Could Not Get Socketname!\n");
	  exit(1);
   }

   //Align Server Port
   serverPort = ntohs(name_addr.sin_port);   

   //Listen at this socket for a max of 5 clients
   listen(sock, 5);
}

//This function verifies a clients credentials (ID# and Name)
int verify_Credentials()
{
	int i, index = -1;
	int verified = -1;
	char id[MAX_BUF], name[MAX_BUF];

	//Read id number and replace newline with NULL
	n = read(newsock,id,MAX_BUF);
	id[strlen(id)-1] = 0;
	
	if (strlen(id) != 8) //If id is not length 8 we know invalid immediately
	{
		n = read(newsock,name,MAX_BUF); //perform dummy read to align with the subsequent name sent from client
		return -1;
	}
  
	//Scan though all 10 available Ids, if valid update index to valid #
	for(i=0; i < 10; i++ )
	{
		if(strncmp(IDs[i], id, 8) == 0) //IDs must be length 8
		{
			index = i;
		}
	}
	//If we have received a valid ID number, now check if name matches up
	if (index > -1)
	{
		//Read name + fix newline to NULL
		 n = read(newsock,name,MAX_BUF);
		 name[strlen(name)-1] = 0;
		 //printf("name: %s\n", name);
		 if(strncmp(Names[index], name, strlen(Names[index])) == 0) //if name DOES matchup at an index, update verified to return that index (for password checking)
		 {
			verified = index;
		 }
	}
	else 
	{
		n = read(newsock,name,MAX_BUF); //perform dummy read even if name is not valid (verified remains -1)
		name[strlen(name)-1] = 0;
		//printf("name: %s\n", name);
	}
	return verified;
}
//This function handles communicating with the client in regards to their credentials (ID + name)
int verify_Client(int socket)
{
  int index, newsock;
  int verified = 0, login_attempts = 0;

  newsock = socket; //update newsock to client's socket

  while(verified == 0) //While client still invalid
  {
		 index = verify_Credentials(); //check ID# + Name via verify_Credentials
		 if( index == -1 )
		 {
			verified = 0;
			printf("ERROR: Invalid Credentials!\n");
		 }
		 else //if index is valid, pass client along
		 {  
			  verified = 1;
			  printf("VERIFIED USER\n");
		 }

		 if ( verified == 1 ) //Pass client along to password step via CONFIRM msg
		 {
			write(newsock, "CONFIRM\n", MAX_BUF);
			return index;
		 }
		 else	//Let client know he has been denied via DENY message
		 {
			write(newsock, "DENY\n", MAX_BUF);
			login_attempts++;
			if (login_attempts == 3) //If client has tried 3 times unsuccessfully, kick him off the server!
			{
			   write(newsock, "EXIT\n", MAX_BUF);
			   return -1;
			}
			else //If client has remaining attempts simply send back 1 deny
			{
				write(newsock, "DENY\n", MAX_BUF);
			}
		 }
	 }
	 return index;
}

//This function works in tandem with the index returned from verify_Credentials to confirm a matching password (returns 1 if verified, -1 if not)
int verify_Password(int index, int pass_length)
{
  char password_current[MAX_BUF];
  int verified = -1;

  bzero(password_current, MAX_BUF); //Clear buffer and read current password attempt
  read(newsock, password_current, MAX_BUF);


	//If the password matches up with current attempt, verify it! (NOTE passwords do not send NULL char at end of string)
	if(strncmp(Passwords[index], password_current, pass_length) == 0)
    {
		verified = 1;
	}
	
	return verified;
}
