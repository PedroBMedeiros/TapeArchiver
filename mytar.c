#include "./mytar.h"
#include <string.h>

#define COMMAND_LINE_OPTIONS 6


/*reads 1 block of archive only and writes into header struct */
unsigned int header_decode(FILE *archive, TarHeader *header) {
    unsigned int checkSum = 0;
    int i;
    char* headerptr = header->name;
    if (fread(header, BLOCK_SIZE, 1, archive) == 0) {
      return checkSum;
    }
    for (i = 0; i < BLOCK_SIZE; i++) {
      if (i < CHKSM_BEGIN || i > CHKSM_END) {
        checkSum += ((unsigned int) *(headerptr + i));
      }
      else {
        checkSum += WHITE_SPACE_ASCII; // ascii value for whitespace
      }
    }
    /*printf("checkSums -> Real: %d Calc: %d\n", oct_to_dec(header->chksum), 
                                                                  checkSum); */
    return checkSum;
}

/* prints listing according to required format*/
void print_listing(Listing *ptr, bool verbose){
  if(verbose == true){
    printf("%10s ", ptr->permissions);
    printf("%-17s ", ptr->owner_group);
    printf("%8s ", ptr->size);
    printf("%16s ", ptr->mtime);
    printf("%.*s\n", MAX_PATH_LENGTH, ptr->pathname);
    // printf("%10s %-17s %8s %16s %s\n", ptr->permissions, ptr->owner_group, 
    //                                   ptr->size, ptr->mtime, ptr->pathname);
  }
  else {
    printf("%.*s\n", MAX_PATH_LENGTH, ptr->pathname);
  }
}


int main(int argc, char *argv[]) {
  char charoptions[COMMAND_LINE_OPTIONS + 1] = 
              { 'c', 't', 'x', 'v', 'f', 'S', '-'}; /* plus 1 to accept '-' */
  int options[COMMAND_LINE_OPTIONS + 1] = {0};
  int i, j, found;
  bool verbose, strict;
  FILE *fptr;
  
  /*visualize command line arguments*/
  /*
    for (i = 0 ; i <argc; i++) {   
        printf("argv[%d]= %s\n",i, argv[i]);
    }
  */
 
  /* not enough arguments */
  if (argc < 2) {
    printf("usage: mytar [ctxvS]f tarfile [ path [ ... ] ]\n");
    perror("invalid command line");
    return 1;
  }

  /*loop to locate valid options*/
  for (i = 0; i < strlen(argv[1]); i++) { 
    for (j = 0; j < strlen(charoptions); j++) {
      found = 0;
      if (argv[1][i] == charoptions[j]) {
        options[j] = 1;
        found = 1;
        break;
      }
    }
    if (found == 0) { /* if there is no match an invalid option was provided*/
      /*printf("invalid option %c\n",argv[1][i]);*/
      printf("usage: mytar [ctxvS]f tarfile [ path [ ... ] ]\n");
      perror("invalid command line");
      return 1;
      }
  }

  /* eliminate invalid option combinations*/ 
  if (options[0]+options[1]+options[2]!=1) { /* c t x are mutually exclusive*/
    printf("usage: mytar [ctxvS]f tarfile [ path [ ... ] ]\n");
    perror("invalid command line");
    return 1;
  }

  if (options[4] < 1) { /* f must be present*/
    //printf("f option is mandatory.\n");
    printf("usage: mytar [ctxvS]f tarfile [ path [ ... ] ]\n");
    perror("invalid command line");
    return 1;
  }

  /* for visualization only*/
  /*
    printf("options(ctxvfS): ");            
    for(int i=0; i<COMMAND_LINE_OPTIONS; i++){
      printf("%d", options[i]);
    }
    printf("\n");
  */

  /* checking for presence of file argument*/
  if(argc < 3) { /* missing file */
    printf("usage: mytar [ctxvS]f tarfile [ path [ ... ] ]\n");
    perror("invalid command line");
    return 1;
  }

  /*call one of 3 possibilities of tar [creation | printing | extraction]*/
  verbose = options[3] == 1 ? true : false; /* flag verbose option in calls*/
  strict = options[5] == 1 ? true : false; /* flag strict option in calls*/
  

  /* creation option */
  if(options[0] == 1){     
    /*printf("entering CREATION routine\n");*/
    /* amount of files to archive equals argc - 3*/
    if (archive_build(argc -3, argv, options, strict, verbose) == 0) {
      perror("Nothing archived");
      return 1;
    }


  /* print option*/
  } else if(options[1] == 1) {     
    /* check if tar file is valid first*/
    /*printf("tar file argument: %s\n",argv[2]);*/

    fptr = fopen(argv[2], "r"); 
    if (fptr == NULL) { /* invalid file name*/
      perror(argv[2]);
      return 1;
    } else {
      fclose(fptr);
    } 
    /*printf("entering PRINT routine\n");*/
    archive_print(argc, argv, options, strict, verbose);
  

  /*extract option*/
  } else if(options[2] == 1 ){     
    /* check if tar file is valid first*/
    /*printf("tar file argument: %s\n",argv[2]);*/
    
    fptr = fopen(argv[2], "r"); 
    if (fptr == NULL) { /* invalid file name*/
      perror(argv[2]);
      return 1;
    } else {
      fclose(fptr);
    } 
    //printf("entering EXTRACTION routine\n");
    archive_extract(argc, argv, options, strict, verbose);

  } else { 
    perror("something went wrong with options");
    return 1;
  }

/*printf("\nDone.\n");*/
return 0;
}
