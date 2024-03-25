#include "./mytar.h"
#include "./archiveprint.h"
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

/* The name of the archived file is produced by concatenating the prefix
* (if of non-zero length), a slash, and the name field. If the prefix is of
* zero length, name is the complete name.*/
int build_name(char* fullName, char* prefix, char* name) {
    int i;
    /* check for existence of prefix */
    if (strlen(prefix) > 0) {
        strcpy(fullName, prefix);
        strcat(fullName, "/");
        strncat(fullName, name, strlen(name));
    } else { /* copy name to fullName character by character */
        for (i = 0; i < NAME_SIZE; i++) {
            fullName[i] = name[i];
            if (fullName[i] == '\0') {
                break;
            }
        }
        fullName[NAME_SIZE] = '\0';        
    }
    return 0;
}

/* helper function to create listing from header entry */
Listing* header_to_listing(char* fullName, TarHeader* header) {
    Listing* newListing = calloc(1, sizeof(Listing));
    newListing->pathname = malloc((strlen(fullName) + 2) * sizeof(char));
    /* name */
    strcpy(newListing->pathname, fullName);

    /* time */
    time_t t = strtol_no_null(header->mtime, 12, 8);
    struct tm* timeptr;
    timeptr = localtime(&t);
    strftime(newListing->mtime, sizeof(newListing->mtime), 
                        "%Y-%m-%d %H:%M", timeptr);
    
    /* owner/group */
    int u, g, uandg; /* length of user, group, and combined*/
    u = strlen(header->uname);
    g = strlen(header->gname);
    uandg = u + g + strlen("/");
    if (u > OWNER_GROUP_WIDTH) { /*long user name*/
        strncpy(newListing->owner_group, header->uname, u);
        newListing->owner_group[OWNER_GROUP_WIDTH] = '\0';
    } else if (uandg < OWNER_GROUP_WIDTH) { /* both together fit*/
        strncpy(newListing->owner_group, header->uname, u);
        strcat(newListing->owner_group, "/");
        strncat(newListing->owner_group, header->gname, g);
        newListing->owner_group[uandg] = '\0';
    } else if ((u+1) == OWNER_GROUP_WIDTH) { /* user plus / exactly fit*/
        strncpy(newListing->owner_group, header->uname, u);
        strcat(newListing->owner_group, "/");
        newListing->owner_group[OWNER_GROUP_WIDTH] = '\0';
    } else { /*both together go past limit*/
        strncpy(newListing->owner_group, header->uname, u);
        strcat(newListing->owner_group,"/");
        strncat(newListing->owner_group,header->gname, 
                        OWNER_GROUP_WIDTH - (u+1));
        newListing->owner_group[uandg] = '\0';
    }
    
    /* type and permissions */
    char type;
    
    /* char header type flag */
    switch (header->typeflag)
    {
    case DIRECTORY:
        type = 'd';
        break;
    case LINK:
        type = 'l'; /* letter 'l' not number 1*/
        break;
    case REG:
    case REG2:
        type = '-';
        break;
    default:
        type = '-';
        break;
    }


    mode_t mode = strtol_no_null(header->mode, MODE_SIZE, BASE_8);
    typeandmode_to_string(type, mode, newListing->permissions);
    
    /* size */
    sprintf(newListing->size, "%ld", strtol_no_null(header->size, 
                                                SIZE_SIZE, BASE_8));
    return newListing;
}

/* uses print listing helper function to print filename */
int echo_filename(char* fullName, TarHeader* header, bool verbose) {
    Listing* listing;
    listing = header_to_listing(fullName, header);
    print_listing(listing, verbose);
    free(listing->pathname);
    free(listing);
    return 0;
}

/* main function to print contents of archive
    lists the contents of given archive file; one per line */
int archive_print(int argc, char *argv[], int options[COMMAND_LINE_OPTIONS], 
                                                    bool strict, bool verbose) {
    FILE *archive;
    TarHeader *header = malloc(sizeof(TarHeader));
    int i;
    unsigned int checkSum;
    char filename[MAX_PATH_LENGTH];
    bool headerExists = false;
    
    /* open file given in arguments -> archive*/
    if ((archive = fopen(argv[2], "rb")) == NULL) {
        perror("problem opening archive file");
        exit(1);
    }
    
    // printf("reading header of %s\n", argv[2]);
    /* looking for valid headers in archive */
    while((checkSum = header_decode(archive, header)) != 0) {
        if (checkSum == strtol_no_null(header->chksum, CHKSUM_SIZE, BASE_8)) {
            headerExists = true;
            //print_tar_header(header);
            strcpy(filename, header->name);
            filename[0] = '\0';
            if (build_name(filename, header->prefix, header->name) != 0) {
                perror("problem building filename");
                exit(1);
            }

            if (argc > 3) { /* look for listed  files only */
                for (i = 3; i < argc; i++) {
                    if (compare_two_paths(argv[i], filename) == 0) {
                        echo_filename(filename, header, verbose);
                    }
                }
            } else { /* otherwise, echo all files */
                echo_filename(filename, header, verbose); 
            }
        }
    }

    /* if no header found, archive is not valid */
    if (!headerExists) {
        errno = MY_ERROR_1;
        perror("Bad/corrupt tarfile");
        exit(1);
    }

    free(header);
    fclose(archive);
    return 0;
}