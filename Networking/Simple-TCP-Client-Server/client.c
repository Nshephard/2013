//Nick Shephard
//SID# 10872355
//CPTS 455 Networking: Project 1
//Prof: Hauser
//CLIENT.C

//Libraries 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>

#define MAX_BUF 512

//Prototypes
int client_init(char *argv[]);
int input_ID(char *input);
int input_Name(char *input);
int server_Response(char* input);

//Globals
struct hostent *host_ptr;              
struct sockaddr_in  server_addr; 
int sock, r, Server_IP, Server_Port; 

//Main function
main(int argc, char *argv[ ])
{
  int n, flagID = 0, flagName = 0, login_attempts = 0, verified = 0, pw_attempts = 3;
  char input[MAX_BUF], id[MAX_BUF], name[MAX_BUF], pass[MAX_BUF];
  ushort pass_length;

  if (argc < 3) //Make sure host and port specified in cmdline at run
  {
     printf("ERROR: Please Include a Hostname and Port!\n");
     exit(1);
  }

  //Setup client
  client_init(argv);

  while (1)
  {  
    while(verified != 1)
    {
        //Get a valid ID and name from user
        flagID = 0; flagName = 0;
        while(flagID == 0 && flagName == 0) //do not proceed until valid
        {
          flagID = input_ID(id);
          flagName = input_Name(name);
        }
        
		//Send id + name to server
        n = write(sock, id, MAX_BUF);
        n = write(sock, name, MAX_BUF);

        //clear buffer and read server's response
        bzero(input, MAX_BUF);
        n = read(sock, input, MAX_BUF);
        input[strlen(input) - 1] = 0;
		
		//check if server verified your ID and PW sent
        verified = server_Response(input);
        if (verified == 0) //if verified = 0, server has denied access
        {
            bzero(input, MAX_BUF);
            n = read(sock, input, MAX_BUF);
            input[strlen(input)-1] = 0;
            if (server_Response(input) == 2) //Check to see if server has shut us down (after 3 attempts)
            {
                printf("Server Terminating Connection! Oh No!\n");
                close(sock); //Close client's socket
                exit(1);
            }
            
        }
        login_attempts++;
		printf("Login Attempts: %d\n", login_attempts);
    }

    //If we have gotten here, then server has verified our credentials, time for the password check
    while( pw_attempts > 0 )
    {
      printf("Please Enter your Password: "); //Get PW from user and remove NULL (not sent for PW)
      fgets(input, MAX_BUF, stdin);
      pass_length = strlen(input)-1;
      sprintf(pass,"%d", htons(pass_length)); //convert password to 2-byte binary number via sprintf
      write(sock, pass, MAX_BUF); //send  htons#
      write(sock, input, MAX_BUF); //send the password
      read(sock, input, MAX_BUF);  //read server's response to password
      input[strlen(input)-1] = 0; //get rid of newline in server's response
      if (server_Response(input) == 3) //If server_Response == 3 then we know the server didn't like our password
      {
        pw_attempts--; //reduce pw_attempts and try again!
      }
      else
      {
		printf("%s\n", input);
        printf("Congrats You Have Successfully Logged In! GOODBYE!\n"); //Confirm successful login client-side and close our socket
        close(sock);
        exit(1);
      }
    }
    
    if(pw_attempts == 0) //if client has failed 3 passwords, shut him out!
    {
      printf("ERROR: You have Provided an Incorrect Password 3 Times! Try Again Later.\n");
      close(sock);
      exit(1);
    }
    
    fgets(input, MAX_BUF, stdin); //get a new password from client and try it again!
    input[strlen(input)-1] = 0;  
    if (input[0] == 0) 
	{
       exit(0);
	}
}}

//Setup client connection
int client_init(char *argv[])
{
  char input[MAX_BUF];
  
  //check for valid hostname
  if ((host_ptr = gethostbyname(argv[1])) == 0)
  {
	 printf("ERROR: Could Not Find Host!\n");
     exit(1);
  }

  //Setup Server's IP and Port (Globals)
  Server_IP   = *(long *)host_ptr->h_addr;
  Server_Port = atoi(argv[2]);

  //Create new client socket
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
     printf("ERROR: Client Could Not Create a New Socket!\n");
     exit(1);
  }

  //Update server's struct values to be used by client
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = Server_IP;
  server_addr.sin_port = htons(Server_Port);

  //Connect to server's address via the fresh socket
  if ((r = connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr))) < 0)
  {
     printf("ERROR: Client Could Not Connect to Server!\n");
     exit(1);
  }
  
  //Read the "verification msg" (welcome msg) sent by the server
  read(sock, input, MAX_BUF);
  printf("%s", input); //print to show client he connected
}


//Prompts user for ID (if verified = 1, then we know our input ID is OK
//NOTE: Additional error-checking handled server-side!
int input_ID(char *input)
{
  int verified = 0;

  printf("Please Enter your 8-Digit USER ID: ");
  fgets(input, MAX_BUF, stdin);
  
  if((strlen(input)-1) == 8) //Remember to ignore the NULL character!
  {
    verified = 1;
  }

  return verified;
}

//Prompts user for Name (if verified = 1, we know we have a valid name (under 50 characters)
//NOTE: Additional error-checking handled server-side!
int input_Name(char *input)
{
  int verified = 0;

  printf("Please Enter Your Username: ");
  fgets(input, MAX_BUF, stdin);

  if(strlen(input) < 50)
  {
    verified = 1;
  }
  
  return verified;
}

//This function simply checks whatever the server sent back via return values for the client to interpret
int server_Response(char* input)
{	
    if(strncmp(input, "CONFIRM", 8) == 0) //Server sent confirm, return 1
    {
        return 1;
    }
    else if (strncmp(input, "DENY", 5) == 0) //Server Denied our creds, return 0
    {
        return 0;
    }
    else if(strncmp(input, "EXIT", 4) == 0) //Server kicking us off! return 2
    {
        return 2;
    }
    else if(strncmp(input, "UhOh!", 5) == 0) //Server prompting for PW, return 3
    {
      return 3;
    }
	else
	{
		return 0; //default return 0
	}
}
