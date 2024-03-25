#include "./mytar.h"
#include "./archiveextract.h"
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <pwd.h>
#include <unistd.h>
#include <sys/types.h>



/* main function to extract files from archive*/
int archive_extract(int argc, char *argv[], int options[6], bool strict, 
                                                bool verbose) {
    FILE* archive;
    unsigned int checkSum;
    TarHeader* header = malloc(sizeof(TarHeader));
    int i;
    char filename[MAX_PATH_LENGTH] = "";
    
    if ((archive = fopen(argv[2], "rb")) == NULL) {
        perror("opening archive file");
        exit(1);
    }
    //printf("Reading header of %s\n", argv[2]); /* visualization only */
    

    /* similar logic to archive_print */
    while((checkSum = header_decode(archive, header)) != 0) { 
        /* keep reading until header decode returns zero for last block*/
        if (checkSum == strtol_no_null(header->chksum, CHKSUM_SIZE, BASE_8)) {
            /* everytime checkSum matches, block is header */
            //print_tar_header(header);
            strcpy(filename, header->name);
            filename[0] = '\0';
            if (build_name(filename, header->prefix, header->name) != 0) {
                perror("problem building filename");
                exit(1);
            }

            if (argc > 3) { /* look for listed  files only */
                for (i = 3; i < argc; i++) {
                    /* extract only requested files */
                    if (compare_two_paths(argv[i], filename) == 0) {
                        //printf("Extracting: %s\n", argv[i]);
                        if (extract_elements(archive, header, filename, 
                                                        verbose, true)!= 0) {
                            perror("Extracting file from archive");
                            exit(1);
                        }
                    }
                }
            } else { /* otherwise, look for all files */
                //printf("Extracting ..."); 
                if (extract_elements(archive, header, filename, 
                                                    verbose, true) != 0) {
                    perror("Extracting file from archive");
                    exit(1);
                }
            }
        }
    }

    fclose(archive);
    free(header);
   
    return 0;
}

/* subroutine to extract individual elements from archive*/
int extract_elements(FILE* tarFile, TarHeader* header, char* outfile, 
                                        bool verbose, bool elementsProvided) {
    mode_t mode;
    uid_t uid;
    gid_t gid;
    int outfileSize, cumulativeSize;
    FILE* outfileptr;
    char readBuffer[BLOCK_SIZE];
    int i, slashCount;
    char* token; 
    char dirPathToBeCreated[MAX_PATH_LENGTH] = "";
    char dirPathToBeChopped[MAX_PATH_LENGTH] = "";

    /* verbose option toggled */
    if (verbose) {
        printf("%.*s\n", 100, outfile);
    }   
    
    /* used to create parent directories when a list of elements is provided */
    if (elementsProvided) {
        slashCount = 0;
        for (i = 0; i < strlen(outfile); i++) {
            if (outfile[i] == '/') {
                slashCount++;
            }
        }

        if (slashCount > 0) {
            strcpy(dirPathToBeChopped, outfile);
            token = strtok(dirPathToBeChopped, "/");
            strcat(dirPathToBeCreated, token);
            strcat(dirPathToBeCreated, "/");
            mkdir(dirPathToBeCreated, 0755);
            while (token != NULL && slashCount > 1) {
                //printf("%s\n", token);
                token = strtok(NULL, "/");
                strcat(dirPathToBeCreated, token);
                strcat(dirPathToBeCreated, "/");
                mkdir(dirPathToBeCreated, 0755);
                slashCount--;
            }
        }
    }

    /* selecting corrent element type */
    switch (header->typeflag)
    {
    /* extracting files */
    case REG:
    case REG2: /* regular file */
        outfileSize = strtol_no_null(header->size, SIZE_SIZE, BASE_8);
        cumulativeSize = 0;        

        
        if ((outfileptr = fopen(outfile, "wb")) == NULL) {
            perror("cannot open file to write");
            exit(1);
        }
        while((fread(readBuffer, BLOCK_SIZE, 1, tarFile) != 0) && 
                (!feof(tarFile)) && 
                                ((cumulativeSize + BLOCK_SIZE) < outfileSize)) {
            /* write one block at a time */
            if(fwrite(readBuffer, BLOCK_SIZE, 1, outfileptr) != 1) {
                perror("Problem writing extracted file");
                exit(1);
            }
            cumulativeSize += BLOCK_SIZE; /* keep track of written size */
            /* reset buffer for next iteration */
            memset(readBuffer, 0, sizeof(readBuffer)); 
        }
        /* write final block */
        if ((outfileSize - cumulativeSize) > 0) {
            if(fwrite(readBuffer, outfileSize - cumulativeSize, 1, outfileptr)
                                                                         != 1) {
                perror("Problem writing extracted file");
                exit(1);
            }
        }
        fclose(outfileptr);
        /* set file metadata */
        mode = strtol_no_null(header->mode, MODE_SIZE, BASE_8);
        if (chmod(header->name, mode) != 0) {
            //perror("problem setting mode of file");
        }
        uid = strtol_no_null(header->uid, ID_SIZE, BASE_8);
        gid = strtol_no_null(header->gid, ID_SIZE, BASE_8);
        if (chown(header->name, uid, gid) != 0) {
            //perror("problem setting uid and gid of file");
        }
        break;
    
    /* extracting directories */
    case DIRECTORY:
        mode = strtol_no_null(header->mode, MODE_SIZE, BASE_8);
        if (mkdir(header->name, mode) != 0) {
            perror("Problem creating directory");
        }
        uid = strtol_no_null(header->uid, ID_SIZE, BASE_8);
        gid = strtol_no_null(header->gid, ID_SIZE, BASE_8);
        if (chown(header->name, uid, gid) != 0) {
            //perror("problem setting uid and gid of directory");
        }   
        break;
    
    /* extracting links */
    case LINK:
        if (symlink(header->linkname, header->name) != 0) {
            perror("Problem creating symbolic link");
            exit(1);
        }
        mode = strtol_no_null(header->mode, MODE_SIZE, BASE_8);
        if (chmod(header->name, mode) != 0) {
            perror("problem setting mode of link");
        }
        uid = strtol_no_null(header->uid, ID_SIZE, BASE_8);
        gid = strtol_no_null(header->gid, ID_SIZE, BASE_8);
        if (chown(header->name, uid, gid) != 0) {
            perror("problem setting uid and gid of directory");
        }   
        break;
    
    default:
        perror("unsupported element type listed in archive");
        exit(1);
        break;
    }
    
    
    return 0;
}