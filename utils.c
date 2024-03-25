#include "./mytar.h"
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>

/* header visualization for debugging*/
void print_tar_header(TarHeader *header){
  printf("name:\t\t%-100s\n", header->name);  
  printf("mode:\t\t%-8s\n", header->mode);    
  printf("uid:\t\t%-8s\n", header->uid);    
  printf("gid:\t\t%-8s\n", header->gid);      
  printf("size:\t\t%-12s\n", header->size);  
 
  printf("mtime:\t\t%-12s ", header->mtime);  
  time_t time_t_mtime = strtol_no_null(header->mtime, 12, 8);
  struct tm *tm = localtime(&time_t_mtime);
  char time_formatted_string[100];
  strftime(time_formatted_string, sizeof(time_formatted_string),
                     "%Y-%m-%d %H:%M", tm);
  printf("The time formatted string is: %s\n", time_formatted_string);
 
   
  printf("chksum:\t\t%-8s\n", header->chksum);    
  printf("typeflag:\t%c\n", header->typeflag);    
  printf("linkname:\t\t%-100s\n", header->linkname);
  printf("magic:\t\t%-6s\n", header->magic);      
  printf("version:\t\t%-2s\n", header->version);  
  printf("uname:\t\t%-32s\n", header->uname);    
  printf("gname:\t\t%-32s\n", header->gname);    
  printf("devmajor:\t\t%-8s\n", header->devmajor);  
  printf("devminor:\t\t%-8s\n", header->devminor);  
  printf("prefix:\t\t%-155s\n", header->prefix);  
  printf("padding:\t\t%-12s\n", header->padding);
 
 
}

/* helper function to convert string to long for string without null */
long int strtol_no_null(const char *string, int length, int base) {
    long int result;
    char *string_with_null = malloc(sizeof(char)*(length+1));
    strncpy(string_with_null, string, length);
    /* adding NULL so strtol is ok*/
    memset(string_with_null + length, '\0', 1);   
    result= strtol(string_with_null, NULL, BASE_8); 
    free(string_with_null);
    return result;
}

/*helper function to convert st_mode to string *
* 0.1.2.3.4.5.6.7.8.9                         *
* d.r.w.x.r.w.x.r.w.x                         */
char *typeandmode_to_string(char type, mode_t st_mode, char *result) {
  result[0] = type;
  result[1] = (st_mode & S_IRUSR) ? 'r' : '-';
  result[2] = (st_mode & S_IWUSR) ? 'w' : '-';
  result[3] = (st_mode & S_IXUSR) ? 'x' : '-';
  result[4] = (st_mode & S_IRGRP) ? 'r' : '-';
  result[5] = (st_mode & S_IWGRP) ? 'w' : '-';
  result[6] = (st_mode & S_IXGRP) ? 'x' : '-';
  result[7] = (st_mode & S_IROTH) ? 'r' : '-';
  result[8] = (st_mode & S_IWOTH) ? 'w' : '-';
  result[9] = (st_mode & S_IXOTH) ? 'x' : '-';
  result[10] = '\0';
  return result;
}

/* removing ending slash when present in file argument */
char *no_slash_string(char* stringIn, char* stringOut) {
    int n, j;
    n = strlen(stringIn);
    /* one by one; check if last char is slash;*/
    if (stringIn[n - 1] == '/') {
        for (j = 0; j < (n -1); j++) {
            stringOut[j] = stringIn[j];
        }
        stringOut[j] = '\0';
    } else {
        strcpy(stringOut, stringIn);
    }
    //printf("noSlashName: %s\n", stringOut);
    return stringOut;
}

/* helper function to compare filenames to create a match for children 
    directories and files */
int compare_two_paths(char* shortPath, char* longPath) {
  /* use string size of smaller one */
  char compare1[strlen(shortPath) + 1];
  char compare2[strlen(shortPath) + 1];
  int result;
  compare1[0] = '\0';
  compare2[0] = '\0';
  /* chop longer string before comparison */
  strncpy(compare1, longPath, strlen(shortPath));
  *(compare1 + strlen(shortPath)) = '\0';
  strcpy(compare2, shortPath);

  result = strcmp(compare1, compare2);
  return result;
}
