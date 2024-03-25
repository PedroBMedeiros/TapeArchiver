#include "./mytar.h"
#include "./archivebuild.h"
#include <unistd.h>
#include <string.h>


/* main routine for building tar archive */
int archive_build(int howmany, char *argv[], int options[6], bool strict, 
                                                                bool verbose) {
    FILE* archive;
    int i, j, n;
    struct stat stats;
    TarHeader* header = calloc(1, sizeof(TarHeader));
    char empty_block[BLOCK_SIZE] = {0}; /* used to finalize files */
    char noSlashName[MAX_PATH_LENGTH] = {0};
    int archivedCount = 0; /* check if at least one file was archived */
    char filename[NAME_SIZE + 1] = "";
    

    if ((archive = fopen(argv[2], "wb")) == NULL) {
        perror("cannot open file to write archive");
        exit(1);
    }
    for (i = 0; i < howmany; i++) { /* loop through passed files */
        if (lstat(argv[i+3], &stats) == 0) {
            
            /* build header */
            
            if (header_encode(header, no_slash_string(argv[i+3], noSlashName), 
                                                        &stats, strict) != 0) {
                perror("problem encoding header");
                exit(1);
            }
            //print_tar_header(header);

            /* verbose option */
            strcpy(filename, header->name);
            if (verbose) {
                printf("%.*s\n", 100, filename);
            }

            /* checking for filetype, for selecting correct archiving type */
            if (S_ISLNK(stats.st_mode)) { /* link */
                if (fwrite(header, BLOCK_SIZE, 1, archive) != 1) {
                    perror("problem writing header to archive");
                    exit(1);
                }

            } else if (S_ISREG(stats.st_mode)) { /* regular file */
                if (fwrite(header, BLOCK_SIZE, 1, archive) != 1) {
                    perror("problem writing header to archive");
                    exit(1);
                } 
                /* archive file */
                if (file_archive(archive, 
                            no_slash_string(argv[i+3], noSlashName)) == 0) {
                    archivedCount++;
                } 

            } else if (S_ISDIR(stats.st_mode)) { /* directory */
                if (fwrite(header, BLOCK_SIZE, 1, archive) != 1) {
                    perror("problem writing header to archive");
                    exit(1);
                }
                /* enter directory to archive */
                if (directory_archive(archive, 
                    no_slash_string(argv[i+3], noSlashName), 
                                    strict, verbose) == 0) {
                        archivedCount++;
                    }
            } 
        } else {
            perror("unable to lstat file to be archived. Moving on");
            // exit(1);
        }
    }
    /* add 2 empty blocks at the end per specification then close archive */
    /* only when at least one item has been archived */
    if (archivedCount > 0) {
        if(fwrite(empty_block, BLOCK_SIZE, 1, archive)!=1) {
            perror("problem writing file in archive");
            exit(1);
        }
        if(fwrite(empty_block, BLOCK_SIZE, 1, archive)!=1) {
            perror("problem writing file in archive");
            exit(1);
        }
        free(header);
    }
    fclose(archive);
    
    return archivedCount;
}

