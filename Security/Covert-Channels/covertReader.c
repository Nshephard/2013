/* Nick Shephard #10872355, CPTS 427 - Proj2: covertReader.c
 * 
 * Creates temporary file 2 to signal to the covertWriter that it has read current covert channel buffer
 *
 * Usage :
 *         covertReader -o output_file
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#define ENCRYTPION_KEY 15 /* These globals must match up with Writer's to ensure sucessful encryption and delivery of data */
#define MAX_TRANSFER_SIZE 20 

char *inputFile = "/tmp/1";
char *outputFile;

/* Open a file */
FILE * open_file (char *fileName, char *mode)
{
  FILE *fP = NULL;

  if ((fP = fopen(fileName, mode)) == NULL)
    perror(fileName);

  return fP;
}

/* Error usage msg */
void usage (char *progName)
{
  fprintf(stderr, "\nUsage:\n"
                  "%s -o output_file \n\n",
                  progName);
  exit(1);
}

/* Parse command line arguments, and set appropriate global variables. */
int parse_arguments (int argc, char **argv)
{
  int i, ch;
  int mandArgs = 0;	/* Flag to keep track of mandatory arguments  */

  opterr = 0;

  while ((ch = getopt(argc, argv, "o:")) != -1)
    switch (ch)
    {
      case 'o':
        outputFile = optarg;
        mandArgs |= 1;
        break;

      case '?':
        /* Just print error message and continue? */
        if (optopt == 'o')
                fprintf(stderr, "Option -%c requires an argument.\n", optopt);
        else if (isprint(optopt))
                fprintf(stderr, "Unknown option `-%c'.\n", optopt);
        else
                fprintf(stderr,
                        "Unknown option character `\\x%x'.\n",
                        optopt);
        break;

      default:
        /* Ignore. */
        break;
    }

  for (i = optind; i < argc; i++)
    printf ("Non-option argument %s\n", argv[i]);

  if (mandArgs != 1)
    return -1;	/* Minimum needed (valid) information not there. */
  else
    return 0;
}

int main (int argc, char **argv)
{
  FILE *fIn, *fOut;
  int ch, i, flag = 0;
  char temp[MAX_TRANSFER_SIZE];
  /* Check Setup */
  if (!(argc == 3) || parse_arguments(argc, argv) == -1)
    usage(argv[0]);

  fOut = open_file(outputFile, "w");

  /* Wait until writer has posted his file in /tmp */
  if ((fIn = fopen(inputFile, "r")) == NULL)
  {
	printf("Waiting for covertWriter...\n");
  }

  while (fIn == NULL)
  {
	fIn = fopen(inputFile, "r");
  }

  
  /* Read / Signal Writer Loop */
  while(1)
  {
	sleep(1);
	fIn = fopen(inputFile, "r");
	if( fIn != NULL)
	{
        	for (i = 0; i< MAX_TRANSFER_SIZE; i++)
        	{
        	  if ((ch = fgetc(fIn)) == EOF)
		  {
        	  flag = 1;
		  break;
		  }
          	 temp[i] = ch;
                 temp[i] += ENCRYTPION_KEY; /* Read + Decrypt each char into buffer */
       		}
		if (flag == 1) /* If remainder of file does no fill buffer */
		{
		  temp[i] = '\0';
	 	  printf("%s\n", temp);
		  fprintf(fOut, "%s", temp);
		  fclose(fIn);
		}
		else
		{	
		temp[MAX_TRANSFER_SIZE] = '\0';
	 	printf("%s\n", temp);
		fprintf(fOut, "%s", temp);
		fclose(fIn);
		}
  		fopen("/tmp/2","w"); /* Signal to writer to update */
		sleep(1);
		remove("/tmp/2");
	}
	else
	if (fopen(inputFile, "r") == NULL)
		break;
  }
/* Destroy the evidence when channel usage complete! */
  remove("/tmp/2"); 
  return 0;
}
