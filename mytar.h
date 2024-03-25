#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>


#ifndef ARCHIVEPRINT_H
#include "./archiveprint.h"
#endif

#ifndef ARCHIVEEXTRACT_H
#include "./archiveextract.h"
#endif

#ifndef ARCHIVEBUILD_H
#include "./archivebuild.h"
#endif
#include <fcntl.h>


 
#ifndef MYTAR_H
#define MYTAR_H

#define COMMAND_LINE_OPTIONS 6
#define BLOCK_SIZE 512 /* block size in bytes*/
#define PERMISSIONS_WIDTH 10
#define OWNER_GROUP_WIDTH 17
#define SIZE_WIDTH 8
#define MTIME_WIDTH 16
#define MAXLENGTH_HEADER_FIELDS 155
#define MAX_PATH_LENGTH 256
#define REG '0'
#define REG2 '\0'
#define LINK '2'
#define DIRECTORY '5'
#define MAGIC "ustar"
#define VERSION "00"

#define BASE_8 8
#define WHITE_SPACE_ASCII 32
 
#define NAME_SIZE 100
#define MODE_SIZE 8
#define ID_SIZE 8
#define SIZE_SIZE 12
#define MTIME_SIZE 12
#define CHKSUM_SIZE 8
#define CHKSM_BEGIN 148
#define CHKSM_END 155
#define TYPEFLAG_SIZE 1
#define LINKNAME_SIZE 100
#define MAGIC_SIZE 6
#define VERSION_SIZE 2
#define UGNAME_SIZE 32
#define DEV_MAJOR_MINOR_SIZE 8
#define PREFIX_SIZE 155
#define PADDING_SIZE 12

#define MY_ERROR_1 1

/* struct for verbose option printing */
typedef struct Listing {
  char permissions[PERMISSIONS_WIDTH +1];
  char owner_group[OWNER_GROUP_WIDTH +1];
  char size[SIZE_WIDTH +1];
  char mtime[MTIME_WIDTH +1];
  char *pathname;
} Listing;

/* per figure 2 of asgn4.pdf*/
typedef struct TarHeader { 
  char name[NAME_SIZE];   	/* 0-99 NULL-terminated if NULL fits */
  char mode[MODE_SIZE];       /* 100-107 */
  char uid[ID_SIZE];        /* 108-115 */
  char gid[ID_SIZE];        /* 116-123 */
  char size[SIZE_SIZE];      /* 124-135 */
  char mtime[MTIME_SIZE];     /* 136-147 */
  char chksum[CHKSUM_SIZE];     /* 148-155 */
  char typeflag;      /* 156 */
  char linkname[LINKNAME_SIZE]; /* 157-256 NULL-terminated if NULL fits */
  char magic[MAGIC_SIZE];    /* 257-262 must be “ustar”, NULL-terminated)*/
  char version[VERSION_SIZE];    /* 263-264 must be “00” (zero-zero) */
  char uname[UGNAME_SIZE];     /* 265-296 NULL-terminated*/
  char gname[UGNAME_SIZE];     /* 297-328 NULL-terminated*/
  char devmajor[DEV_MAJOR_MINOR_SIZE];   /* 329-336 */
  char devminor[DEV_MAJOR_MINOR_SIZE];   /* 337-344 */
  char prefix[PREFIX_SIZE];   /* 345-499 */
  char padding[PADDING_SIZE];   /* 500-511 adds 12 to get to 512B for a block*/
} TarHeader;

unsigned int header_decode(FILE *archive, TarHeader *header);
void print_tar_header(TarHeader *header);
long int strtol_no_null(const char *string, int length, int base);
char *st_modeToString(mode_t st_mode, char *result);
char *typeandmode_to_string(char type, mode_t st_mode, char *result);
void print_listing(Listing *ptr, bool verbose);
unsigned int oct_to_dec(char *oct_string);

char *no_slash_string(char* stringIn, char* stringOut);
void no_slash_string2(char* stringIn, char** stringOut);
int compare_two_paths(char* path1, char* path2);

int build_name(char* fullName, char* prefix, char* name);
int extract_elements(FILE* tarFile, TarHeader* header, char* outfile,
                         bool verbose, bool elementsProvided);

int header_encode(TarHeader* header, char* record_name, struct stat *stats,
                                                                 bool strict);

#endif