/* subroutine of archive_build to encode header */
int header_encode(TarHeader* header, char* record_name, struct stat *stats,
                                                                 bool strict) {
    int i, j, k, m, p;
    char zeroes[SIZE_SIZE] = "00000000000"; /* for directory and link size */
    struct passwd* uid;
    struct group* gid;
    unsigned int checkSum = 0;
    char* headerptr = header->name;

    /* header-> name and header->prefix*/ /* depend on length */
    /* When creating archives, as much of the name as will fit should be      *
    *  placed in the name field, with the (LEFT) overflow going to prefix.    */
    /* The name of the archived file is produced by concatenating the prefix  *
    * (if of non-zero length), a slash, and the name field. If the prefix is  *
    * of zero length, name is the complete name.                              */

    if (strlen(record_name) < NAME_SIZE) { /* name fits */
        strcpy(header->name, record_name);
    } else { /* doesn't fit */
        /* vvvvv Visualization of procedure vvvvvv*/
        /* |........................NAME IS LONG|ER THAN 100 BYTES */
        /* |(prefix )\0-------------------------|(name  up  to 100)*/
        j = strlen(record_name) - NAME_SIZE;
        k = j;
        while (record_name[k] != '/') {
            k++; 
            if(k > strlen(record_name)) {
                break;
            }
        }
        strncpy(header->prefix, record_name, k); /* prefix build */
        for(m = k + 1, p = 0; m < strlen(record_name); m++, p++) {
            header->name[p] = record_name[m];
        }
        
        if  (m < NAME_SIZE) {
            header->name[m] = '\0';
        }
    }
    
    /*header->mode, uid, gid*/
    /* The numeric representation of the file protection modes stored as an 
    octal number in an ASCII string terminated by one or more space or null 
    characters.*/
    sprintf(header->mode, "%07o", stats->st_mode);  /* 7-wide octal with leading
                                                     0 and Null at the end*/
    header->mode[0] = '0'; /* putting zeros to match header seen from tar*/
    header->mode[1] = '0';
    header->mode[2] = '0';

    sprintf(header->uid, "%07o", stats->st_uid);
    sprintf(header->gid, "%07o", stats->st_gid);

    /* header->size and typeflag depend on type of record */
    /* The size of the file encoded as an octal number in an ASCII string
    * terminated by one or more space or nul characters.The size of symlinks and
    * directories is zero. 
    * Typeflag ’0’ and ’\0’ Reg; ’2’ link; ’5’ directory */

    if (S_ISREG(stats->st_mode)) { /* regular file */
        header->typeflag = REG;
        sprintf(header->size, "%011lo", stats->st_size); /* */
    } else if (S_ISDIR(stats->st_mode)) { /* directory */
        header->typeflag = DIRECTORY;
        strcpy(header->size, zeroes);
    } else if (S_ISLNK(stats->st_mode)) { /* link */
        header->typeflag = LINK;
        strcpy(header->size, zeroes);
        if (readlink(record_name, header->linkname, LINKNAME_SIZE) == -1) {
            perror("problem reading linkname of record to be archived");
            exit(1);
        }
    }

    if (header->typeflag == DIRECTORY) {
        strcat(header->name, "/");
    }

    /* header->mtime */
    /* octal number in an ASCII string terminated by null */
    sprintf(header->mtime, "%011lo", stats->st_mtime); /*11-wide plus Null*/
    
    /* header-> magic always "ustar\0" */
    strcpy(header->magic, MAGIC);
    /* hesder->version always "00" */
    //strcpy(header->version, VERSION); /* \0 will spill over but ok */    

    /* header->uname and gname*/
    /* numeric user/group id owner, encoded as an octal number in an ascii 
    string terminated by or null*/
    uid = getpwuid(stats->st_uid); 
    gid = getgrgid(stats->st_gid);
    strcpy(header->uname, uid->pw_name);
    strcpy(header->gname, gid->gr_name);
    
    /* dev major and dev minor */
    header->devmajor[0] = '\0';
    header->devminor[0] = '\0';

    /* padding */
    char padding[PADDING_SIZE] = {0};
    strncpy(header->padding, padding, PADDING_SIZE);

    /*header->checksum*/
    /* must add up bytes excluding checksum then add 8 x value of spaces*/
    /* encoded as an octal number in an ASCII string terminated by null */
    for (i = 0; i < BLOCK_SIZE; i++) {
        if (i < CHKSM_BEGIN || i > CHKSM_END) { /* excluding checkSum area*/
            checkSum+= (unsigned int) *(headerptr + i);
        } else {
            checkSum += WHITE_SPACE_ASCII; /* adding whitespaces for checkSum*/
        }
    }

    sprintf(header->chksum, "%06o", checkSum);

    return 0;
}

