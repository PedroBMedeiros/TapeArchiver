#ifndef ARCHIVEBUILD_H
#define ARCHIVEBUILD_H

int archive_build(int howmany, char *argv[], int options[6], bool strict, 
                                                        bool verbose);

int file_archive(FILE *archive, char *filename);
int directory_archive(FILE* archive, char *dirname, bool strict, bool verbose);

#endif