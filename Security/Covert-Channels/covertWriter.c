/* Nick Shephard #10872355, CPTS 427 - Proj2: covertWriter.c
 * 
 * Fills the file "1" located in the tmp directory with bytes of data from the confidential input file, updating once file "2" is present.
 *
 * Usage :
 *         covertWriter -i input_file
 */

#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#define ENCRYTPION_KEY 15
#define MAX_TRANSFER_SIZE 20 /* UPDATE HERE FOR LARGER CHANNEL BUFFER, Also must update the covertReader's globals under same name to match */

char *inputFile = NULL;
char *outputFile = "/tmp/1";

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
                  "%s -i input_file \n\n",
                  progName);
  exit(1);
}

/* Parse command line arguments, and set appropriate global variables. */
int parse_arguments (int argc, char **argv)
{
  int i, ch;
  int mandArgs = 0;	/* Flag to keep track of mandatory arguments  */

  opterr = 0;

  while ((ch = getopt(argc, argv, "i:")) != -1)
    switch (ch)
    {
      case 'i':
        inputFile = optarg;
        mandArgs |= 1;
        break;

      case '?':
        /* Just print error message and continue? */
        if (optopt == 'i')
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

/* Return size of inputFile */
int get_input_size (char *inputFile)
{
  struct stat buf = {};
  int fileSize = 0;

  if (stat(inputFile, &buf) == -1)
  {
    perror(inputFile);
    return -1;
  }

  fileSize = (int) buf.st_size;

  return fileSize;
}

int main (int argc, char **argv)
{
  FILE *fIn, *fOut, *receiveCheck;
  char temp[MAX_TRANSFER_SIZE];
  int conf_file_size = 0, i = 0, ch, flag = 0;

  /* Check Setup */
  if (!(argc == 3) || parse_arguments(argc, argv) == -1)
    usage(argv[0]);


  printf("The inputfile is: %s\n", inputFile);
  fIn = open_file(inputFile, "r");
  fOut = open_file(outputFile, "w+");

  conf_file_size = get_input_size(inputFile);
  printf("This confidential file is: %d bytes.\n", conf_file_size);
 
  while (!feof(fIn)) /* Keep channel open until entirety of Writer's buffer has been received */
  {
        for (i = 0; i< MAX_TRANSFER_SIZE; i++)
        {
          if ((ch = fgetc(fIn)) == EOF)
	  {
          flag = 1;
	  break;
	  }
          temp[i] = ch;
	  temp[i] -= ENCRYTPION_KEY; /* Read + Encrypt each char into buffer, then print to outputfile */
        }
	if (flag == 1) /* If remainder of file does no fill buffer */
	{
	  temp[i] = '\0';
 	  printf("%s\n", temp);
	  fprintf(fOut, "%s", temp);
          fclose(fOut);
	}
	else
	{	
	temp[MAX_TRANSFER_SIZE] = '\0';
 	printf("%s\n", temp);
	fprintf(fOut, "%s", temp);
        fclose(fOut);
	}
        while (1) /* Check for the pressence of file 2, the reader's signal to create a new "/tmp/1" and update with new data */
	{
		receiveCheck = fopen("/tmp/2", "r");		
		if (receiveCheck != NULL)
		{
		fOut = open_file(outputFile, "w+");
		sleep(1);
		break;
		}
	}
}
/* Destroy the evidence when channel usage complete! */
fclose(fOut);
fclose(fIn);
remove("/tmp/1");
return 0;
}