/* subroutine to write file types to archive*/
int file_archive(FILE *archive, char *filename) {
    FILE* infileptr;
    char readBuffer[BLOCK_SIZE] = {0};

    if ((infileptr = fopen(filename, "rb")) == NULL) {
        perror("opening file to write in archive. moving onto next one");
        return 0;
    }
    while ((fread(readBuffer, BLOCK_SIZE, 1, infileptr) != 0) && 
                                                        (!feof(infileptr))) {
        /* write one block at a time */
        if (fwrite(readBuffer, BLOCK_SIZE, 1, archive) != 1) {
            perror("problem writing file in archive");
            exit(1);
        }
        memset(readBuffer, 0, sizeof(readBuffer)); /* erase buffer every time*/
    }
    /* write final block */
    if (fwrite(readBuffer, BLOCK_SIZE, 1, archive) != 1) {
            perror("problem writing file in archive");
            exit(1);
    }
    fclose(infileptr);
    return 0;
}

/* subroutine to write directory types to archive */
int directory_archive(FILE* archive, char *dirname, bool strict, bool verbose) {
    DIR* dirptr;
    char *dirNameSlash, *appendedName;
    struct stat dirstats;
    struct stat fullNameStats;
    struct dirent* direntries;
    TarHeader* header = calloc(1, sizeof(TarHeader));
    char filename[NAME_SIZE + 1] = "";

    appendedName = malloc(sizeof(*appendedName));
    dirNameSlash = 
            malloc(sizeof(*dirNameSlash) + strlen(dirname) + strlen("/") + 1);

    strcpy(dirNameSlash, dirname);
    strcpy(dirNameSlash + strlen(dirname), "/");

    if ((dirptr = opendir(dirname)) == NULL) {
        perror("problem opening directory");
        exit(1);
    }
    if (lstat(dirname, &dirstats) != 0) {
        perror("problem getting stats");
        exit(1);
    }
    /* reading directory entries one at a time */
    while ((direntries = readdir(dirptr)) != NULL) {
        /* eliminate . directory comparing inode values like in mypwd */
        if ((direntries->d_name[0] != '.') && 
                                    (direntries->d_ino != dirstats.st_ino)) {
            appendedName = realloc(appendedName, strlen(dirNameSlash) + 
                                                strlen(direntries->d_name) + 1);
            appendedName[0] = '\0';
            strcpy(appendedName, dirNameSlash);
            strcat(appendedName, direntries->d_name);
            
            if (lstat(appendedName, &fullNameStats) == 0) {
                /* building header */
                if (header_encode(header ,appendedName, &fullNameStats, strict)
                                                                 != 0) {
                    perror("problem encoding header");
                }
                /* verbose option toggled */
                strcpy(filename, header->name);
                if (verbose) {
                    printf("%.*s\n", 100, filename);
                }

                /* execute recursion based on type of element */
                if (S_ISLNK(fullNameStats.st_mode)) { /* link */
                    if (fwrite(header, BLOCK_SIZE, 1, archive) != 1) {
                        perror("problem writing header");
                        exit(1);
                    }
                } else if (S_ISREG(fullNameStats.st_mode)) { /* regular file */
                    if (fwrite(header, BLOCK_SIZE, 1, archive) != 1) {
                        perror("problem writing header");
                        exit(1);
                    }
                    file_archive(archive, appendedName);
                } else if (S_ISDIR(fullNameStats.st_mode)) { /* directory */
                    if (fwrite(header, BLOCK_SIZE, 1, archive) != 1) {
                        perror("problem writing header");
                        exit(1);
                    }
                    /* recursion */
                    directory_archive(archive, appendedName, strict, verbose);
                }
            } else {
                perror("problem with lstat on compound file");
                exit(1);
            }
        }
    }

    free(appendedName);
    free(dirNameSlash);
    closedir(dirptr);
    free(header);

    return 0;
}
